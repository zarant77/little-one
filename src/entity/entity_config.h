#ifndef LITTLE_ONE_ENTITY_CONFIG_H
#define LITTLE_ONE_ENTITY_CONFIG_H

#include <stdint.h>

#include "../sprites/generated_sprite.h"

typedef struct {
    int16_t x;
    int16_t y;
    int16_t radius;
} HurtZone;

typedef struct {
    int width;
    int height;
    uint32_t color;
    SpriteId sprite_id;
    const char* animationId;
} EntityVisualConfig;

static inline int32_t hurt_zone_world_x(
        int32_t entity_x,
        int32_t entity_width,
        const HurtZone* zone
) {
    int32_t center_x = entity_x + entity_width / 2;

    if (zone == 0) {
        return center_x;
    }

    return center_x + (int32_t)zone->x;
}

static inline int32_t hurt_zone_world_y(
        int32_t entity_y,
        int32_t entity_height,
        const HurtZone* zone
) {
    int32_t center_y = entity_y + entity_height / 2;

    if (zone == 0) {
        return center_y;
    }

    return center_y + (int32_t)zone->y;
}

static inline int hurt_zones_overlap(
        int32_t ax,
        int32_t ay,
        int32_t aw,
        int32_t ah,
        const HurtZone* a,
        int32_t bx,
        int32_t by,
        int32_t bw,
        int32_t bh,
        const HurtZone* b
) {
    int32_t acx;
    int32_t acy;
    int32_t bcx;
    int32_t bcy;
    int32_t dx;
    int32_t dy;
    int32_t radius_sum;

    if (a == 0 || b == 0 || a->radius < 0 || b->radius < 0) {
        return 0;
    }

    acx = hurt_zone_world_x(ax, aw, a);
    acy = hurt_zone_world_y(ay, ah, a);
    bcx = hurt_zone_world_x(bx, bw, b);
    bcy = hurt_zone_world_y(by, bh, b);
    dx = acx - bcx;
    dy = acy - bcy;
    radius_sum = (int32_t)a->radius + (int32_t)b->radius;

    return dx * dx + dy * dy <= radius_sum * radius_sum;
}

#endif
