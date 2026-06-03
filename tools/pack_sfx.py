#!/usr/bin/env python3

from __future__ import annotations

import argparse
import json
import re
from dataclasses import dataclass
from pathlib import Path
from typing import Any


DEFAULT_INPUT_DIR = Path("src/assets/sfx")
DEFAULT_OUTPUT_FILE = Path("src/audio/generated_sound.c")

WAVE_MAP = {
    "square": "SOUND_WAVE_SQUARE",
    "triangle": "SOUND_WAVE_TRIANGLE",
    "sine": "SOUND_WAVE_SINE",
    "noise": "SOUND_WAVE_NOISE",
}

SOUND_ID_ORDER = {
    "jump": 0,
    "smash": 1,
    "damage": 2,
    "death": 3,
    "ork_death": 4,
    "boar_death": 5,
    "rat_death": 6,
}


@dataclass(frozen=True)
class SoundCommand:
    wave: str
    frequency_start: int
    frequency_end: int
    duration_ms: int
    volume: int


@dataclass(frozen=True)
class SoundDefinitionData:
    sound_id: str
    commands: list[SoundCommand]
    source_path: Path


def as_dict(value: Any, label: str) -> dict[str, Any]:
    if not isinstance(value, dict):
        raise ValueError(f"{label} must be an object")
    return value


def as_list(value: Any, label: str) -> list[Any]:
    if not isinstance(value, list):
        raise ValueError(f"{label} must be an array")
    return value


def as_str(value: Any, label: str) -> str:
    if not isinstance(value, str):
        raise ValueError(f"{label} must be a string")
    return value


def as_int(value: Any, label: str) -> int:
    if not isinstance(value, int):
        raise ValueError(f"{label} must be an integer")
    return value


def validate_id(value: str, label: str) -> None:
    if not re.fullmatch(r"[a-z0-9_-]+", value):
        raise ValueError(f"{label} must match [a-z0-9_-]+")


def validate_u8(value: int, label: str) -> None:
    if value < 0 or value > 255:
        raise ValueError(f"{label} must be in range 0..255")


def validate_u16(value: int, label: str) -> None:
    if value < 0 or value > 65535:
        raise ValueError(f"{label} must be in range 0..65535")


def make_symbol(value: str) -> str:
    symbol = re.sub(r"[^A-Za-z0-9]+", "_", value).strip("_").upper()

    if not symbol:
        raise ValueError("Generated C symbol is empty")

    if symbol[0].isdigit():
        symbol = f"SOUND_{symbol}"

    return symbol


def parse_sound_file(path: Path) -> SoundDefinitionData:
    data = as_dict(json.loads(path.read_text(encoding="utf-8")), str(path))

    file_type = as_str(data.get("type"), f"{path}: type")
    if file_type != "sfx":
        raise ValueError(f"{path}: type must be 'sfx'")

    sound_id = as_str(data.get("id"), f"{path}: id")
    validate_id(sound_id, f"{path}: id")

    raw_commands = as_list(data.get("commands"), f"{path}: commands")
    commands: list[SoundCommand] = []

    for index, raw_command in enumerate(raw_commands):
        label = f"{path}: commands[{index}]"
        command = as_dict(raw_command, label)

        wave = as_str(command.get("wave"), f"{label}.wave")
        if wave not in WAVE_MAP:
            if wave == "saw":
                raise ValueError(f"{label}: unsupported wave 'saw' because SoundWaveKind has no SOUND_WAVE_SAW")
            raise ValueError(f"{label}: unsupported wave '{wave}'")

        frequency_start = as_int(command.get("frequencyStart"), f"{label}.frequencyStart")
        frequency_end = as_int(command.get("frequencyEnd"), f"{label}.frequencyEnd")
        duration_ms = as_int(command.get("durationMs"), f"{label}.durationMs")
        volume = as_int(command.get("volume"), f"{label}.volume")

        validate_u16(frequency_start, f"{label}.frequencyStart")
        validate_u16(frequency_end, f"{label}.frequencyEnd")
        validate_u16(duration_ms, f"{label}.durationMs")
        validate_u8(volume, f"{label}.volume")

        if duration_ms == 0:
            raise ValueError(f"{label}: durationMs must be greater than 0")

        commands.append(
            SoundCommand(
                wave=wave,
                frequency_start=frequency_start,
                frequency_end=frequency_end,
                duration_ms=duration_ms,
                volume=volume,
            )
        )

    if not commands:
        raise ValueError(f"{path}: commands must not be empty")

    return SoundDefinitionData(
        sound_id=sound_id,
        commands=commands,
        source_path=path,
    )


def emit_sound_commands(sound: SoundDefinitionData) -> str:
    symbol = make_symbol(sound.sound_id)
    name = f"{symbol}_SOUND_COMMANDS"

    lines: list[str] = []
    lines.append(f"// Source: {sound.source_path.as_posix()}")
    lines.append(f"static const SoundCommand {name}[] = {{")

    for command in sound.commands:
        lines.append("    {")
        lines.append(f"        .wave = {WAVE_MAP[command.wave]},")
        lines.append(f"        .frequency_start = {command.frequency_start},")
        lines.append(f"        .frequency_end = {command.frequency_end},")
        lines.append(f"        .duration_ms = {command.duration_ms},")
        lines.append(f"        .volume = {command.volume},")
        lines.append("    },")

    lines.append("};")
    return "\n".join(lines)


def emit_sound_registry(sounds: list[SoundDefinitionData]) -> str:
    lines: list[str] = []

    lines.append("const SoundDefinition PACKED_SOUND_DEFINITIONS[] = {")

    for sound in sounds:
        symbol = make_symbol(sound.sound_id)
        commands_name = f"{symbol}_SOUND_COMMANDS"

        lines.append("    {")
        lines.append(f'        .id = "{sound.sound_id}",')
        lines.append(f"        .commands = {commands_name},")
        lines.append(f"        .command_count = sizeof({commands_name}) / sizeof({commands_name}[0]),")
        lines.append("    },")

    lines.append("};")
    lines.append("")
    lines.append(
        "const size_t PACKED_SOUND_COUNT = "
        "sizeof(PACKED_SOUND_DEFINITIONS) / sizeof(PACKED_SOUND_DEFINITIONS[0]);"
    )

    return "\n".join(lines)


def emit_sound_c(sounds: list[SoundDefinitionData]) -> str:
    chunks: list[str] = []

    chunks.append("// Generated by tools/pack_sfx.py. Do not edit manually.")
    chunks.append('#include "generated_sound.h"')
    chunks.append("")

    for sound in sounds:
        chunks.append(emit_sound_commands(sound))
        chunks.append("")

    chunks.append(emit_sound_registry(sounds))
    chunks.append("")

    return "\n".join(chunks)


def load_sounds(input_dir: Path) -> list[SoundDefinitionData]:
    paths = sorted(input_dir.glob("*.sfx.json"))
    sounds = [parse_sound_file(path) for path in paths]

    used_ids: set[str] = set()

    for sound in sounds:
        if sound.sound_id in used_ids:
            raise ValueError(f"Duplicate SFX id: {sound.sound_id}")

        used_ids.add(sound.sound_id)

    sounds.sort(key=lambda item: (SOUND_ID_ORDER.get(item.sound_id, len(SOUND_ID_ORDER)), item.sound_id))
    return sounds


def convert_all(input_dir: Path, output_file: Path) -> None:
    if not input_dir.exists():
        raise FileNotFoundError(f"Input directory does not exist: {input_dir}")

    sounds = load_sounds(input_dir)

    if not sounds:
        print(f"No *.sfx.json files found in {input_dir}")
        return

    output_file.parent.mkdir(parents=True, exist_ok=True)
    output_file.write_text(emit_sound_c(sounds), encoding="utf-8")

    print(f"Generated {output_file}")
    print(f"Packed {len(sounds)} sound definition(s)")


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Pack Little One SFX JSON files into one C source file."
    )

    parser.add_argument(
        "--input-dir",
        type=Path,
        default=DEFAULT_INPUT_DIR,
        help="Directory with *.sfx.json files.",
    )

    parser.add_argument(
        "--output",
        type=Path,
        default=DEFAULT_OUTPUT_FILE,
        help="Generated C output file.",
    )

    args = parser.parse_args()
    convert_all(args.input_dir, args.output)


if __name__ == "__main__":
    main()
