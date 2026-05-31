#ifndef LITTLE_ONE_MUSIC_GENERATOR_H
#define LITTLE_ONE_MUSIC_GENERATOR_H

#include "generated_music.h"
#include "music_definition.h"

int music_generator_generate(const MusicDefinition* definition, GeneratedMusic* music);
void music_generator_release(GeneratedMusic* music);

#endif
