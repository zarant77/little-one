#ifndef LITTLE_ONE_SOUND_GENERATOR_H
#define LITTLE_ONE_SOUND_GENERATOR_H

#include "generated_sound.h"
#include "sound_definition.h"

enum {
    SOUND_GENERATOR_SAMPLE_RATE = 22050
};

int sound_generator_generate(const SoundDefinition* definition, GeneratedSound* sound);
void sound_generator_release(GeneratedSound* sound);

#endif
