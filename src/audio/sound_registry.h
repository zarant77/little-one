#ifndef LITTLE_ONE_SOUND_REGISTRY_H
#define LITTLE_ONE_SOUND_REGISTRY_H

#include <stddef.h>

#include "generated_sound.h"
#include "sound_definition.h"

extern const SoundDefinition JUMP_SOUND;
extern const SoundDefinition SMASH_SOUND;
extern const SoundDefinition HIT_SOUND;
extern const SoundDefinition DEATH_SOUND;

extern const SoundDefinition* SOUND_DEFINITIONS[];
extern const size_t SOUND_COUNT;

void sound_registry_initialize_all(void);
void sound_registry_shutdown_all(void);

const GeneratedSound* sound_registry_get(SoundId sound_id);
const GeneratedSound* sound_registry_get_by_id(const char* sound_id);

#endif
