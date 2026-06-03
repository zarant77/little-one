#!/usr/bin/env python3

import json
import re
from pathlib import Path
from typing import Any

PROJECT_ROOT = Path(__file__).resolve().parents[1]

ASSETS_DIR = PROJECT_ROOT / "src" / "assets"
FONT_ASSETS_DIR = ASSETS_DIR / "fonts"

OUTPUT_DIR = PROJECT_ROOT / "src" / "fonts"
OUTPUT_HEADER = OUTPUT_DIR / "fonts_packed.h"
OUTPUT_SOURCE = OUTPUT_DIR / "fonts_packed.c"

FONT_ID_RE = re.compile(r"[a-z0-9_-]+")
LINE_RE = re.compile(r"[0-9a-fA-F]{4}")

SUPPORTED_VERSION = 1
SUPPORTED_TYPE = "vector"
SUPPORTED_GRID_SIZE = 16

UINT8_MAX = 255


def fail(message: str) -> None:
    raise RuntimeError(message)


def warn(message: str) -> None:
    print(f"[WARNING] {message}")


def require_object(name: str, value: Any) -> dict[str, Any]:
    if not isinstance(value, dict):
        fail(f"{name} must be an object")
    return value


def require_array(name: str, value: Any) -> list[Any]:
    if not isinstance(value, list):
        fail(f"{name} must be an array")
    return value


def require_string(name: str, value: Any) -> str:
    if not isinstance(value, str):
        fail(f"{name} must be a string")
    return value


def require_int(name: str, value: Any, min_value: int, max_value: int) -> int:
    if not isinstance(value, int) or value < min_value or value > max_value:
        fail(f"{name} must be an integer in range {min_value}..{max_value}, got {value}")
    return value


def sanitize_symbol(value: str) -> str:
    symbol = re.sub(r"[^a-zA-Z0-9_]", "_", value).upper()
    if not symbol:
        fail("symbol cannot be empty")
    if symbol[0].isdigit():
        symbol = f"_{symbol}"
    return symbol


def glyph_symbol_suffix(codepoint: int) -> str:
    return f"U{codepoint:04X}"


def escape_c_comment(value: str) -> str:
    return value.replace("\\", "\\\\").replace("*/", "* /").replace("\n", "\\n")


def escape_c_string(value: str) -> str:
    return (
        value
        .replace("\\", "\\\\")
        .replace('"', '\\"')
        .replace("\n", "\\n")
        .replace("\r", "\\r")
        .replace("\t", "\\t")
    )


def parse_line(file_path: Path, glyph_char: str, index: int, value: Any) -> str:
    line = require_string(f"{file_path}: glyph {glyph_char!r} line {index}", value)
    if not LINE_RE.fullmatch(line):
        fail(f"{file_path}: glyph {glyph_char!r} line {index} must be exactly 4 hex characters, got {line!r}")
    return line.upper()


def load_font(file_path: Path) -> dict[str, Any]:
    data = json.loads(file_path.read_text(encoding="utf-8"))
    root = require_object(f"{file_path}: root", data)

    version = root.get("version")
    font_id = root.get("id")
    font_type = root.get("type")
    grid_size = root.get("gridSize")
    default_advance = root.get("defaultAdvance")
    line_thickness = root.get("lineThickness")
    glyphs_raw = root.get("glyphs")

    if version != SUPPORTED_VERSION:
        fail(f"{file_path}: version must be {SUPPORTED_VERSION}")

    font_id = require_string(f"{file_path}: id", font_id)
    if not FONT_ID_RE.fullmatch(font_id):
        fail(f"{file_path}: id must match [a-z0-9_-]+")

    if font_type != SUPPORTED_TYPE:
        fail(f"{file_path}: type must be {SUPPORTED_TYPE!r}")

    grid_size = require_int(f"{file_path}: gridSize", grid_size, SUPPORTED_GRID_SIZE, SUPPORTED_GRID_SIZE)
    default_advance = require_int(f"{file_path}: defaultAdvance", default_advance, 0, UINT8_MAX)
    line_thickness = require_int(f"{file_path}: lineThickness", line_thickness, 1, UINT8_MAX)

    glyph_items = require_array(f"{file_path}: glyphs", glyphs_raw)
    glyphs: list[dict[str, Any]] = []
    seen_chars: set[str] = set()

    for glyph_index, glyph_raw in enumerate(glyph_items):
        glyph = require_object(f"{file_path}: glyph {glyph_index}", glyph_raw)
        glyph_char = require_string(f"{file_path}: glyph {glyph_index} char", glyph.get("char"))

        if len(glyph_char) != 1:
            fail(f"{file_path}: glyph {glyph_index} char must contain exactly one Unicode character")

        if glyph_char in seen_chars:
            fail(f"{file_path}: duplicate glyph char {glyph_char!r}")

        seen_chars.add(glyph_char)

        glyph_name_raw = glyph.get("name")
        glyph_name = None
        if glyph_name_raw is not None:
            glyph_name = require_string(f"{file_path}: glyph {glyph_char!r} name", glyph_name_raw)

        advance = require_int(
            f"{file_path}: glyph {glyph_char!r} advance",
            glyph.get("advance", default_advance),
            0,
            UINT8_MAX,
        )

        lines_raw = require_array(f"{file_path}: glyph {glyph_char!r} lines", glyph.get("lines"))
        if len(lines_raw) > UINT8_MAX:
            fail(f"{file_path}: glyph {glyph_char!r} has too many lines, max {UINT8_MAX}")

        lines = [
            parse_line(file_path, glyph_char, line_index, line_raw)
            for line_index, line_raw in enumerate(lines_raw)
        ]

        glyphs.append({
            "char": glyph_char,
            "name": glyph_name,
            "codepoint": ord(glyph_char),
            "advance": advance,
            "lines": lines,
        })

    return {
        "id": font_id,
        "grid_size": grid_size,
        "default_advance": default_advance,
        "line_thickness": line_thickness,
        "glyphs": glyphs,
        "source": file_path,
    }


def load_valid_fonts(files: list[Path]) -> tuple[list[dict[str, Any]], int]:
    fonts: list[dict[str, Any]] = []
    skipped = 0
    seen_ids: dict[str, Path] = {}

    for file_path in files:
        try:
            font = load_font(file_path)
            font_id = str(font["id"])

            if font_id in seen_ids:
                fail(f"{file_path}: duplicate font id {font_id!r}, already used by {seen_ids[font_id]}")

            seen_ids[font_id] = file_path
            fonts.append(font)
        except Exception as error:
            skipped += 1
            warn(f"Skipping {file_path}: {error}")

    return fonts, skipped


def make_header() -> str:
    return """#pragma once

#include <stdint.h>

typedef uint16_t PackedFontLine;

typedef struct {
    uint32_t codepoint;
    uint8_t advance;
    const PackedFontLine *lines;
    uint8_t line_count;
} PackedFontGlyph;

typedef struct {
    const char *id;

    uint8_t grid_size;
    uint8_t default_advance;
    uint8_t line_thickness;

    const PackedFontGlyph *glyphs;
    uint16_t glyph_count;
} PackedFont;

extern const PackedFont *PACKED_FONT_DEFINITIONS[];
extern const uint16_t PACKED_FONT_DEFINITION_COUNT;
"""


def build_c_file(fonts: list[dict[str, Any]]) -> str:
    parts: list[str] = []
    parts.append('#include "fonts_packed.h"\n\n')

    for font in fonts:
        font_id = str(font["id"])
        font_symbol = sanitize_symbol(font_id)
        glyphs = font["glyphs"]

        if not isinstance(glyphs, list):
            fail(f"{font_id}: glyphs must be a list")

        for glyph in glyphs:
            if not isinstance(glyph, dict):
                fail(f"{font_id}: glyph must be an object")

            lines = glyph["lines"]
            if not isinstance(lines, list):
                fail(f"{font_id}: glyph lines must be a list")

            if len(lines) == 0:
                continue

            codepoint = int(glyph["codepoint"])
            suffix = glyph_symbol_suffix(codepoint)
            parts.append(f"static const PackedFontLine {font_symbol}_{suffix}_LINES[] = {{\n")

            for line in lines:
                parts.append(f"    0x{str(line).upper()},\n")

            parts.append("};\n\n")

        parts.append(f"static const PackedFontGlyph {font_symbol}_GLYPHS[] = {{\n")

        for glyph in glyphs:
            if not isinstance(glyph, dict):
                fail(f"{font_id}: glyph must be an object")

            glyph_char = str(glyph["char"])
            codepoint = int(glyph["codepoint"])
            advance = int(glyph["advance"])
            lines = glyph["lines"]

            if not isinstance(lines, list):
                fail(f"{font_id}: glyph lines must be a list")

            suffix = glyph_symbol_suffix(codepoint)
            line_ref = "0"
            if len(lines) > 0:
                line_ref = f"{font_symbol}_{suffix}_LINES"

            parts.append(
                f"    {{ 0x{codepoint:04X}, {advance}, {line_ref}, {len(lines)} }},"
                f" /* {escape_c_comment(glyph_char)} */\n"
            )

        parts.append("};\n\n")
        parts.append(f"""static const PackedFont {font_symbol}_FONT = {{
    .id = "{escape_c_string(font_id)}",

    .grid_size = {int(font["grid_size"])},
    .default_advance = {int(font["default_advance"])},
    .line_thickness = {int(font["line_thickness"])},

    .glyphs = {font_symbol}_GLYPHS,
    .glyph_count = {len(glyphs)},
}};

""")

    parts.append("const PackedFont *PACKED_FONT_DEFINITIONS[] = {\n")

    for font in fonts:
        font_id = str(font["id"])
        font_symbol = sanitize_symbol(font_id)
        parts.append(f"    &{font_symbol}_FONT,\n")

    parts.append("};\n\n")
    parts.append(f"const uint16_t PACKED_FONT_DEFINITION_COUNT = {len(fonts)};\n")
    return "".join(parts)


def main() -> None:
    files = sorted(FONT_ASSETS_DIR.rglob("*.font.json"))

    if not files:
        fail(f"No *.font.json files found in {FONT_ASSETS_DIR}")

    fonts, skipped = load_valid_fonts(files)

    if not fonts:
        fail("No valid fonts found")

    OUTPUT_DIR.mkdir(parents=True, exist_ok=True)
    OUTPUT_HEADER.write_text(make_header(), encoding="utf-8")
    OUTPUT_SOURCE.write_text(build_c_file(fonts), encoding="utf-8")

    glyph_count = sum(len(font["glyphs"]) for font in fonts)
    line_count = sum(len(glyph["lines"]) for font in fonts for glyph in font["glyphs"])

    print(f"Packed fonts: {len(fonts)}")
    print(f"Packed glyphs: {glyph_count}")
    print(f"Packed lines: {line_count}")
    print(f"Skipped fonts: {skipped}")
    print(f"Input: {FONT_ASSETS_DIR}")
    print(f"Output: {OUTPUT_DIR}")


if __name__ == "__main__":
    main()
