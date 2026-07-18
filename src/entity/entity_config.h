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
    int16_t x;
    int16_t y;
    int16_t width;
    int16_t height;
} CollisionBoundary;

typedef struct {
    int width;
    int height;
    uint32_t color;
    SpriteId sprite_id;
    const char* animationId;
    const char* deathAnimationId;
    const char* attackAnimationId;
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

static inline int32_t clamp_i32(int32_t value, int32_t min, int32_t max) {
    if (value < min) {
        return min;
    }

    if (value > max) {
        return max;
    }

    return value;
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

static inline int rect_overlaps_hurt_zone(
        int32_t rect_entity_x,
        int32_t rect_entity_y,
        const CollisionBoundary* rect,
        int32_t circle_entity_x,
        int32_t circle_entity_y,
        int32_t circle_entity_width,
        int32_t circle_entity_height,
        const HurtZone* circle
) {
    int32_t rect_left;
    int32_t rect_top;
    int32_t rect_right;
    int32_t rect_bottom;
    int32_t circle_x;
    int32_t circle_y;
    int32_t closest_x;
    int32_t closest_y;
    int32_t dx;
    int32_t dy;
    int32_t radius;

    if (rect == 0 || circle == 0 || rect->width < 0 || rect->height < 0 || circle->radius < 0) {
        return 0;
    }

    rect_left = rect_entity_x + (int32_t)rect->x;
    rect_top = rect_entity_y + (int32_t)rect->y;
    rect_right = rect_left + (int32_t)rect->width;
    rect_bottom = rect_top + (int32_t)rect->height;

    circle_x = hurt_zone_world_x(circle_entity_x, circle_entity_width, circle);
    circle_y = hurt_zone_world_y(circle_entity_y, circle_entity_height, circle);

    closest_x = clamp_i32(circle_x, rect_left, rect_right);
    closest_y = clamp_i32(circle_y, rect_top, rect_bottom);

    dx = circle_x - closest_x;
    dy = circle_y - closest_y;
    radius = (int32_t)circle->radius;

    return dx * dx + dy * dy <= radius * radius;
}

#endif
