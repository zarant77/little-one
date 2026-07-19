#include "threat_config.h"

#define VISUAL(w, h, c, sprite, movement, death, attack) \
    { .width=(w), .height=(h), .color=(c), .sprite_id=(sprite), \
      .animationId=(movement), .deathAnimationId=(death), .attackAnimationId=(attack) }

static const ThreatConfig THREAT_CONFIGS[] = {
    { "threat.boar", THREAT_GROUND_ENEMY, 1, 2, 600.0f, 0, 0,
      { 0, 16, 54 }, VISUAL(200, 150, 0xc06e00ff, SPRITE_BOAR, "walk", "boar_death_tumble", "enemy_attack") },
    { "threat.ork", THREAT_GROUND_ENEMY, 1, 3, 300.0f, 0, 0,
      { 0, 4, 68 }, VISUAL(250, 250, 0x00aa00ff, SPRITE_ORK, "walk", "ork_death_knockback", "enemy_attack") },
    { "threat.rat", THREAT_GROUND_ENEMY, 1, 3, 1500.0f, 0, 0,
      { 0, 10, 26 }, VISUAL(200, 100, 0x666666ff, SPRITE_RAT, "walk", "rat_death_flip", "enemy_attack") },
    { "threat.bird", THREAT_FLYING_ENEMY, 1, 5, 1200.0f, -400, -100,
      { 0, 0, 48 }, VISUAL(220, 95, 0x3f7fd6ff, SPRITE_BIRD, "fly", "bird_death_camera_hit", "flying_attack"),
      { THREAT_TRAJECTORY_SMOOTH_CURVE, 72, 1650, 0, 0 } },
    { "threat.bat", THREAT_FLYING_ENEMY, 1, 8, 1500.0f, -400, -100,
      { 0, 0, 48 }, VISUAL(200, 75, 0x3f7fd6ff, SPRITE_BAT, "bat_flight_twitch", "bat_death_spin_drop", "flying_attack"),
      { THREAT_TRAJECTORY_JAGGED_ZIGZAG, 105, 520, 24, 85 } },
    { "threat.stump", THREAT_STATIC_OBSTACLE, 1, 0, 0, 0, 0,
      { 0, 0, 100 }, VISUAL(250, 250, 0xff0000ff, SPRITE_STUMP, "predator_idle", "obstacle_break", "obstacle_attack") },
    { "threat.rock", THREAT_STATIC_OBSTACLE, 1, 0, 0, 0, 0,
      { 0, 0, 100 }, VISUAL(250, 250, 0xe500d2ff, SPRITE_ROCK, "predator_snap_idle", "obstacle_break", "obstacle_attack") },
    { "threat.cactus", THREAT_STATIC_OBSTACLE, 1, 0, 0, 0, 0,
      { 0, -40, 60 }, VISUAL(160, 300, 0xe500d2ff, SPRITE_CACTUS, "predator_sway", "obstacle_break", "obstacle_attack") },
};

static const int THREAT_CONFIG_COUNT = sizeof(THREAT_CONFIGS) / sizeof(THREAT_CONFIGS[0]);

const ThreatConfig* threat_config_get_all(int* count) {
    if (count != 0) *count = THREAT_CONFIG_COUNT;
    return THREAT_CONFIGS;
}

const ThreatConfig* threat_config_get(int index) {
    return index >= 0 && index < THREAT_CONFIG_COUNT ? THREAT_CONFIGS + index : 0;
}
