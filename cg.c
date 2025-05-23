/* cg.c - game logic simulator routines
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

#include "cg.h"
#include "mathgeom.h"
#include <stdlib.h>
#include <math.h>
#include <float.h>
#include <assert.h>
#include "sound.h"
#include <SDL2/SDL.h>

collision_map cmap;

/* ==================== Ship ==================== */
void cg_revert_held_freigh(struct cgl *l)
{
	extern void airport_push_cargo(struct airport*);
	for (size_t i = 0; i < l->ship->num_freight; ++i)
		airport_push_cargo(l->ship->freight[i].ap);
	l->ship->num_freight = 0;
}
void cg_restart_ship(struct cgl *l)
{
	l->ship->vx = l->ship->vy = 0;
	l->ship->rot_speed = 0;
	l->ship->x = l->hb->base->x + (l->hb->base->w - SHIP_W)/2,
	l->ship->y = l->hb->base->y - 20;
	l->ship->engine = 0;
	l->ship->rot = 3/2.0 * M_PI; /* vertical */
	l->ship->airport = l->hb;
	l->ship->fuel = MAX_FUEL;
	l->ship->dead = 0;
	cg_revert_held_freigh(l);
}
void cg_ship_init(struct cgl *l)
{
	cg_restart_ship(l);
	l->ship->has_turbo = 0;
	l->ship->max_freight = 1;
	l->ship->life = DEFAULT_LIFE;
	l->ship->freight = calloc(l->num_all_freight,
			sizeof(*l->ship->freight));
	/* FIXME: Do something with it */
	l->ship->max_vx = 42;
	l->ship->max_vy = 72;
}
void cg_ship_set_engine(struct ship *ship, int eng)
{
	ship->engine = eng && ship->fuel > 0;
	sound_play_engine(ship->engine);
}
void cg_ship_step(struct ship* s, double dt)
{
	double ax = 0, ay = 0;
	if (!s->airport)
		cg_ship_rotate(s, s->rot_speed*dt);
	double drot = discrete_rot(s->rot)/24.0 * 2*M_PI;
	if (s->engine) {
		ax = cos(drot) * ENGINE_ACCEL;
		ay = sin(drot) * ENGINE_ACCEL;
		s->fuel -= FUEL_SPEED * dt;
		if (s->fuel < 0)
			s->engine = 0;
	}
	ax += -s->vx*AIR_RESISTANCE;
	ay += -s->vy*AIR_RESISTANCE;
	/* no gravity on airport to prevent multiple airport collision */
	if (!s->airport)
		ay += GRAVITY;
	/* taking off */
	if (s->airport && ay < 0) {
		/* cancel any pending cargo transfer */
		s->airport->sched_cargo_transfer = 0;
		s->airport = NULL;
	}
	if (s->airport)
		/* clear any speed caused by fans, magns, etc. */
		s->vx = s->vy = 0;
	s->vx += ax * dt;
	s->vy += ay * dt;
	s->x += s->vx * dt;
	s->y += s->vy * dt;
}
void cg_ship_kill(struct cgl *l)
{
	l->ship->dead = 1;
	l->kaboom_end = l->time + 1;
	sound_play_collision();
}
void cg_ship_rotate(struct ship *s, double delta)
{
	s->rot += delta;
	normalize_angle(&s->rot);
}
/* ==================== /Ship ==================== */

void cg_init(struct cgl *l)
{
	l->time = 0.0;
	l->ship = calloc(1, sizeof(*l->ship));
	cg_ship_init(l);
	l->kaboom_end = -DBL_MAX;
	l->status = Alive;
}

/* ==================== Collision detectors ==================== */
/* check if ship's center is inside the tile */
int cg_collision_rect_point(const struct tile *ship, const struct tile *tile)
{
	double x = ship->x + ship->w / 2,
	       y = ship->y + ship->h / 2;
	if (tile->x <= x && x <= tile->x + tile->w &&
	    tile->y <= y && y <= tile->y + tile->h)
		return 1;
	return 0;
}
/* check if tile t's bounding box collides with the ship within rectangle r,
 * knowing that r's origin in collision map is (img_x, img_y) */
int cg_collision_rect(const struct rect *r, int img_x, int img_y,
		__attribute__((unused)) const struct tile *t)
{
	for (unsigned j = 0; j < r->h; ++j)
		for (unsigned i = 0; i < r->w; ++i)
			if (cmap[img_y + j][img_x + i])
				return 1;
	return 0;
}
/* check if tile t collides with the ship within the rectangle r, knowing
 * that r's origin in collision map is (img_x, img_y) */
int cg_collision_bitmap(const struct rect *r, int img_x, int img_y,
		const struct tile *t)
{
	int tile_img_x = t->tex_x + (r->x - t->x),
	    tile_img_y = t->tex_y + (r->y - t->y);
	for (unsigned j = 0; j < r->h; ++j)
		for (unsigned i = 0; i < r->w; ++i)
			if (cmap[img_y + j][img_x + i] &&
			    cmap[tile_img_y + j][tile_img_x + i])
				return 1;
	return 0;
}
/* ==================== /Collision detectors ==================== */

void cg_handle_collisions(struct cgl *l)
{
	extern void cg_handle_collisions_block(struct cgl*, block);
	size_t x = max(0, l->ship->x / BLOCK_SIZE),
	       y = max(0, l->ship->y / BLOCK_SIZE);
	int end_x = min(l->ship->x + SHIP_W,
			l->width * BLOCK_SIZE),
	    end_y = min(l->ship->y + SHIP_H,
			l->height * BLOCK_SIZE);
	for (size_t j = y; (signed)j*BLOCK_SIZE < end_y; ++j)
		for (size_t i = x; (signed)i*BLOCK_SIZE < end_x; ++i)
			cg_handle_collisions_block(l, l->blocks[j][i]);
}
void cg_handle_collisions_block(struct cgl *l, block blk)
{
	extern void cg_call_collision_handler(struct cgl*, struct tile*);
	struct rect r;
	struct tile stile;
	ship_to_tile(l->ship, &stile);
	for (size_t i = 0; blk[i] != NULL; ++i) {
		if (!tiles_intersect(&stile, blk[i], &r))
			continue;
		int img_x = stile.tex_x + (r.x - stile.x),
		    img_y = stile.tex_y + (r.y - stile.y);
		int coll = 0;
		switch (blk[i]->collision_test) {
		case RectPoint:
			coll = cg_collision_rect_point(&stile, blk[i]);
			break;
		case Rect:
			coll = cg_collision_rect(&r, img_x, img_y, blk[i]);
			break;
		case Bitmap:
			coll = cg_collision_bitmap(&r, img_x, img_y, blk[i]);
			break;
		case Cannon:
			/* FIXME */
			coll = 1;
			break;
		case NoCollision:
			break;
		}
		if (coll)
			cg_call_collision_handler(l, blk[i]);
	}
}
void cg_call_collision_handler(struct cgl *l, struct tile *tile)
{
	extern int cg_handle_collision_gate(struct gate*),
	           cg_handle_collision_lgate(struct ship*, struct lgate*),
	           cg_handle_collision_airgen(struct airgen*),
		   cg_handle_collision_airport(struct ship*, struct airport*),
		   cg_handle_collision_fan(struct ship*, struct fan*),
		   cg_handle_collision_magnet(struct ship*, struct magnet*);
	int killed = 0;
	switch (tile->collision_type) {
	case GateAction:
		killed = cg_handle_collision_gate((struct gate*)tile->data);
		break;
	case LGateAction:
		killed = cg_handle_collision_lgate(l->ship, (struct lgate*)tile->data);
		break;
	case AirgenAction:
		killed = cg_handle_collision_airgen((struct airgen*)tile->data);
		break;
	case AirportAction:
		killed = cg_handle_collision_airport(l->ship, (struct airport*)tile->data);
		break;
	case FanAction:
		killed = cg_handle_collision_fan(l->ship, (struct fan*)tile->data);
		break;
	case MagnetAction:
		killed = cg_handle_collision_magnet(l->ship, (struct magnet*)tile->data);
		break;
	case Kaboom:
		killed = 1;
		break;
	}
	/* remember that collision detection handler may be called multiple
	 * times per step! */
	if (killed && !l->ship->dead)
		cg_ship_kill(l);
}

void cg_kaboom_step(struct cgl *l)
{
	/* FIXME: step kaboom */
}
void cg_objects_animate(struct cgl *l, double time)
{
	extern void animate_fan(struct fan*, double),
	            animate_magnet(struct magnet*, double),
	            animate_airgen(struct airgen*, double),
	            animate_bar(struct bar*, double),
	            animate_key(struct airport*, double);
	for (size_t i = 0; i < l->nmagnets; ++i)
		animate_magnet(&l->magnets[i], time);
	for (size_t i = 0; i < l->nfans; ++i)
		animate_fan(&l->fans[i], time);
	for (size_t i = 0; i < l->nairgens; ++i)
		animate_airgen(&l->airgens[i], time);
	for (size_t i = 0; i < l->nbars; ++i)
		animate_bar(&l->bars[i], time);
	for (size_t i = 0; i < l->nairports; ++i)
		animate_key(&l->airports[i], time);
}
/* perform logic simulation of all objects */
void cg_objects_step(struct cgl *l, double time, double dt)
{
	extern void cg_step_airgen(struct airgen*, struct ship*, double),
	            cg_step_bar(struct bar*, double, double),
	            cg_step_gate(struct gate*, double),
	            cg_step_lgate(struct lgate*, struct ship*, double),
		    cg_step_airport(struct airport*, struct ship*, double),
		    cg_step_fan(struct fan*, struct ship*, double),
		    cg_step_magnet(struct magnet*, struct ship*, double);
	for (size_t i = 0; i < l->nairgens; ++i)
		cg_step_airgen(&l->airgens[i], l->ship, dt);
	for (size_t i = 0; i < l->nbars; ++i)
		cg_step_bar(&l->bars[i], time, dt);
	for (size_t i = 0; i < l->ngates; ++i)
		cg_step_gate(&l->gates[i], dt);
	for (size_t i = 0; i < l->nlgates; ++i)
		cg_step_lgate(&l->lgates[i], l->ship, dt);
	for (size_t i = 0; i < l->nairports; ++i)
		cg_step_airport(&l->airports[i], l->ship, time);
	for (size_t i = 0; i < l->nfans; ++i)
		cg_step_fan(&l->fans[i], l->ship, dt);
	for (size_t i = 0; i < l->nmagnets; ++i)
		cg_step_magnet(&l->magnets[i], l->ship, dt);
}
void cg_step(struct cgl *l, double time)
{
	double dt = time - l->time;
	cg_objects_animate(l, time);
	cg_objects_step(l, time, dt);
	if (l->hb->num_cargo == l->num_all_freight) {
		l->status = Victory;
		goto end;
	}
	if (l->status == Lost)
		goto end;
	if (!l->ship->dead) {
		cg_ship_step(l->ship, dt);
		cg_handle_collisions(l);
	} else {
		if (l->kaboom_end > time) {
			cg_kaboom_step(l);
		} else {
			--l->ship->life;
			if (l->ship->life == -1)
				l->status = Lost;
			else
				cg_restart_ship(l);
		}
	}
end:
	l->time = time;
}

/* ==================== Collision handlers ==================== */
int cg_handle_collision_gate(struct gate *gate)
{
	gate->active = 1;
	return 0;
}
int cg_handle_collision_lgate(struct ship *ship, struct lgate *lgate)
{
	lgate->active = 1;
	lgate->open = 1;
	for (size_t i = 0; i < 4; ++i)
		if (lgate->keys[i] && !ship->keys[i])
			lgate->open = 0;
	return 0;
}
int cg_handle_collision_airgen(struct airgen *airgen)
{
	airgen->active = 1;
	return 0;
}
int cg_handle_collision_airport(struct ship *ship, struct airport *airport)
{
	struct tile allowed, stile;
	rect_to_tile(&airport->lbbox, &allowed);
	ship_to_tile(ship, &stile);
	if (discrete_rot(ship->rot) == ROT_UP &&
			cg_collision_rect_point(&stile, &allowed) &&
			abs(ship->vx) < ship->max_vx &&
			abs(ship->vy) < ship->max_vy)
			{
			airport->ship_touched = 1;
			sound_play_landing();
			}
	else
		return 1;
	return 0;
}
double field_modifier(enum dir dir, struct tile *act, struct ship *ship)
{
	int beg = 0;
	double modifier = 0;
	switch (dir) {
	case Up:    beg = act->y + act->h; break;
	case Down:  beg = act->y; break;
	case Left:  beg = act->x + act->w; break;
	case Right: beg = act->x; break;
	}
	switch (dir) {
	case Up:
	case Down:
		modifier = 1 - fabs(beg - (ship->y + SHIP_H/2)) / act->h;
		break;
	case Left:
	case Right:
		modifier = 1 - fabs(beg - (ship->x + SHIP_W/2)) / act->w;
		break;
	}
	return modifier;
}
int cg_handle_collision_fan(struct ship *ship, struct fan *fan)
{
	fan->modifier = field_modifier(fan->dir, fan->act, ship);
	return 0;
}
int cg_handle_collision_magnet(struct ship *ship, struct magnet *magnet)
{
	magnet->modifier = field_modifier(magnet->dir, magnet->act, ship);
	return 0;
}
/* ==================== /Collision handlers ==================== */

/* ==================== Object simulators ==================== */
/* auxilliary function used by all sliding tiles */
void update_sliding_tile(enum dir dir, struct tile *t, int len)
{
	switch (dir) {
	case Right:
		t->tex_x -= len - t->w;
		t->w = len;
		break;
	case Left:
		t->x -= len - t->w;
		t->w = len;
		break;
	case Down:
		t->tex_y -= len - t->h;
		t->h = len;
		break;
	case Up:
		t->y -= len - t->h;
		t->h = len;
		break;
	}
}

static const double bar_speeds[] = {5.65, 7.43, 10.83, 21.67, 43.33, 69.33};
static inline double bar_rand_speed(const struct bar *bar)
{
	return bar_speeds[rand_range(bar->min_s, bar->max_s)];
}
inline double bar_next_change(double time)
{
	return time + (rand()/(float)RAND_MAX + 0.5) *
		BAR_SPEED_CHANGE_INTERVAL;
}
void cg_step_bar(struct bar *bar, double time, double dt)
{
	if (bar->flen + bar->slen > bar->len) {
		bar->slen = bar->len - bar->flen;
		bar->fspeed = -bar_rand_speed(bar);
		bar->sspeed = -bar_rand_speed(bar);
	} else if (bar->flen <= BAR_MIN_LEN) {
		bar->fspeed = bar_rand_speed(bar);
	} else if (bar->gap_type == Constant && bar->slen <= BAR_MIN_LEN) {
		bar->fspeed = -bar_rand_speed(bar);
	} else if (bar->freq && bar->fnext_change <= time) {
		bar->fspeed = rand_sign() * bar_rand_speed(bar);
		bar->fnext_change = bar_next_change(time);
	}
	bar->flen += bar->fspeed * dt;
	bar->flen = fmin(bar->len, fmax(BAR_MIN_LEN, bar->flen));
	switch (bar->gap_type) {
	case Constant:
		bar->slen = bar->len - bar->flen - bar->gap;
		break;
	case Variable:
		if (bar->slen <= BAR_MIN_LEN) {
			bar->sspeed = bar_rand_speed(bar);
		} else if (bar->freq && bar->snext_change <= time) {
			bar->sspeed = rand_sign() * bar_rand_speed(bar);
			bar->snext_change = bar_next_change(time);
		}
		bar->slen += bar->sspeed * dt;
		break;
	}
	bar->slen = fmin(bar->len, fmax(BAR_MIN_LEN, bar->slen));
	switch (bar->orientation) {
	case Vertical:
		update_sliding_tile(Down, bar->fbar, (int)bar->flen);
		update_sliding_tile(Up, bar->sbar, (int)bar->slen);
		break;
	case Horizontal:
		update_sliding_tile(Right, bar->fbar, (int)bar->flen);
		update_sliding_tile(Left, bar->sbar, (int)bar->slen);
		break;
	}
}

void update_gate_bar(enum gate_type type, struct tile *bar, int len)
{
	switch (type) {
	case GateTop:
		update_sliding_tile(Down,  bar, len);
		break;
	case GateBottom:
		update_sliding_tile(Up,    bar, len);
		break;
	case GateLeft:
		update_sliding_tile(Right, bar, len);
		break;
	case GateRight:
		update_sliding_tile(Left,  bar, len);
		break;
	}
}

void cg_step_gate(struct gate *gate, double dt)
{
	if (!gate->active && gate->len < gate->max_len)
		gate->len = fmin(gate->max_len,
				gate->len + GATE_BAR_SPEED * dt);
	if (gate->active && gate->len > 0)
		gate->len = fmax(GATE_BAR_MIN_LEN,
				gate->len - GATE_BAR_SPEED * dt);
	update_gate_bar(gate->type, gate->bar, (int)gate->len);
	gate->active = 0;
}

void cg_step_lgate(struct lgate *lgate, struct ship *ship, double dt)
{
	for (size_t i = 0; i < 4; ++i) {
		if (!lgate->active) {
			lgate->light[i]->type = Transparent;
		} else {
			if (lgate->keys[i] && !ship->keys[i])
				lgate->light[i]->type = Blink;
			else if (lgate->keys[i])
				lgate->light[i]->type = Simple;
			else
				lgate->light[i]->type = Transparent;
		}
	}
	if (!lgate->open && lgate->len < lgate->max_len)
		lgate->len = fmin(lgate->max_len,
				lgate->len + GATE_BAR_SPEED * dt);
	if (lgate->open && lgate->len > 0)
		lgate->len = fmax(GATE_BAR_MIN_LEN,
				lgate->len - GATE_BAR_SPEED * dt);
	update_gate_bar(lgate->type, lgate->bar, (int)lgate->len);
	lgate->open = 0;
	lgate->active = 0;
}

void cg_step_airgen(struct airgen *airgen, struct ship *ship, double dt)
{
	if (!airgen->active)
		return;
	switch (airgen->spin) {
	case CW:
		cg_ship_rotate(ship, AIRGEN_ROT_SPEED * dt);
		break;
	case CCW:
		cg_ship_rotate(ship, -AIRGEN_ROT_SPEED * dt);
		break;
	}
	airgen->active = 0;
}

void cg_step_airport(struct airport *airport, struct ship *ship, double time)
{
	extern void airport_schedule_transfer(struct airport*, double),
	            airport_pop_cargo(struct airport*),
		    ship_load_freight(struct ship*, struct airport*),
		    ship_unload_freight(struct ship*, struct airport*);
	if (airport->sched_cargo_transfer && airport->transfer_time < time) {
		airport->sched_cargo_transfer = 0;
		switch (airport->type) {
		case Key:
			ship->keys[airport->c.key] = 1;
			airport_pop_cargo(airport);
			sound_play_key();
			break;
		case Extras:
			switch (airport->c.extras[airport->num_cargo - 1]) {
			case Turbo:
				ship->has_turbo = 1; break;
			case Cargo:
				++ship->max_freight; break;
			case Life:
				++ship->life; break;
			}
			sound_play_extra();
			airport_pop_cargo(airport);
			break;
		case Freight:
			ship_load_freight(ship, airport);
			sound_play_pickup();
			break;
		case Homebase:
			ship_unload_freight(ship, airport);
		    sound_play_dropitem();
			break;
		case Fuel:
			ship->fuel = min(MAX_FUEL, ship->fuel + FUEL_BARREL);
			airport_pop_cargo(airport);
			sound_play_fuel();
			break;
		}
	}
	if (!airport->ship_touched)
		return;
	ship->y = airport->base->y - 20;
	ship->vx = ship->vy = 0;
	ship->airport = airport;
	switch (airport->type) {
	case Freight:
		if (airport->num_cargo > 0 && ship->num_freight < ship->max_freight)
			airport_schedule_transfer(airport, time);
		break;
	case Extras:
	case Key:
		if (airport->num_cargo > 0)
			airport_schedule_transfer(airport, time);
		break;
	case Fuel:
		if (airport->num_cargo > 0 && ship->fuel <= MAX_FUEL - 1)
			airport_schedule_transfer(airport, time);
		break;
	case Homebase:
		if (ship->num_freight > 0)
			airport_schedule_transfer(airport, time);
		break;
	}
	airport->ship_touched = 0;
}

/* ==================== Cargo operations ==================== */
void airport_pop_cargo(struct airport *airport)
{
	--airport->num_cargo;
	airport->cargo[airport->num_cargo]->type = Transparent;
	airport->cargo[airport->num_cargo]->collision_test = NoCollision;
}
/* revert one pop */
void airport_push_cargo(struct airport *airport)
{
	airport->cargo[airport->num_cargo]->type = Simple;
	airport->cargo[airport->num_cargo]->collision_test = Rect;
	++airport->num_cargo;
}
void ship_load_freight(struct ship *ship, struct airport *airport)
{
	ship->freight[ship->num_freight++] =
		airport->c.freight[airport->num_cargo-1];
	airport_pop_cargo(airport);
}
void ship_unload_freight(struct ship *ship, struct airport *airport)
{
	airport->c.freight[airport->num_cargo++] =
		ship->freight[--ship->num_freight];
}
/* ==================== /Cargo operations ==================== */

void airport_schedule_transfer(struct airport *airport, double time)
{
	airport->sched_cargo_transfer = 1;
	airport->transfer_time = time + 1;
}
static const double fan_accel[] = {80, 40};
void cg_step_fan(struct fan *fan, struct ship *ship, double dt)
{
	if (fan->modifier == 0)
		return;
	double dv = fan_accel[fan->power] * fan->modifier * dt;
	switch (fan->dir) {
	case Down:
		ship->vy += dv; break;
	case Up:
		ship->vy -= dv; break;
	case Right:
		ship->vx += dv; break;
	case Left:
		ship->vx -= dv; break;
	}
	fan->modifier = 0;
}
static const double magn_accel = 50;
void cg_step_magnet(struct magnet *magnet, struct ship *ship, double dt)
{
	if (magnet->modifier == 0)
		return;
	double dv = magn_accel * magnet->modifier * dt;
	switch (magnet->dir) {
	case Down:
		ship->vy -= dv; break;
	case Up:
		ship->vy += dv; break;
	case Right:
		ship->vx -= dv; break;
	case Left:
		ship->vx += dv; break;
	}
	magnet->modifier = 0;
}
/* ==================== /Object simulators ==================== */

/* ==================== Object animators ==================== */
/* For static objects, whose animations do not influence the gameplay */
static const int magnet_anim_order[] = {0, 1, 2, 1};
static const int fan_anim_order[] = {0, 1, 2};
static const int airgen_anim_order[] = {0, 1, 2, 3, 4, 5, 6, 7};
static const int bar_anim_order[][2] = {{0, 1}, {1, 0}};
static const int key_anim_order[] = {0, 1, 2, 3, 4, 5, 6, 7};
void animate_fan(struct fan *fan, double time)
{
	int phase = round(time * FAN_ANIM_SPEED);
	int cur_tex = fan_anim_order[phase % 3];
	fan->base->tex_x = fan->tex_x + cur_tex * fan->base->w;
}
void animate_magnet(struct magnet *magnet, double time)
{
	int phase = round(time * MAGNET_ANIM_SPEED);
	int cur_tex = magnet_anim_order[phase % 4];
	magnet->magn->tex_x = magnet->tex_x + cur_tex * magnet->magn->w;
}
void animate_airgen(struct airgen *airgen, double time)
{
	int phase = round(time * AIRGEN_ANIM_SPEED);
	int cur_tex = airgen_anim_order[phase % 8];
	airgen->base->tex_x = airgen->tex_x + cur_tex * airgen->base->w;
}
void animate_bar(struct bar *bar, double time)
{
	int phase = round(time * BAR_ANIM_SPEED);
	int cur_tex = bar_anim_order[0][phase % 2];
	bar->beg->tex_x = bar->btex_x + cur_tex * BAR_TEX_OFFSET;
	cur_tex = bar_anim_order[1][phase % 2];
	bar->end->tex_x = bar->etex_x + cur_tex * BAR_TEX_OFFSET;
}
void animate_key(struct airport *airport, double time)
{
	if (airport->type != Key)
		return;
	int phase = round(time * KEY_ANIM_SPEED);
	int cur_tex = key_anim_order[phase % 8];
	airport->cargo[0]->tex_x = KEY_TEX_X + cur_tex * airport->cargo[0]->w;
}
/* ==================== /Object animators ==================== */

size_t cg_freight_remaining(const struct cgl *l)
{
	size_t nfreight = 0;
	for (size_t i = 0; i < l->nairports; ++i) {
		if (l->airports[i].type == Freight)
			nfreight += l->airports[i].num_cargo;
	}
	return nfreight;
}
void cg_get_freight_airports(const struct cgl *l, struct freight f[])
{
	size_t k = 0;
	for (size_t i = 0; i < l->nairports; ++i) {
		if (l->airports[i].type == Freight) {
			struct airport *a = &l->airports[i];
			for (size_t j = 0; j < a->num_cargo; ++j)
				f[k++] = a->c.freight[j];
		}
	}
}
