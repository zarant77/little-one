#include "generated_sprite.h"

#include "sprite_registry.h"

static uint32_t PLAYER_PIXELS[128 * 128];
static uint32_t BOAR_PIXELS[160 * 160];
static uint32_t ORK_PIXELS[200 * 200];
static uint32_t RAT_PIXELS[128 * 64];
static uint32_t ROCK_PIXELS[150 * 150];
static uint32_t STUMP_PIXELS[80 * 180];

static GeneratedSprite GENERATED_SPRITES[SPRITE_ID_COUNT] = {
        { 0, 0, 0, 0, 0, PLAYER_PIXELS },
        { 0, 0, 0, 0, 0, BOAR_PIXELS },
        { 0, 0, 0, 0, 0, ORK_PIXELS },
        { 0, 0, 0, 0, 0, RAT_PIXELS },
        { 0, 0, 0, 0, 0, ROCK_PIXELS },
        { 0, 0, 0, 0, 0, STUMP_PIXELS },
};

static int generated_sprites_initialized = 0;

static int sprite_alpha(uint32_t color)
{
    return (int)(color & 0xff);
}

static int sprite_id_equals(const char* left, const char* right)
{
    int index = 0;

    if (left == 0 || right == 0)
    {
        return 0;
    }

    while (left[index] != 0 && right[index] != 0)
    {
        if (left[index] != right[index])
        {
            return 0;
        }
        index += 1;
    }

    return left[index] == right[index];
}

static void sprite_clear(uint32_t* pixels, int width, int height)
{
    int pixel_count = width * height;

    for (int pixel_index = 0; pixel_index < pixel_count; ++pixel_index)
    {
        pixels[pixel_index] = 0x00000000;
    }
}

static void sprite_put_pixel(uint32_t* pixels, int width, int height, int x, int y, uint32_t color)
{
    if (sprite_alpha(color) == 0)
    {
        return;
    }

    if (x < 0 || y < 0 || x >= width || y >= height)
    {
        return;
    }

    pixels[y * width + x] = color;
}

static void sprite_draw_rect(
        uint32_t* pixels,
        int sprite_width,
        int sprite_height,
        int center_x,
        int center_y,
        int width,
        int height,
        uint32_t color
)
{
    int left = center_x - width / 2;
    int top = center_y - height / 2;
    int right = left + width;
    int bottom = top + height;

    for (int y = top; y < bottom; ++y)
    {
        for (int x = left; x < right; ++x)
        {
            sprite_put_pixel(pixels, sprite_width, sprite_height, x, y, color);
        }
    }
}

static void sprite_draw_circle(
        uint32_t* pixels,
        int sprite_width,
        int sprite_height,
        int center_x,
        int center_y,
        int radius,
        uint32_t color
)
{
    int min_x = center_x - radius;
    int max_x = center_x + radius;
    int min_y = center_y - radius;
    int max_y = center_y + radius;
    int radius_squared = radius * radius;

    for (int y = min_y; y <= max_y; ++y)
    {
        for (int x = min_x; x <= max_x; ++x)
        {
            int dx = x - center_x;
            int dy = y - center_y;

            if (dx * dx + dy * dy <= radius_squared)
            {
                sprite_put_pixel(pixels, sprite_width, sprite_height, x, y, color);
            }
        }
    }
}

static int sprite_edge(int ax, int ay, int bx, int by, int px, int py)
{
    return (px - ax) * (by - ay) - (py - ay) * (bx - ax);
}

static void sprite_draw_triangle(
        uint32_t* pixels,
        int sprite_width,
        int sprite_height,
        int center_x,
        int center_y,
        int width,
        int height,
        uint32_t color
)
{
    int x1 = center_x;
    int y1 = center_y - height / 2;
    int x2 = center_x - width / 2;
    int y2 = center_y + height / 2;
    int x3 = center_x + width / 2;
    int y3 = center_y + height / 2;
    int min_x = x1;
    int max_x = x1;
    int min_y = y1;
    int max_y = y1;

    if (x2 < min_x) { min_x = x2; }
    if (x3 < min_x) { min_x = x3; }
    if (x2 > max_x) { max_x = x2; }
    if (x3 > max_x) { max_x = x3; }
    if (y2 < min_y) { min_y = y2; }
    if (y3 < min_y) { min_y = y3; }
    if (y2 > max_y) { max_y = y2; }
    if (y3 > max_y) { max_y = y3; }

    for (int y = min_y; y <= max_y; ++y)
    {
        for (int x = min_x; x <= max_x; ++x)
        {
            int edge1 = sprite_edge(x1, y1, x2, y2, x, y);
            int edge2 = sprite_edge(x2, y2, x3, y3, x, y);
            int edge3 = sprite_edge(x3, y3, x1, y1, x, y);

            if ((edge1 >= 0 && edge2 >= 0 && edge3 >= 0)
                    || (edge1 <= 0 && edge2 <= 0 && edge3 <= 0))
            {
                sprite_put_pixel(pixels, sprite_width, sprite_height, x, y, color);
            }
        }
    }
}

static void sprite_generate(const SpriteDefinition* definition, GeneratedSprite* sprite)
{
    sprite_clear(sprite->pixels, sprite->width, sprite->height);

    for (int command_index = 0; command_index < definition->command_count; ++command_index)
    {
        const SpriteCommand* command = definition->commands + command_index;
        int rotation = (int)command->rotation;

        (void)rotation;

        if (command->kind == SPRITE_CMD_RECT)
        {
            sprite_draw_rect(
                    sprite->pixels,
                    sprite->width,
                    sprite->height,
                    command->x,
                    command->y,
                    command->w,
                    command->h,
                    command->color
            );
        }
        else if (command->kind == SPRITE_CMD_CIRCLE)
        {
            sprite_draw_circle(
                    sprite->pixels,
                    sprite->width,
                    sprite->height,
                    command->x,
                    command->y,
                    command->w,
                    command->color
            );
        }
        else if (command->kind == SPRITE_CMD_TRIANGLE)
        {
            sprite_draw_triangle(
                    sprite->pixels,
                    sprite->width,
                    sprite->height,
                    command->x,
                    command->y,
                    command->w,
                    command->h,
                    command->color
            );
        }
    }
}

void generated_sprite_initialize_all(void)
{
    if (generated_sprites_initialized)
    {
        return;
    }

    for (size_t sprite_index = 0; sprite_index < SPRITE_COUNT && sprite_index < SPRITE_ID_COUNT; ++sprite_index)
    {
        const SpriteDefinition* definition = SPRITE_DEFINITIONS[sprite_index];
        GeneratedSprite* sprite = GENERATED_SPRITES + sprite_index;

        sprite->id = definition->id;
        sprite->width = definition->width;
        sprite->height = definition->height;
        sprite->pivot_x = definition->pivot_x;
        sprite->pivot_y = definition->pivot_y;

        sprite_generate(definition, sprite);
    }

    generated_sprites_initialized = 1;
}

const GeneratedSprite* generated_sprite_get(SpriteId sprite_id)
{
    if (sprite_id < 0 || sprite_id >= SPRITE_ID_COUNT)
    {
        return 0;
    }

    return GENERATED_SPRITES + sprite_id;
}

const GeneratedSprite* generated_sprite_get_by_id(const char* sprite_id)
{
    for (size_t sprite_index = 0; sprite_index < SPRITE_COUNT && sprite_index < SPRITE_ID_COUNT; ++sprite_index)
    {
        if (sprite_id_equals(SPRITE_DEFINITIONS[sprite_index]->id, sprite_id))
        {
            return GENERATED_SPRITES + sprite_index;
        }
    }

    return 0;
}
