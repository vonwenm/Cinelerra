#include "clip.h"
#include "cplayback.h"
#include "cwindow.h"
#include "cwindowgui.h"
#include "edl.h"
#include "edlsession.h"
#include "labels.h"
#include "localsession.h"
#include "mainsession.h"
#include "mtimebar.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "patchbay.h"
#include "playbackengine.h"
#include "plugin.h"
#include "samplescroll.h"
#include "trackcanvas.h"
#include "tracks.h"
#include "transportque.h"
#include "zoombar.h"


void MWindow::update_plugins()
{
// Show plugins which are visible and hide plugins which aren't
// Update plugin pointers in plugin servers
}


int MWindow::expand_sample()
{
	if(gui)
	{
		if(edl->local_session->zoom_sample < 0x100000)
		{
			edl->local_session->zoom_sample *= 2;
			gui->zoombar->sample_zoom->update(edl->local_session->zoom_sample);
			zoom_sample(edl->local_session->zoom_sample);
		}
	}
	return 0;
}

int MWindow::zoom_in_sample()
{
	if(gui)
	{
		if(edl->local_session->zoom_sample > 1)
		{
			edl->local_session->zoom_sample /= 2;
			gui->zoombar->sample_zoom->update(edl->local_session->zoom_sample);
			zoom_sample(edl->local_session->zoom_sample);
		}
	}
	return 0;
}

int MWindow::zoom_sample(int64_t zoom_sample)
{
	CLIP(zoom_sample, 1, 0x100000);
	edl->local_session->zoom_sample = zoom_sample;
	find_cursor();
	gui->get_scrollbars();

	if(!gui->samplescroll) edl->local_session->view_start = 0;
	samplemovement(edl->local_session->view_start);
	gui->zoombar->sample_zoom->update(zoom_sample);
	return 0;
}

void MWindow::find_cursor()
{
// 	if((edl->local_session->selectionend > 
// 		(double)gui->canvas->get_w() * 
// 		edl->local_session->zoom_sample / 
// 		edl->session->sample_rate) ||
// 		(edl->local_session->selectionstart > 
// 		(double)gui->canvas->get_w() * 
// 		edl->local_session->zoom_sample / 
// 		edl->session->sample_rate))
// 	{
		edl->local_session->view_start = Units::round((edl->local_session->selectionend + 
			edl->local_session->selectionstart) / 
			2 *
			edl->session->sample_rate /
			edl->local_session->zoom_sample - 
			(double)gui->canvas->get_w() / 
			2);
// 	}
// 	else
// 		edl->local_session->view_start = 0;

//printf("MWindow::find_cursor %f\n", edl->local_session->view_start);
	if(edl->local_session->view_start < 0) edl->local_session->view_start = 0;
}


void MWindow::fit_selection()
{
	if(edl->local_session->selectionstart == edl->local_session->selectionend)
	{
		double total_samples = edl->tracks->total_length() * edl->session->sample_rate;
		for(edl->local_session->zoom_sample = 1; 
			gui->canvas->get_w() * edl->local_session->zoom_sample < total_samples; 
			edl->local_session->zoom_sample *= 2)
			;
	}
	else
	{
		double total_samples = (edl->local_session->selectionend - edl->local_session->selectionstart) * edl->session->sample_rate;
		for(edl->local_session->zoom_sample = 1; 
			gui->canvas->get_w() * edl->local_session->zoom_sample < total_samples; 
			edl->local_session->zoom_sample *= 2)
			;
	}

	edl->local_session->zoom_sample = MIN(0x100000, edl->local_session->zoom_sample);
	zoom_sample(edl->local_session->zoom_sample);
}

void MWindow::zoom_amp(int64_t zoom_amp)
{
	edl->local_session->zoom_y = zoom_amp;
	gui->canvas->draw(0, 0);
	gui->canvas->flash();
	gui->patchbay->update();
	gui->flush();
}

void MWindow::zoom_track(int64_t zoom_track)
{
	edl->local_session->zoom_y = (int64_t)((float)edl->local_session->zoom_y * 
		zoom_track / 
		edl->local_session->zoom_track);
	CLAMP(edl->local_session->zoom_y, MIN_AMP_ZOOM, MAX_AMP_ZOOM);
	edl->local_session->zoom_track = zoom_track;
	trackmovement(edl->local_session->track_start);
//printf("MWindow::zoom_track %d %d\n", edl->local_session->zoom_y, edl->local_session->zoom_track);
}

void MWindow::trackmovement(int track_start)
{
	edl->local_session->track_start = track_start;
	if(edl->local_session->track_start < 0) edl->local_session->track_start = 0;
	edl->tracks->update_y_pixels(theme);
	gui->get_scrollbars();
	gui->canvas->draw(0, 0);
	gui->patchbay->update();
	gui->canvas->flash();
	gui->flush();
}

void MWindow::move_up(int64_t distance)
{
	if(!gui->trackscroll) return;
	if(distance == 0) distance = edl->local_session->zoom_track;
	edl->local_session->track_start -= distance;
	trackmovement(edl->local_session->track_start);
}

void MWindow::move_down(int64_t distance)
{
	if(!gui->trackscroll) return;
	if(distance == 0) distance = edl->local_session->zoom_track;
	edl->local_session->track_start += distance;
	trackmovement(edl->local_session->track_start);
}

int MWindow::goto_end()
{
	int64_t old_view_start = edl->local_session->view_start;

	if(edl->tracks->total_length() > (double)gui->canvas->get_w() * 
		edl->local_session->zoom_sample / 
		edl->session->sample_rate)
	{
		edl->local_session->view_start = 
			Units::round(edl->tracks->total_length() * 
				edl->session->sample_rate /
				edl->local_session->zoom_sample - 
				gui->canvas->get_w() / 
				2);
	}
	else
	{
		edl->local_session->view_start = 0;
	}

	if(gui->shift_down())
	{
		edl->local_session->selectionend = edl->tracks->total_length();
	}
	else
	{
		edl->local_session->selectionstart = 
			edl->local_session->selectionend = 
			edl->tracks->total_length();
	}

	if(edl->local_session->view_start != old_view_start) 
		samplemovement(edl->local_session->view_start);

	update_plugin_guis();
	gui->patchbay->update();
	gui->cursor->update();
	gui->canvas->activate();
	gui->zoombar->update();
	cwindow->update(1, 0, 0, 0, 0);
	return 0;
}

int MWindow::goto_start()
{
	int64_t old_view_start = edl->local_session->view_start;

	edl->local_session->view_start = 0;
	if(gui->shift_down())
	{
		edl->local_session->selectionstart = 0;
	}
	else
	{
		edl->local_session->selectionstart = 
			edl->local_session->selectionend = 
			0;
	}

	if(edl->local_session->view_start != old_view_start)
		samplemovement(edl->local_session->view_start);

	update_plugin_guis();
	gui->patchbay->update();
	gui->cursor->update();
	gui->canvas->activate();
	gui->zoombar->update();
	cwindow->update(1, 0, 0, 0, 0);
	return 0;
}

int MWindow::samplemovement(int64_t view_start)
{
	edl->local_session->view_start = view_start;
	if(edl->local_session->view_start < 0) edl->local_session->view_start = 0;
	gui->canvas->draw();
	gui->cursor->show();
	gui->canvas->flash();
	gui->timebar->update();
	gui->zoombar->update();

	if(gui->samplescroll) gui->samplescroll->set_position();
	return 0;
}

int MWindow::move_left(int64_t distance)
{
	if(!distance) 
		distance = gui->canvas->get_w() / 
			10;
	edl->local_session->view_start -= distance;
	if(edl->local_session->view_start < 0) edl->local_session->view_start = 0;
	samplemovement(edl->local_session->view_start);
	return 0;
}

int MWindow::move_right(int64_t distance)
{
	if(!distance) 
		distance = gui->canvas->get_w() / 
			10;
	edl->local_session->view_start += distance;
	samplemovement(edl->local_session->view_start);
	return 0;
}

void MWindow::select_all()
{
	edl->local_session->selectionstart = 0;
	edl->local_session->selectionend = edl->tracks->total_length();
	gui->update(0, 1, 1, 1, 0, 1, 0);
	gui->canvas->activate();
	cwindow->update(1, 0, 0);
}

int MWindow::next_label()
{
	Label *current;
	Labels *labels = edl->labels;

// Test for label under cursor position
	for(current = labels->first; 
		current && !edl->equivalent(current->position, edl->local_session->selectionend); 
		current = NEXT)
		;

// Test for label before cursor position
	if(!current)
		for(current = labels->last;
			current && current->position > edl->local_session->selectionend;
			current = PREVIOUS)
			;

// Test for label after cursor position
	if(!current)
		current = labels->first;
	else
// Get next label
		current = NEXT;

	if(current)
	{

		edl->local_session->selectionend = current->position;
		if(!gui->shift_down()) edl->local_session->selectionstart = edl->local_session->selectionend;

		if(edl->local_session->selectionend >= (double)edl->local_session->view_start *
			edl->local_session->zoom_sample /
			edl->session->sample_rate + 
			gui->canvas->time_visible() ||
			edl->local_session->selectionend < (double)edl->local_session->view_start *
			edl->local_session->zoom_sample /
			edl->session->sample_rate)
		{
			samplemovement((int64_t)(edl->local_session->selectionend *
				edl->session->sample_rate /
				edl->local_session->zoom_sample - 
				gui->canvas->get_w() / 
				2));
			cwindow->update(1, 0, 0, 0, 0);
		}
		else
		{
			update_plugin_guis();
			gui->patchbay->update();
			gui->timebar->update();
			gui->cursor->hide();
			gui->cursor->draw();
			gui->zoombar->update();
			gui->canvas->flash();
			gui->flush();
			cwindow->update(1, 0, 0);
		}
	}
	else
	{
		goto_end();
	}
	return 0;
}

int MWindow::prev_label()
{
	Label *current;
	Labels *labels = edl->labels;

// Test for label under cursor position
	for(current = labels->first; 
		current && !edl->equivalent(current->position, edl->local_session->selectionstart); 
		current = NEXT)
		;

// Test for label after cursor position
	if(!current)
		for(current = labels->first;
			current && current->position < edl->local_session->selectionstart;
			current = NEXT)
			;

// Test for label before cursor position
	if(!current) 
		current = labels->last;
	else
// Get previous label
		current = PREVIOUS;

	if(current)
	{
		edl->local_session->selectionstart = current->position;
		if(!gui->shift_down()) edl->local_session->selectionend = edl->local_session->selectionstart;

// Scroll the display
		if(edl->local_session->selectionstart >= edl->local_session->view_start *
			edl->local_session->zoom_sample /
			edl->session->sample_rate + 
			gui->canvas->time_visible() 
		||
			edl->local_session->selectionstart < edl->local_session->view_start *
			edl->local_session->zoom_sample /
			edl->session->sample_rate)
		{
			samplemovement((int64_t)(edl->local_session->selectionstart *
				edl->session->sample_rate /
				edl->local_session->zoom_sample - 
				gui->canvas->get_w() / 
				2));
			cwindow->update(1, 0, 0, 0, 0);
		}
		else
// Don't scroll the display
		{
			update_plugin_guis();
			gui->patchbay->update();
			gui->timebar->update();
			gui->cursor->hide();
			gui->cursor->draw();
			gui->zoombar->update();
			gui->canvas->flash();
			gui->flush();
			cwindow->update(1, 0, 0);
		}
	}
	else
	{
		goto_start();
	}
	return 0;
}

int MWindow::expand_y()
{
	int result = edl->local_session->zoom_y * 2;
	result = MIN(result, MAX_AMP_ZOOM);
	zoom_amp(result);
	gui->zoombar->update();
	return 0;
}

int MWindow::zoom_in_y()
{
	int result = edl->local_session->zoom_y / 2;
	result = MAX(result, MIN_AMP_ZOOM);
	zoom_amp(result);
	gui->zoombar->update();
	return 0;
}

int MWindow::expand_t()
{
	int result = edl->local_session->zoom_track * 2;
	result = MIN(result, MAX_TRACK_ZOOM);
	zoom_track(result);
	gui->zoombar->update();
	return 0;
}

int MWindow::zoom_in_t()
{
	int result = edl->local_session->zoom_track / 2;
	result = MAX(result, MIN_TRACK_ZOOM);
	zoom_track(result);
	gui->zoombar->update();
	return 0;
}
