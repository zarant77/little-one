#!/usr/bin/env python3

import json
import re
from pathlib import Path
from typing import Any

def find_project_root() -> Path:
    script_path = Path(__file__).resolve()
    candidates = [
        Path.cwd().resolve(),
        script_path.parent,
        *script_path.parents,
    ]

    for candidate in candidates:
        if (candidate / "src").is_dir():
            return candidate

    return script_path.parents[1]


PROJECT_ROOT = find_project_root()
ASSETS_DIR = PROJECT_ROOT / "src" / "assets"
SPRITE_ASSETS_DIR = ASSETS_DIR / "sprites"
DEFINITIONS_DIR = PROJECT_ROOT / "src" / "sprites" / "definitions"

SPRITE_MAX_VALUE = 1024
COMMAND_MAX_VALUE = 1023

KIND_ID = {
    "rect": 0,
    "circle": 1,
    "triangle": 2,
}


def fail(message: str) -> None:
    raise RuntimeError(message)


def warn(message: str) -> None:
    print(f"[WARNING] {message}")


def require_int(name: str, value: Any, min_value: int, max_value: int) -> None:
    if not isinstance(value, int) or value < min_value or value > max_value:
        fail(f"{name} must be an integer in range {min_value}..{max_value}, got {value}")


def require_number(name: str, value: Any) -> float:
    if not isinstance(value, (int, float)):
        fail(f"{name} must be a number, got {value}")

    return float(value)


def require_color(name: str, value: Any) -> str:
    if not isinstance(value, str):
        fail(f"{name} must be a string")

    color = value.lower()

    if not re.fullmatch(r"[0-9a-f]{8}", color):
        fail(f"{name} must be rrggbbaa hex, got {value}")

    return color


def pack_command_value(name: str, value: int) -> tuple[int, int]:
    if value < 0 or value > COMMAND_MAX_VALUE:
        fail(f"{name}={value} is out of range 0..{COMMAND_MAX_VALUE}")

    byte_value = value & 0xFF
    bank = value >> 8

    return byte_value, bank


def pack_rotation(degrees: float) -> int:
    normalized = degrees % 360.0
    return int(round((normalized * 255.0) / 360.0)) & 255


def symbol_name(sprite_id: str) -> str:
    return re.sub(r"[^a-zA-Z0-9_]", "_", sprite_id).upper()


def load_sprite(file_path: Path) -> dict[str, Any]:
    data = json.loads(file_path.read_text(encoding="utf-8"))

    if not isinstance(data, dict):
        fail(f"{file_path}: root must be an object")

    primitives = data.get("primitives")

    if not isinstance(primitives, list):
        fail(f"{file_path}: primitives must be an array")

    sprite: dict[str, Any] = {
        "version": data.get("version"),
        "id": data.get("id"),
        "width": data.get("width"),
        "height": data.get("height"),
        "primitives": [],
    }

    if sprite["version"] != 1:
        fail(f"{file_path}: version must be 1")

    if not isinstance(sprite["id"], str) or not re.fullmatch(r"[a-z0-9_-]+", sprite["id"]):
        fail(f"{file_path}: id must match [a-z0-9_-]+")

    require_int(f"{file_path}: width", sprite["width"], 1, SPRITE_MAX_VALUE)
    require_int(f"{file_path}: height", sprite["height"], 1, SPRITE_MAX_VALUE)

    parsed_primitives: list[dict[str, Any]] = []

    for index, item in enumerate(primitives):
        if not isinstance(item, dict):
            fail(f"{file_path}: primitive {index} must be an object")

        kind = item.get("kind")

        if kind not in KIND_ID:
            fail(f"{file_path}: primitive {index} has invalid kind {kind}")

        primitive: dict[str, Any] = {
            "kind": kind,
            "x": item.get("x"),
            "y": item.get("y"),
            "w": item.get("w"),
            "h": item.get("h"),
            "rotation": require_number(
                f"{file_path}: primitive {index} rotation",
                item.get("rotation"),
            ),
            "color": require_color(
                f"{file_path}: primitive {index} color",
                item.get("color"),
            ),
        }

        require_int(f"{file_path}: primitive {index} x", primitive["x"], 0, COMMAND_MAX_VALUE)
        require_int(f"{file_path}: primitive {index} y", primitive["y"], 0, COMMAND_MAX_VALUE)
        require_int(f"{file_path}: primitive {index} w", primitive["w"], 0, COMMAND_MAX_VALUE)
        require_int(f"{file_path}: primitive {index} h", primitive["h"], 0, COMMAND_MAX_VALUE)

        parsed_primitives.append(primitive)

    sprite["primitives"] = parsed_primitives
    return sprite


def validate_sprite_packable(sprite: dict[str, Any]) -> None:
    sprite_id = str(sprite["id"])

    require_int(f"{sprite_id}.width", sprite["width"], 1, SPRITE_MAX_VALUE)
    require_int(f"{sprite_id}.height", sprite["height"], 1, SPRITE_MAX_VALUE)

    primitives = sprite["primitives"]

    if not isinstance(primitives, list):
        fail(f"{sprite_id}: primitives must be a list")

    for index, primitive_raw in enumerate(primitives):
        if not isinstance(primitive_raw, dict):
            fail(f"{sprite_id}: primitive {index} must be an object")

        pack_command_value(f"{sprite_id}.primitives[{index}].x", int(primitive_raw["x"]))
        pack_command_value(f"{sprite_id}.primitives[{index}].y", int(primitive_raw["y"]))
        pack_command_value(f"{sprite_id}.primitives[{index}].w", int(primitive_raw["w"]))
        pack_command_value(f"{sprite_id}.primitives[{index}].h", int(primitive_raw["h"]))


def make_header() -> str:
    return """#pragma once

#include <stdint.h>

#define PACKED_SPRITE_CMD_RECT 0
#define PACKED_SPRITE_CMD_CIRCLE 1
#define PACKED_SPRITE_CMD_TRIANGLE 2

#pragma pack(push, 1)

typedef struct {
    uint8_t kind;
    uint8_t x;
    uint8_t y;
    uint8_t w;
    uint8_t h;
    uint8_t rotation;
    uint8_t color_index;
    uint8_t bank_mask;
} PackedSpriteCommand;

typedef struct {
    const char *id;

    uint16_t width;
    uint16_t height;

    const PackedSpriteCommand *commands;
    uint16_t command_count;
} PackedSpriteDefinition;

#pragma pack(pop)

extern const uint32_t SPRITE_PALETTE[];
extern const uint16_t SPRITE_PALETTE_COUNT;

extern const PackedSpriteDefinition *PACKED_SPRITE_DEFINITIONS[];
extern const uint16_t PACKED_SPRITE_DEFINITION_COUNT;
"""


def build_c_file(
    sprites: list[dict[str, Any]],
    palette: list[str],
    palette_index: dict[str, int],
) -> str:
    parts: list[str] = []

    parts.append("""#include "sprites_packed.h"

const uint32_t SPRITE_PALETTE[] = {
""")

    for color in palette:
        parts.append(f"    0x{color},\n")

    parts.append("};\n\n")
    parts.append(f"const uint16_t SPRITE_PALETTE_COUNT = {len(palette)};\n")

    for sprite in sprites:
        sprite_id = str(sprite["id"])
        name = symbol_name(sprite_id)
        primitives = sprite["primitives"]

        if not isinstance(primitives, list):
            fail(f"{sprite_id}: primitives must be a list")

        parts.append(f"\nstatic const PackedSpriteCommand {name}_COMMANDS[] = {{\n")

        for index, primitive_raw in enumerate(primitives):
            if not isinstance(primitive_raw, dict):
                fail(f"{sprite_id}: primitive {index} must be an object")

            primitive = primitive_raw

            x, x_bank = pack_command_value(f"{sprite_id}.primitives[{index}].x", int(primitive["x"]))
            y, y_bank = pack_command_value(f"{sprite_id}.primitives[{index}].y", int(primitive["y"]))
            w, w_bank = pack_command_value(f"{sprite_id}.primitives[{index}].w", int(primitive["w"]))
            h, h_bank = pack_command_value(f"{sprite_id}.primitives[{index}].h", int(primitive["h"]))

            bank_mask = (
                (x_bank << 0)
                | (y_bank << 2)
                | (w_bank << 4)
                | (h_bank << 6)
            )

            color = str(primitive["color"])
            color_id = palette_index[color]
            rotation = pack_rotation(float(primitive["rotation"]))
            kind = KIND_ID[str(primitive["kind"])]

            parts.append(
                f"    {{ {kind}, {x}, {y}, {w}, {h}, {rotation}, {color_id}, {bank_mask} }},\n"
            )

        parts.append("};\n")

        width = int(sprite["width"])
        height = int(sprite["height"])

        parts.append(f"""
static const PackedSpriteDefinition {name}_SPRITE = {{
    .id = "{sprite_id}",

    .width = {width},
    .height = {height},

    .commands = {name}_COMMANDS,
    .command_count = {len(primitives)},
}};
""")

    parts.append("\nconst PackedSpriteDefinition *PACKED_SPRITE_DEFINITIONS[] = {\n")

    for sprite in sprites:
        sprite_id = str(sprite["id"])
        parts.append(f"    &{symbol_name(sprite_id)}_SPRITE,\n")

    parts.append("};\n\n")
    parts.append(f"const uint16_t PACKED_SPRITE_DEFINITION_COUNT = {len(sprites)};\n")

    return "".join(parts)


def load_valid_sprites(files: list[Path]) -> tuple[list[dict[str, Any]], int]:
    sprites: list[dict[str, Any]] = []
    skipped = 0

    for file_path in files:
        try:
            sprite = load_sprite(file_path)
            validate_sprite_packable(sprite)
            sprites.append(sprite)
        except Exception as error:
            skipped += 1
            try:
                display_path = file_path.relative_to(PROJECT_ROOT)
            except ValueError:
                display_path = file_path

            warn(f"Skipping {display_path}: {error}")

    return sprites, skipped


def find_sprite_files() -> list[Path]:
    return sorted(
        file_path
        for file_path in SPRITE_ASSETS_DIR.rglob("*.json")
        if file_path.is_file()
    )


def validate_unique_sprite_ids(sprites: list[dict[str, Any]]) -> None:
    seen: set[str] = set()

    for sprite in sprites:
        sprite_id = str(sprite["id"])

        if sprite_id in seen:
            fail(f"Duplicate sprite id: {sprite_id}")

        seen.add(sprite_id)


def main() -> None:
    files = find_sprite_files()

    if not files:
        fail(f"No sprite JSON files found in {SPRITE_ASSETS_DIR}")

    sprites, skipped = load_valid_sprites(files)

    if not sprites:
        fail("No valid sprites found")

    validate_unique_sprite_ids(sprites)
    DEFINITIONS_DIR.mkdir(parents=True, exist_ok=True)

    palette: list[str] = []
    palette_index: dict[str, int] = {}

    for sprite in sprites:
        primitives = sprite["primitives"]

        if not isinstance(primitives, list):
            fail(f"{sprite['id']}: primitives must be a list")

        for primitive_raw in primitives:
            if not isinstance(primitive_raw, dict):
                fail(f"{sprite['id']}: primitive must be an object")

            color = str(primitive_raw["color"])

            if color not in palette_index:
                if len(palette) >= 256:
                    fail("Global palette is limited to 256 colors")

                palette_index[color] = len(palette)
                palette.append(color)

    (DEFINITIONS_DIR / "sprites_packed.h").write_text(
        make_header(),
        encoding="utf-8",
    )

    (DEFINITIONS_DIR / "sprites_packed.c").write_text(
        build_c_file(sprites, palette, palette_index),
        encoding="utf-8",
    )

    print(f"Packed sprites: {len(sprites)}")
    print(f"Skipped sprites: {skipped}")
    print(f"Palette colors: {len(palette)}")
    print(f"Input: {SPRITE_ASSETS_DIR}")
    print(f"Output: {DEFINITIONS_DIR}")


if __name__ == "__main__":
    main()