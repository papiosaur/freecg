/* sound.h - Gestion des sons pour FreeCG
 * Copyright (C) 2025
 *
 * Ce fichier fait partie de FreeCG.
 *
 * FreeCG est un logiciel libre ; vous pouvez le redistribuer et/ou le modifier
 * selon les termes de la Licence Publique Générale GNU telle que publiée par
 * la Free Software Foundation ; soit la version 3 de la Licence, soit
 * (à votre convenance) une version ultérieure.
 *
 * FreeCG est distribué dans l'espoir qu'il sera utile,
 * mais SANS AUCUNE GARANTIE ; sans même la garantie implicite de
 * COMMERCIALISATION ou D'ADAPTATION À UN USAGE PARTICULIER. Voir la
 * Licence Publique Générale GNU pour plus de détails.
 *
 * Vous devriez avoir reçu une copie de la Licence Publique Générale GNU
 * avec FreeCG. Si ce n'est pas le cas, consultez <http://www.gnu.org/licenses/>.
 */

#ifndef SOUND_H
#define SOUND_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>

// Codes d'erreur spécifiques aux sons
enum sound_error_codes {
    SOUND_SUCCESS = 0,
    SOUND_ERROR_INIT = -1,
    SOUND_ERROR_LOAD = -2
};

// Structure pour les effets sonores
typedef struct {
    Mix_Chunk *engine;     // Son du moteur
    Mix_Chunk *collision;  // Son de collision
	Mix_Chunk *dropitem;   // Son de dépose item
    Mix_Chunk *fuel;       // Son de ramassage d'objets
    Mix_Chunk *landing;    // Son d'atterrissage
    Mix_Chunk *pickup;     // Son de victoire
    Mix_Chunk *key;        // Son de ramassage clé
    Mix_Chunk *extra;      // Son pour les extras
	Mix_Chunk *ambient;    // Son d'ambiance (optionnel)
} SoundEffects;

// Variables externes
extern SoundEffects sounds;
extern int sound_enabled;

// Initialisation du système audio
int sound_init(void);

// Chargement des sons
int sound_load(void);

// Libération des ressources sonores
void sound_free(void);

// Joue un son sur un canal spécifique
// channel: numéro du canal (-1 pour auto-assignation)
// loops: nombre de répétitions (-1 pour répéter indéfiniment)
void sound_play(Mix_Chunk *sound, int channel, int loops);

// Arrête un son sur un canal spécifique
void sound_stop(int channel);

// Fonctions spécifiques pour les sons du jeu
void sound_play_engine(int play);
void sound_play_collision(void);
void sound_play_dropitem(void);
void sound_play_fuel(void);
void sound_play_landing(void);
void sound_play_pickup(void);
void sound_play_key(void);
void sound_play_extra(void);
void sound_play_ambient(void);

#endif /* SOUND_H */