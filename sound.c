/* sound.c - Implémentation des fonctions de gestion du son pour FreeCG
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

#include "sound.h"
#include <stdio.h>

// Définition des variables globales
SoundEffects sounds;
int sound_enabled = 1;

// Canaux réservés pour certains sons
#define ENGINE_CHANNEL 0
#define AMBIENT_CHANNEL 7

// Initialisation du système audio
int sound_init(void) {
    // Initialisation de la structure
    sounds.engine = NULL;
    sounds.landing = NULL;
    sounds.collision = NULL;
    sounds.pickup = NULL;
    sounds.dropitem = NULL;
    sounds.fuel = NULL;
    sounds.key = NULL;
    sounds.extra = NULL;
    sounds.ambient = NULL;

    // Initialisation de SDL_mixer avec SDL2
    // Modification ici : ajout d'un paramètre supplémentaire (fréquence d'échantillonnage en Hz)
    if (Mix_OpenAudioDevice(44100, MIX_DEFAULT_FORMAT, 2, 1024, NULL, SDL_AUDIO_ALLOW_ANY_CHANGE) == -1) {
    fprintf(stderr, "Erreur lors de l'initialisation de SDL_mixer : %s\n", Mix_GetError());
    sound_enabled = 0;
    return SOUND_ERROR_INIT;
}

    // Allocation de 16 canaux de mixage
    Mix_AllocateChannels(16);
    
    return SOUND_SUCCESS;
}

// Chargement des sons
int sound_load(void) {
    if (!sound_enabled) {
        return SOUND_SUCCESS; // En cas de désactivation du son, on considère que le chargement est réussi
    }

    // Chargement des effets sonores
    sounds.engine = Mix_LoadWAV("sounds/SOUND1.WAV");
    sounds.landing = Mix_LoadWAV("sounds/SOUND2.WAV");
    sounds.collision = Mix_LoadWAV("sounds/SOUND3.WAV");
    sounds.pickup = Mix_LoadWAV("sounds/SOUND4.WAV");
    sounds.dropitem = Mix_LoadWAV("sounds/SOUND5.WAV");
    sounds.fuel = Mix_LoadWAV("sounds/SOUND6.WAV");
    sounds.key = Mix_LoadWAV("sounds/SOUND7.WAV");
    sounds.extra = Mix_LoadWAV("sounds/SOUND8.WAV");

    // Vérification si les sons principaux ont été chargés
    if (!sounds.engine || !sounds.collision || !sounds.fuel) {
        fprintf(stderr, "Erreur lors du chargement des sons : %s\n", Mix_GetError());
        // On continue même en cas d'erreur, mais on retourne un code d'erreur
        return SOUND_ERROR_LOAD;
    }

    // Si le son d'ambiance est disponible, le jouer en boucle
    //if (sounds.ambient) {
    //    Mix_PlayChannel(AMBIENT_CHANNEL, sounds.ambient, -1);
    //    // Réduire le volume du son d'ambiance
    //    Mix_Volume(AMBIENT_CHANNEL, MIX_MAX_VOLUME / 4);
    //}

    return SOUND_SUCCESS;
}

// Libération des ressources sonores
void sound_free(void) {
    // Arrêter tous les canaux audio
    Mix_HaltChannel(-1);

    // Libérer tous les effets sonores
    if (sounds.engine) Mix_FreeChunk(sounds.engine);
    if (sounds.landing) Mix_FreeChunk(sounds.landing);
    if (sounds.collision) Mix_FreeChunk(sounds.collision);
    if (sounds.pickup) Mix_FreeChunk(sounds.pickup);
    if (sounds.dropitem) Mix_FreeChunk(sounds.dropitem);
    if (sounds.fuel) Mix_FreeChunk(sounds.fuel);
    if (sounds.key) Mix_FreeChunk(sounds.key);
    if (sounds.extra) Mix_FreeChunk(sounds.extra);

    // Réinitialiser les pointeurs
    sounds.engine = NULL;
    sounds.landing = NULL;
    sounds.collision = NULL;
    sounds.pickup = NULL;
    sounds.dropitem = NULL;
    sounds.fuel = NULL;
    sounds.key = NULL;
    sounds.extra = NULL;

    // Fermer SDL_mixer
    Mix_CloseAudio();
}

// Joue un son sur un canal spécifique
void sound_play(Mix_Chunk *sound, int channel, int loops) {
    if (!sound_enabled || !sound) {
        return;
    }
    
    Mix_PlayChannel(channel, sound, loops);
}

// Arrête un son sur un canal spécifique
void sound_stop(int channel) {
    if (!sound_enabled) {
        return;
    }
    
    Mix_HaltChannel(channel);
}

// Fonctions spécifiques pour les sons du jeu

// Active ou désactive le son du moteur
void sound_play_engine(int play) {
    static int current_state = 0;
    
    if (!sound_enabled || current_state == play) {
        return;  // Ne rien faire si l'état n'a pas changé
    }
    
    current_state = play;
    
    if (play) {
        // Vérifier si le son est déjà en cours de lecture sur ce canal
        if (!Mix_Playing(ENGINE_CHANNEL)) {
            Mix_PlayChannel(ENGINE_CHANNEL, sounds.engine, -1);
        }
    } else {
        Mix_HaltChannel(ENGINE_CHANNEL);
    }
}

void sound_play_landing(void) {
    sound_play(sounds.landing, 1, 0);
}

void sound_play_collision(void) {
    sound_play(sounds.collision, 2, 0);
}

void sound_play_pickup(void) {
    sound_play(sounds.pickup, 3, 0);
}

void sound_play_dropitem(void) {
    sound_play(sounds.dropitem, 4, 0);
}

void sound_play_fuel(void) {
    sound_play(sounds.fuel, 5, 0);
}

void sound_play_key(void) {
    sound_play(sounds.key, 6, 0);
}

void sound_play_extra(void) {
    sound_play(sounds.extra, 7, 0);
}