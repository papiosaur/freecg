/* graphics.h - renderer's data structures and animation constants
 * Copyright (C) 2010 Michal Trybus.
 *
 * This file is part of FreeCG.
 *
 * FreeCG is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * FreeCG is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with FreeCG. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef GRAPHICS_H
#define GRAPHICS_H

#include "cg.h"
#include "texmgr.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>

struct camera {
	double x, y;
	double nx, ny;
	double scale;
};
struct glengine {
	double time;
	struct texmgr *ttm,
		      *ftm,
		      *otm;
	struct drect viewport;
	struct camera cam;
	double win_w, win_h;
	struct cgl *l;
	unsigned int frame;
	GLuint curtex;
};
extern struct glengine gl;

void gl_init(struct cgl*, struct texmgr*, struct texmgr*, struct texmgr*);
void gl_resize_viewport(double, double);
void gl_set_window(SDL_Window *window);
void gl_update_window(double);

static inline void gl_bind_texture(struct texmgr *tm)
{
	if (gl.curtex != tm->texno) {
		glBindTexture(GL_TEXTURE_2D, tm->texno);
		gl.curtex = tm->texno;
	}
}

#define BLINK_SPEED 1.8
#define CAM_SPEED 2

#endif
