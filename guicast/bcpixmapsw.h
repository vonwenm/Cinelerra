
/*
 * CINELERRA
 * Copyright (C) 2005 Pierre Dumuid
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * 
 */

#ifndef BCPIXMAPSW_H
#define BCPIXMAPSW_H

#include "bcpixmapsw.h"
#include "vframe.inc"
#include "bcsubwindow.h"


class BC_PixmapSW : public BC_SubWindow 
{
public:
	BC_PixmapSW(int x, int y, BC_Pixmap *thepixmap);
	virtual ~BC_PixmapSW();

	int initialize();
	virtual int handle_event() { return 0; };
	virtual char* get_caption() { return ""; };

	int reposition_widget(int x, int y);

private:
	int draw();
	BC_Pixmap *thepixmap;
};


#endif
