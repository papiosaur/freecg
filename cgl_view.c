/* Copyright (C) 2010 Michal Trybus.
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

#include "graphics.h"
#include "osd.h"
#include "texmgr.h"
#include "gfx.h"
#include "cg.h"

#include <stdio.h>
#include <assert.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <math.h>
#include <sound.h>

#define MODE SDL_WINDOW_OPENGL
#define SCREEN_W 800
#define SCREEN_H 600
#define SCALE_STEP 0.2
#define SCALE_ASTEP 0.01

static const char *version __attribute__((used)) = "$VER: FreeCG 1.0 (08.05.25) ported by Papiosaur";

unsigned long __stack = 1024 * 1024;

int mouse, running;
SDL_GameController *gameController = NULL;
SDL_Window *window;

void process_event(SDL_Event *e)
{
    switch (e->type) {
    case SDL_QUIT:
        running = 0;
        break;
    case SDL_MOUSEMOTION:
        if (mouse) {
            gl.cam.nx -= e->motion.xrel/gl.cam.scale;
            gl.cam.ny -= e->motion.yrel/gl.cam.scale;
        }
        break;
    case SDL_MOUSEBUTTONUP:
        switch (e->button.button) {
        case 1:
            mouse = 0;
            break;
        case 4:
            gl.cam.scale += 0.2;
            break;
        case 5:
            gl.cam.scale -= 0.2;
            break;
        }
        break;
    case SDL_MOUSEBUTTONDOWN:
        switch(e->button.button) {
        case 1:
            mouse = 1;
            break;
        }
        break;
    case SDL_CONTROLLERAXISMOTION:
        // Gestion des mouvements analogiques - stick gauche (rotation)
        if (e->caxis.axis == SDL_CONTROLLER_AXIS_LEFTX) {
            // Définir une zone morte pour éviter le drift du stick
            if (abs(e->caxis.value) > 3200) {
                // Convertir la valeur du stick (-32768 à 32767) en vitesse de rotation
                float rot_speed = e->caxis.value / 6000.0;
                gl.l->ship->rot_speed = rot_speed;
            } else {
                // Dans la zone morte
                gl.l->ship->rot_speed = 0;
            }
        }
        break;
    case SDL_CONTROLLERBUTTONDOWN:
        // Boutons de manette
        switch (e->cbutton.button) {
        case SDL_CONTROLLER_BUTTON_A:
            // Bouton A - activer le moteur
            cg_ship_set_engine(gl.l->ship, 1);
            break;
        case SDL_CONTROLLER_BUTTON_B:
            // Bouton B
            osd_toggle();
            break;
        case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:
            // Bouton LB - clé 1
            gl.l->ship->keys[0] = !gl.l->ship->keys[0];
            break;
        case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER:
            // Bouton RB - clé 2
            gl.l->ship->keys[1] = !gl.l->ship->keys[1];
            break;
        case SDL_CONTROLLER_BUTTON_BACK:
            // Bouton Back - clé 3 (à la place de LT)
            gl.l->ship->keys[2] = !gl.l->ship->keys[2];
            break;
        case SDL_CONTROLLER_BUTTON_START:
            // Bouton Start - clé 4 (à la place de RT)
            gl.l->ship->keys[3] = !gl.l->ship->keys[3];
            break;
        }
        break;
    case SDL_CONTROLLERBUTTONUP:
        switch (e->cbutton.button) {
        case SDL_CONTROLLER_BUTTON_A:
            // Bouton A - désactiver le moteur quand relâché
            cg_ship_set_engine(gl.l->ship, 0);
            break;
        }
        break;
    case SDL_KEYDOWN:
        switch (e->key.keysym.sym) {
        case SDLK_RETURN:
            if (e->key.keysym.mod & (KMOD_LALT | KMOD_RALT)) {
				Uint32 flags =SDL_GetWindowFlags(window);
				SDL_SetWindowFullscreen(window,
					(flags & SDL_WINDOW_FULLSCREEN) ? 0 : SDL_WINDOW_FULLSCREEN);
			}
            break;
		case SDLK_ESCAPE:
            running = 0;
            break;
        case SDLK_KP_1:
            gl.l->ship->keys[0] = !gl.l->ship->keys[0];
            break;
        case SDLK_KP_2:
            gl.l->ship->keys[1] = !gl.l->ship->keys[1];
            break;
        case SDLK_KP_3:
            gl.l->ship->keys[2] = !gl.l->ship->keys[2];
            break;
        case SDLK_KP_4:
            gl.l->ship->keys[3] = !gl.l->ship->keys[3];
            break;
        case SDLK_LEFT:
            gl.l->ship->rot_speed = -5.5;
            break;
        case SDLK_RIGHT:
            gl.l->ship->rot_speed = 5.5;
            break;
        case SDLK_UP:
			cg_ship_set_engine(gl.l->ship, 1);
			break;
        case SDLK_o:
            osd_toggle();
            break;
        default:
            break;
        }
        break;
    case SDL_KEYUP:
        switch(e->key.keysym.sym) {
        case SDLK_UP:
			cg_ship_set_engine(gl.l->ship, 0);
            break;
        case SDLK_LEFT:
            if (gl.l->ship->rot_speed == -5.5)
                gl.l->ship->rot_speed = 0;
            break;
        case SDLK_RIGHT:
            if (gl.l->ship->rot_speed == 5.5)
                gl.l->ship->rot_speed = 0;
            break;
        default:
            break;
        }
        break;
    }
}

int main(int argc, char *argv[])
{
	if (!(argc == 2 || argc == 4)) {
		printf("Usage: %s file.cgl [width height]\n", argv[0]);
		exit(-1);
	}
	SDL_Surface *gfx = load_gfx("data/GRAVITY.GFX");
	if (!gfx) {
		fprintf(stderr, "read_gfx: %s\n", SDL_GetError());
		abort();
	}
	SDL_Surface *png = load_png("data/font.png");
	if (!png) {
		fprintf(stderr, "load_png: %s\n", SDL_GetError());
		abort();
	}
	SDL_Surface *osd = load_png("data/osd.png");
	if (!osd) {
		fprintf(stderr, "load_png: %s\n", SDL_GetError());
		abort();
	}
	struct cgl *cgl = read_cgl(argv[1], NULL);
	if (!cgl) {
		fprintf(stderr, "read_cgl: %s\n", SDL_GetError());
		abort();
	}
	cgl_preprocess(cgl);
	cg_init(cgl);
	make_collision_map(gfx, cmap);
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMECONTROLLER) != 0) {
		fprintf(stderr, "SDL failed: %s\n", SDL_GetError());
		abort();
	}
	
	// Initialisation des manettes
	int numJoysticks = SDL_NumJoysticks();
	if (numJoysticks > 0) {
		for (int i = 0; i < numJoysticks; i++) {
			if (SDL_IsGameController(i)) {
				gameController = SDL_GameControllerOpen(i);
				if (gameController) {
					printf("Game controller connected: %s\n", SDL_GameControllerName(gameController));
					break;  // On s'arrête au premier contrôleur trouvé
				} else {
					fprintf(stderr, "Could not open gamecontroller %i: %s\n", i, SDL_GetError());
				}
			}
		}
	} else {
		printf("No game controllers connected.\n");
	}
	
	sound_init();
	sound_load();
	
	SDL_GLContext glContext;

	if (argc == 4) {
		int w = atoi(argv[2]),
			h = atoi(argv[3]);
		if (w && h) {
			window = SDL_CreateWindow("FreeCG", 
								SDL_WINDOWPOS_UNDEFINED, 
								SDL_WINDOWPOS_UNDEFINED,
								w, h, 
								SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
		} else {
			fprintf(stderr, "Wrong resolution");
			window = SDL_CreateWindow("FreeCG", 
								SDL_WINDOWPOS_UNDEFINED, 
								SDL_WINDOWPOS_UNDEFINED,
								SCREEN_W, SCREEN_H, 
								SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
		}
	} else {
		window = SDL_CreateWindow("FreeCG", 
							SDL_WINDOWPOS_UNDEFINED, 
							SDL_WINDOWPOS_UNDEFINED,
							SCREEN_W, SCREEN_H, 
							SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
	}

	if (!window) {
		fprintf(stderr, "Window creation failed: %s\n", SDL_GetError());
		abort();
	}

	// Création du contexte OpenGL
	glContext = SDL_GL_CreateContext(window);
	if (!glContext) {
		fprintf(stderr, "OpenGL context creation failed: %s\n", SDL_GetError());
		abort();
	}
	
	gl_set_window(window);
	
	int w, h;
	SDL_GetWindowSize(window, &w, &h);
	gl_resize_viewport(w, h);
	
	struct texmgr *ttm = tm_request_texture(gfx);
	struct texmgr *fnt = tm_request_texture(png);
	struct texmgr *otm = tm_request_texture(osd);
	gl_init(cgl, ttm, fnt, otm);
	int t = SDL_GetTicks(),
	    nt = t,
	    time = t,
	    fr = 0;
	running = 1;
	mouse = 0;
	SDL_Event e;
	
	// Définition des constantes pour la limitation du framerate
    const int TARGET_FPS = 60;
    const int FRAME_DELAY = 1000 / TARGET_FPS;
    int frameStart;
    int frameTime;
	
	while (running) {
		
		// Enregistrement du temps de début de frame
        frameStart = SDL_GetTicks();
		
		while (SDL_PollEvent(&e))
			process_event(&e);
			
		time = SDL_GetTicks();
		nt = time - t;
		cg_step(cgl, time / 1000.0);
		if (nt > 5000) {
			printf("%d frames in %d ms - %.1f fps\n",
					gl.frame - fr, nt, (float)(gl.frame - fr) / nt * 1000);
			if (cgl->status == Lost)
				printf("Dead. Game over!");
			if (cgl->status == Victory)
				printf("You won!");
			fflush(stdout);

			t += nt;
			fr = gl.frame;
		}
		gl.cam.nx = cgl->ship->x + SHIP_W/2.0;
		gl.cam.ny = cgl->ship->y + SHIP_H/2.0;
		gl_update_window(time / 1000.0);
		SDL_GL_SwapWindow(window);
		
		// Calcul du temps écoulé pour cette frame
        frameTime = SDL_GetTicks() - frameStart;
        
        // Si le temps écoulé est inférieur au délai cible, attendre
        if (frameTime < FRAME_DELAY) {
            SDL_Delay(FRAME_DELAY - frameTime);
		}
	}
	
	// Nettoyage final
	if (gameController) {
		SDL_GameControllerClose(gameController);
	}
	sound_free();
	free_cgl(cgl);
	SDL_GL_DeleteContext(glContext);
	SDL_DestroyWindow(window);
	SDL_Quit();
	return 0;
}