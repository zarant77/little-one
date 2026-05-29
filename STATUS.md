# Little One — Status

## Project

**Little One** is a tiny native Android game written in pure C.

Inspired by:

* .kkrieger
* old demoscene productions
* tiny executables
* procedural content generation
* small games that do a lot with almost nothing

Tagline:

> The world is dangerous.
> But you are the .wwarrior.
> The Little One.

---

# Current Version

**v0.0.9 — Solid Ground**

---

# Current Milestone

## v0.1.0 — Procedural Graphics

Goal:

Replace plain colored rectangles with generated primitive sprites.

No PNG files.

No runtime parsing.

No external assets.

All sprite data is compiled directly into the executable as tiny C arrays.

---

# Technology

## Platform

* Android NativeActivity
* Pure C
* Android NDK
* Software rendering
* No SDL
* No OpenGL
* No Vulkan
* No Java gameplay code
* No Kotlin gameplay code

## Build

* Gradle
* CMake
* NDK

APK size is currently extremely small.

---

# Implemented

## Platform

* NativeActivity startup
* Native game loop
* Window lifecycle handling
* Input queue handling
* Stable 60 FPS cap
* FPS diagnostics
* Rotation handling improvements
* Landscape mode

## Rendering

Implemented:

* Software framebuffer renderer
* Rectangle rendering
* Configurable colors
* FPS display
* Score display
* 7-segment score rendering

Planned / in progress:

* Circle rendering
* Triangle rendering
* Generated sprite renderer
* Primitive sprite registry
* Sprite selection by sprite id

## Input

Touch controls:

### Left side

Movement control:

* drag left → move left
* drag right → move right

### Right side

Action button:

* tap on ground → jump
* tap in air → smash

Multi-touch supported.

---

# Gameplay

## Player

Implemented:

* movement
* gravity
* jump
* smash attack
* landing

Config-driven.

## World

Implemented:

* scrolling world
* configurable scroll speed
* configurable ground height

## Enemies

Implemented:

* enemy configs
* random enemy spawning
* enemy collisions
* enemy smash kills
* enemy score values
* random enemy selection from config array

Current enemies:

* boar
* ork
* rat

## Obstacles

Implemented:

* obstacle configs
* random obstacle spawning
* obstacle collisions
* random obstacle selection from config array

Current obstacles:

* rock
* stump

## Score

Implemented:

* score counter
* best score
* 7-segment rendering

---

# Configuration

Implemented:

## game_settings

Contains:

* target FPS
* uncapped FPS flag
* debug flags
* ground position
* world scroll speed

## player_config

Contains:

* hp
* move speed
* jump velocity
* smash velocity
* size
* visual settings

## enemy_config

Contains:

* hp
* score value
* move speed
* spawn height range
* size
* visual settings

Planned:

* sprite_id

## obstacle_config

Contains:

* size
* visual settings

Planned:

* sprite_id

---

# Current State

The core endless runner gameplay works.

Player can:

* move
* jump
* smash enemies
* avoid obstacles
* score points
* die
* restart

The game is already playable.

Current rendering is still mostly primitive / placeholder-based, but the next step is to replace entity rectangles with generated sprites.

---

# Procedural Graphics Protocol

## Draw Command

A draw command is stored as a flat array of integer values.

Command format:

```text
[kind, x, y, w, h, rotation, color]
```

Command size:

```c
#define DRAW_COMMAND_SIZE 7
```

Command fields:

```text
kind     — primitive type
x        — local pivot x inside sprite
y        — local pivot y inside sprite
w        — primitive width / radius
h        — primitive height
rotation — rotation in degrees
color    — RGBA color
```

For all primitives:

```text
x/y = geometric center / pivot
```

## Primitive Kinds

```c
typedef enum {
    DRAW_RECT = 0,
    DRAW_CIRCLE = 1,
    DRAW_TRIANGLE = 2
} DrawKind;
```

## Rect

```text
[DRAW_RECT, cx, cy, w, h, rotation, color]
```

Meaning:

* `cx/cy` — geometric center
* `w` — width
* `h` — height
* `rotation` — rotation around center

## Circle

```text
[DRAW_CIRCLE, cx, cy, radius, 0, 0, color]
```

Meaning:

* `cx/cy` — circle center
* `w` — radius
* `h` — unused, should be `0`
* `rotation` — ignored / reserved

## Triangle

```text
[DRAW_TRIANGLE, cx, cy, base_width, height, rotation, color]
```

Meaning:

* triangle is isosceles
* `cx/cy` — geometric center
* `w` — base width
* `h` — height
* `rotation` — rotation around center

For `rotation = 0`, triangle points upward.

Triangle points before rotation:

```c
x1 = cx - w / 2;
y1 = cy + h / 3;

x2 = cx + w / 2;
y2 = cy + h / 3;

x3 = cx;
y3 = cy - (2 * h) / 3;
```

---

# Generated Sprite

A generated sprite is a list of draw commands.

```c
typedef struct {
    const uint32_t* data;
    int command_count;
    int width;
    int height;
} GeneratedSprite;
```

Example:

```c
static const uint32_t PLAYER_SPRITE_DATA[] = {
    DRAW_TRIANGLE, 10,  9, 10, 14,   0, 0xffd36bff,
    DRAW_TRIANGLE, 22,  9, 10, 14,   0, 0xffd36bff,

    DRAW_CIRCLE,   16, 17, 12,  0,   0, 0xffd36bff,
    DRAW_CIRCLE,   11, 15,  2,  0,   0, 0x111111ff,
    DRAW_CIRCLE,   21, 15,  2,  0,   0, 0x111111ff,

    DRAW_RECT,     16, 32, 14, 16,   0, 0xff8a3dff,
};

static const GeneratedSprite PLAYER_SPRITE = {
    .data = PLAYER_SPRITE_DATA,
    .command_count = 6,
    .width = 32,
    .height = 40
};
```

---

# Sprite Renderer

Planned renderer API:

```c
void renderer_draw_generated_sprite(
    Framebuffer* framebuffer,
    const GeneratedSprite* sprite,
    int x,
    int y
);
```

Where:

```text
x/y = top-left screen position of the sprite
command x/y = local geometric center inside the sprite
```

Final primitive position:

```c
final_x = x + command_x;
final_y = y + command_y;
```

Initial implementation may support only `rotation = 0` visually, but the protocol must keep the rotation field.

---

# Sprite Registry

Planned sprite id system:

```c
typedef enum {
    SPRITE_PLAYER = 0,
    SPRITE_BOAR,
    SPRITE_ORK,
    SPRITE_RAT,
    SPRITE_ROCK,
    SPRITE_STUMP
} SpriteId;
```

Planned lookup:

```c
const GeneratedSprite* get_generated_sprite(SpriteId sprite_id);
```

Runtime should use enum ids to stay tiny.

Human-readable ids like `"stump"` may be used later by CatPaint or export tools, but the game should compile them into enum values.

---

# Required Primitive Sprites

## Player

Main hero sprite:

* round head
* simple body
* eyes
* small triangular ears or details

## Enemies

### Boar

Primitive silhouette:

* horizontal body
* round snout
* small legs
* tusk or triangle detail
* dark eye

### Ork

Primitive silhouette:

* blocky body
* round or rectangular head
* small triangle ears or horns
* dark eyes
* green-ish body color

### Rat

Primitive silhouette:

* small long body
* round head
* tiny ears
* thin tail made from small rects or triangles
* dark eye

## Obstacles

### Rock

Primitive obstacle:

* irregular-looking rock made from overlapping rects, circles, and triangles
* should read as a rock despite simple geometry

### Stump

Primitive obstacle:

* vertical stump body
* top cap approximation
* rings or darker marks
* optional roots using small triangles or rects

---

# CatPaint Subproject

CatPaint is planned as a small web-based editor for Little One generated sprites.

Purpose:

* draw sprites from primitive shapes
* edit rects, circles, and triangles visually
* use the same command protocol as Little One
* export sprite data into C arrays
* allow human-readable sprite ids like `"player"`, `"boar"`, `"ork"`, `"rat"`, `"rock"`, `"stump"`

CatPaint may store editable project data in JSON or another convenient web format, but Little One runtime should not parse JSON.

Runtime output must remain static C data.

Example CatPaint export target:

```c
static const uint32_t STUMP_SPRITE_DATA[] = {
    DRAW_RECT,   16, 24, 16, 24, 0, 0x8a4a2fff,
    DRAW_CIRCLE, 16, 12, 10,  0, 0, 0xa85f3aff,
    DRAW_CIRCLE, 16, 12,  5,  0, 0, 0x5a2f1aff,
};
```

---

# Planned Roadmap

## v0.1.0

Procedural graphics.

Add:

* draw command protocol
* generated sprite renderer
* rect / circle / triangle primitive support
* generated player sprite
* generated sprites for boar, ork, rat, rock, stump
* sprite registry
* sprite id in entity configs

## v0.1.1

CatPaint export workflow.

Add:

* web editor for primitive sprites
* visual editing of draw commands
* export to C arrays
* optional preview scaling
* sprite id management

## v0.1.2

Animations.

Possible:

* squash & stretch
* walk animation
* jump animation
* smash animation
* frame-based generated sprites

## v0.1.3

Feedback.

Add:

* screen shake
* vibration
* hit stop
* particles

## v0.1.4

Audio.

Add:

* jump sounds
* smash sounds
* death sounds

## v0.1.5

Save system.

Store:

* best score
* unlocked cats
* settings

## v0.2.0

Looks like a real game.

Procedural sprites, animation, feedback, audio, progression.

---

# Long-Term Goal

Create a fun endless runner that:

* fits in a tiny APK
* uses procedural graphics
* uses procedural animation
* minimizes external assets
* stays simple
* feels good to play

The world is dangerous.

But you are the .wwarrior.

The Little One.
