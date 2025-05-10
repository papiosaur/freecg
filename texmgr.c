/* texmgr.c - a simple opengl texture manager
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

#include "texmgr.h"
#include "gfx.h"
#include <math.h>

struct texmgr *tm_request_texture(const SDL_Surface *image)
{
	extern GLuint tm_load_texture(SDL_Surface*);
	struct texmgr *texm = calloc(1, sizeof(*texm));
	texm->w = 1 << (int)ceil(log2(image->w));
	texm->h = 1 << (int)ceil(log2(image->h));
	
	// Modification 1: SDL_CreateRGBSurface utilise 0 au lieu de SDL_SWSURFACE en SDL2
	SDL_Surface *tile = SDL_CreateRGBSurface(0, texm->w, texm->h, 32,
			RMASK, GMASK, BMASK, AMASK);
	
	SDL_Rect rect = {
		.x = 0,
		.y = 0,
		.w = image->w,
		.h = image->h
	};
	
	// Modification 2: Configuration de la surface pour le blending correct
	SDL_SetSurfaceBlendMode(tile, SDL_BLENDMODE_NONE);
	
	if (SDL_MUSTLOCK(tile))
		SDL_LockSurface(tile);
	
	// SDL_BlitSurface reste inchangé
	SDL_BlitSurface((SDL_Surface*)image, &rect, tile, NULL);
	
	texm->texno = tm_load_texture(tile);
	
	if (SDL_MUSTLOCK(tile))
		SDL_UnlockSurface(tile);
	
	SDL_FreeSurface(tile);
	return texm;
}

GLuint tm_load_texture(SDL_Surface *image)
{
	GLuint texno;
	glGenTextures(1, &texno);
	glBindTexture(GL_TEXTURE_2D, texno);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image->w, image->h, 0,
			GL_RGBA, GL_UNSIGNED_BYTE, image->pixels);
	
	// Modification 3: Utilisation de GL_CLAMP_TO_EDGE au lieu de GL_CLAMP, qui est obsolète
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	
	return texno;
}