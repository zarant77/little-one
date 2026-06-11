#!/usr/bin/env python3

from __future__ import annotations

import json
import math
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
PATH_MAX_THICKNESS = 256
PATH_MAX_SEGMENTS = 64

KIND_ID = {
    "rect": 0,
    "circle": 1,
    "triangle": 2,
}

PATH_CAPS = {"butt", "round"}
PATH_JOINS = {"round"}
PATH_SMOOTHING = {"none", "quadratic"}

Point = tuple[float, float]


class SpritePackError(RuntimeError):
    pass


def fail(message: str) -> None:
    raise SpritePackError(message)


def warn(message: str) -> None:
    print(f"[WARNING] {message}")


def require_int(name: str, value: Any, min_value: int, max_value: int) -> None:
    if isinstance(value, bool) or not isinstance(value, int) or value < min_value or value > max_value:
        fail(f"{name} must be an integer in range {min_value}..{max_value}, got {value}")


def require_number(name: str, value: Any) -> float:
    if isinstance(value, bool) or not isinstance(value, (int, float)):
        fail(f"{name} must be a number, got {value}")

    number = float(value)

    if not math.isfinite(number):
        fail(f"{name} must be finite, got {value}")

    return number


def require_number_range(name: str, value: Any, min_value: float, max_value: float) -> float:
    number = require_number(name, value)

    if number < min_value or number > max_value:
        fail(f"{name} must be in range {min_value}..{max_value}, got {value}")

    return number


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


def round_command_number(value: float) -> int:
    return int(round(value))


def clamp_command_number(value: float) -> int:
    return max(0, min(COMMAND_MAX_VALUE, round_command_number(value)))


def validate_sprite_id(file_path: Path, value: Any) -> str:
    if not isinstance(value, str) or not re.fullmatch(r"[a-z0-9_-]+", value):
        fail(f"{file_path}: id must match [a-z0-9_-]+")

    return value


def validate_sprite_version(file_path: Path, value: Any) -> int:
    if isinstance(value, bool) or not isinstance(value, int):
        fail(f"{file_path}: version must be an integer")

    if value not in (1, 2):
        fail(f"{file_path}: version must be 1 or 2")

    return value


def parse_basic_primitive(file_path: Path, item: dict[str, Any], location: str) -> dict[str, Any]:
    kind = item.get("kind")

    if kind not in KIND_ID:
        fail(f"{file_path}: {location} has invalid kind {kind}")

    primitive: dict[str, Any] = {
        "kind": kind,
        "x": item.get("x"),
        "y": item.get("y"),
        "w": item.get("w"),
        "h": item.get("h"),
        "rotation": require_number(
            f"{file_path}: {location} rotation",
            item.get("rotation"),
        ),
        "color": require_color(
            f"{file_path}: {location} color",
            item.get("color"),
        ),
    }

    require_int(f"{file_path}: {location} x", primitive["x"], 0, COMMAND_MAX_VALUE)
    require_int(f"{file_path}: {location} y", primitive["y"], 0, COMMAND_MAX_VALUE)
    require_int(f"{file_path}: {location} w", primitive["w"], 0, COMMAND_MAX_VALUE)
    require_int(f"{file_path}: {location} h", primitive["h"], 0, COMMAND_MAX_VALUE)

    return primitive


def parse_point(file_path: Path, value: Any, location: str) -> Point:
    if not isinstance(value, list) or len(value) != 2:
        fail(f"{file_path}: {location} must be [x, y]")

    x = require_number_range(f"{file_path}: {location}[0]", value[0], 0, COMMAND_MAX_VALUE)
    y = require_number_range(f"{file_path}: {location}[1]", value[1], 0, COMMAND_MAX_VALUE)

    return x, y


def parse_path_primitive(file_path: Path, item: dict[str, Any], location: str) -> dict[str, Any]:
    raw_points = item.get("points")

    if not isinstance(raw_points, list):
        fail(f"{file_path}: {location}.points must be an array")

    points = [
        parse_point(file_path, raw_point, f"{location}.points[{index}]")
        for index, raw_point in enumerate(raw_points)
    ]

    if len(points) < 2:
        fail(f"{file_path}: {location}.points must contain at least 2 points")

    thickness = require_number_range(
        f"{file_path}: {location}.thickness",
        item.get("thickness"),
        1,
        PATH_MAX_THICKNESS,
    )

    cap = item.get("cap", "round")
    join = item.get("join", "round")
    smoothing = item.get("smoothing", "none")
    segments = item.get("segments", 8)

    if cap not in PATH_CAPS:
        warn(f"{file_path}: {location}.cap has invalid value {cap}, using round")
        cap = "round"

    if join not in PATH_JOINS:
        warn(f"{file_path}: {location}.join has invalid value {join}, using round")
        join = "round"

    if smoothing not in PATH_SMOOTHING:
        warn(f"{file_path}: {location}.smoothing has invalid value {smoothing}, using none")
        smoothing = "none"

    require_int(f"{file_path}: {location}.segments", segments, 1, PATH_MAX_SEGMENTS)

    return {
        "kind": "path",
        "points": points,
        "thickness": thickness,
        "cap": cap,
        "join": join,
        "smoothing": smoothing,
        "segments": int(segments),
        "color": require_color(f"{file_path}: {location}.color", item.get("color")),
    }


def parse_editor_primitive(file_path: Path, item: Any, location: str) -> dict[str, Any]:
    if not isinstance(item, dict):
        fail(f"{file_path}: {location} must be an object")

    kind = item.get("kind")

    if kind == "path":
        return parse_path_primitive(file_path, item, location)

    return parse_basic_primitive(file_path, item, location)


def midpoint(a: Point, b: Point) -> Point:
    return (a[0] + b[0]) * 0.5, (a[1] + b[1]) * 0.5


def quadratic_point(p0: Point, control: Point, p1: Point, t: float) -> Point:
    mt = 1.0 - t
    x = mt * mt * p0[0] + 2.0 * mt * t * control[0] + t * t * p1[0]
    y = mt * mt * p0[1] + 2.0 * mt * t * control[1] + t * t * p1[1]

    return x, y


def smooth_quadratic_points(points: list[Point], segments: int) -> list[Point]:
    if len(points) <= 2:
        return points[:]

    result: list[Point] = [points[0]]
    last_control_index = len(points) - 2

    for control_index in range(1, len(points) - 1):
        control = points[control_index]

        if control_index == 1:
            start = points[0]
        else:
            start = midpoint(points[control_index - 1], control)

        if control_index == last_control_index:
            end = points[-1]
        else:
            end = midpoint(control, points[control_index + 1])

        if result[-1] != start:
            result.append(start)

        for step in range(1, segments + 1):
            t = step / segments
            result.append(quadratic_point(start, control, end, t))

    return result


def path_generated_points(path: dict[str, Any]) -> list[Point]:
    points = path["points"]

    if path["smoothing"] == "quadratic":
        return smooth_quadratic_points(points, int(path["segments"]))

    return points[:]


def make_rect_from_segment(a: Point, b: Point, thickness: float, color: str) -> dict[str, Any] | None:
    dx = b[0] - a[0]
    dy = b[1] - a[1]
    length = math.hypot(dx, dy)

    if length < 0.5:
        return None

    return {
        "kind": "rect",
        "x": clamp_command_number((a[0] + b[0]) * 0.5),
        "y": clamp_command_number((a[1] + b[1]) * 0.5),
        "w": max(1, min(COMMAND_MAX_VALUE, round_command_number(length))),
        "h": max(1, min(COMMAND_MAX_VALUE, round_command_number(thickness))),
        "rotation": math.degrees(math.atan2(dy, dx)),
        "color": color,
    }


def make_circle_at_point(point: Point, diameter: int, color: str) -> dict[str, Any]:
    return {
        "kind": "circle",
        "x": clamp_command_number(point[0]),
        "y": clamp_command_number(point[1]),
        "w": diameter,
        "h": diameter,
        "rotation": 0,
        "color": color,
    }


def expand_path_to_primitives(path: dict[str, Any]) -> list[dict[str, Any]]:
    points = path_generated_points(path)
    thickness = float(path["thickness"])
    color = str(path["color"])
    cap = str(path["cap"])
    join = str(path["join"])

    generated: list[dict[str, Any]] = []

    for index in range(len(points) - 1):
        rect = make_rect_from_segment(points[index], points[index + 1], thickness, color)

        if rect is not None:
            generated.append(rect)

    diameter = max(1, min(COMMAND_MAX_VALUE, round_command_number(thickness)))

    if join == "round" and len(points) > 2:
        for point in points[1:-1]:
            generated.append(make_circle_at_point(point, diameter, color))

    if cap == "round":
        generated.append(make_circle_at_point(points[0], diameter, color))
        generated.append(make_circle_at_point(points[-1], diameter, color))

    return generated


def expand_editor_primitive(primitive: dict[str, Any]) -> list[dict[str, Any]]:
    if primitive["kind"] == "path":
        return expand_path_to_primitives(primitive)

    return [primitive]


def parse_and_expand_primitive(file_path: Path, item: Any, location: str) -> list[dict[str, Any]]:
    primitive = parse_editor_primitive(file_path, item, location)
    return expand_editor_primitive(primitive)


def flatten_nodes(file_path: Path, nodes: Any, location: str = "nodes") -> list[dict[str, Any]]:
    if not isinstance(nodes, list):
        fail(f"{file_path}: {location} must be an array")

    primitives: list[dict[str, Any]] = []

    for index, node in enumerate(nodes):
        node_location = f"{location}[{index}]"

        if not isinstance(node, dict):
            warn(f"Skipping {file_path}: {node_location} is not an object")
            continue

        node_type = node.get("type")

        if node_type == "primitive":
            primitive = node.get("primitive")

            if primitive is None:
                warn(f"Skipping {file_path}: {node_location}.primitive is missing")
                continue

            primitives.extend(
                parse_and_expand_primitive(
                    file_path,
                    primitive,
                    f"{node_location}.primitive",
                )
            )
            continue

        if node_type == "group":
            children = node.get("children", [])

            if children is None:
                children = []

            if not isinstance(children, list):
                warn(f"Skipping children of {file_path}: {node_location}.children is not an array")
                continue

            primitives.extend(
                flatten_nodes(
                    file_path,
                    children,
                    f"{node_location}.children",
                )
            )
            continue

        warn(f"Skipping {file_path}: {node_location} has invalid type {node_type}")

    return primitives


def parse_flat_primitives(file_path: Path, primitives: Any) -> list[dict[str, Any]]:
    if not isinstance(primitives, list):
        fail(f"{file_path}: primitives must be an array")

    parsed_primitives: list[dict[str, Any]] = []

    for index, item in enumerate(primitives):
        parsed_primitives.extend(
            parse_and_expand_primitive(file_path, item, f"primitive {index}")
        )

    return parsed_primitives


def load_sprite(file_path: Path) -> dict[str, Any]:
    data = json.loads(file_path.read_text(encoding="utf-8"))

    if not isinstance(data, dict):
        fail(f"{file_path}: root must be an object")

    version = validate_sprite_version(file_path, data.get("version"))
    sprite_id = validate_sprite_id(file_path, data.get("id"))
    width = data.get("width")
    height = data.get("height")
    pivot_x = data.get("pivotX", data.get("pivot_x", 0))
    pivot_y = data.get("pivotY", data.get("pivot_y", 0))

    require_int(f"{file_path}: width", width, 1, SPRITE_MAX_VALUE)
    require_int(f"{file_path}: height", height, 1, SPRITE_MAX_VALUE)
    require_int(f"{file_path}: pivotX", pivot_x, 0, COMMAND_MAX_VALUE)
    require_int(f"{file_path}: pivotY", pivot_y, 0, COMMAND_MAX_VALUE)

    if "nodes" in data:
        primitives = flatten_nodes(file_path, data.get("nodes"))
        source_format = "nodes"
    elif "primitives" in data:
        primitives = parse_flat_primitives(file_path, data.get("primitives"))
        source_format = "primitives"
    else:
        fail(f"{file_path}: expected nodes or primitives")

    return {
        "version": version,
        "id": sprite_id,
        "width": width,
        "height": height,
        "pivot_x": pivot_x,
        "pivot_y": pivot_y,
        "primitives": primitives,
        "source_format": source_format,
    }


def validate_sprite_packable(sprite: dict[str, Any]) -> None:
    sprite_id = str(sprite["id"])

    require_int(f"{sprite_id}.width", sprite["width"], 1, SPRITE_MAX_VALUE)
    require_int(f"{sprite_id}.height", sprite["height"], 1, SPRITE_MAX_VALUE)
    require_int(f"{sprite_id}.pivot_x", sprite["pivot_x"], 0, COMMAND_MAX_VALUE)
    require_int(f"{sprite_id}.pivot_y", sprite["pivot_y"], 0, COMMAND_MAX_VALUE)

    primitives = sprite["primitives"]

    if not isinstance(primitives, list):
        fail(f"{sprite_id}: primitives must be a list")

    for index, primitive_raw in enumerate(primitives):
        if not isinstance(primitive_raw, dict):
            fail(f"{sprite_id}: primitive {index} must be an object")

        kind = primitive_raw.get("kind")

        if kind not in KIND_ID:
            fail(f"{sprite_id}: generated primitive {index} has invalid kind {kind}")

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

    uint16_t pivot_x;
    uint16_t pivot_y;

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
        pivot_x = int(sprite["pivot_x"])
        pivot_y = int(sprite["pivot_y"])

        parts.append(f"""
static const PackedSpriteDefinition {name}_SPRITE = {{
    .id = "{sprite_id}",

    .width = {width},
    .height = {height},

    .pivot_x = {pivot_x},
    .pivot_y = {pivot_y},

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


def build_palette(sprites: list[dict[str, Any]]) -> tuple[list[str], dict[str, int]]:
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

    return palette, palette_index


def main() -> None:
    files = find_sprite_files()

    if not files:
        fail(f"No sprite JSON files found in {SPRITE_ASSETS_DIR}")

    sprites, skipped = load_valid_sprites(files)

    if not sprites:
        fail("No valid sprites found")

    validate_unique_sprite_ids(sprites)
    DEFINITIONS_DIR.mkdir(parents=True, exist_ok=True)

    palette, palette_index = build_palette(sprites)

    (DEFINITIONS_DIR / "sprites_packed.h").write_text(
        make_header(),
        encoding="utf-8",
    )

    (DEFINITIONS_DIR / "sprites_packed.c").write_text(
        build_c_file(sprites, palette, palette_index),
        encoding="utf-8",
    )

    total_commands = sum(len(sprite["primitives"]) for sprite in sprites)

    print(f"Packed sprites: {len(sprites)}")
    print(f"Skipped sprites: {skipped}")
    print(f"Packed commands: {total_commands}")
    print(f"Palette colors: {len(palette)}")
    print(f"Input: {SPRITE_ASSETS_DIR}")
    print(f"Output: {DEFINITIONS_DIR}")


if __name__ == "__main__":
    main()
