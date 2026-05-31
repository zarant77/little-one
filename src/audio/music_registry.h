#ifndef LITTLE_ONE_MUSIC_REGISTRY_H
#define LITTLE_ONE_MUSIC_REGISTRY_H

#include <stddef.h>

#include "generated_music.h"
#include "music_definition.h"

extern const MusicDefinition MAIN_THEME_MUSIC;

extern const MusicDefinition* MUSIC_DEFINITIONS[];
extern const size_t MUSIC_COUNT;

void music_registry_initialize_all(void);
void music_registry_shutdown_all(void);

const GeneratedMusic* music_registry_get(MusicId music_id);
const GeneratedMusic* music_registry_get_by_id(const char* music_id);

#endif
