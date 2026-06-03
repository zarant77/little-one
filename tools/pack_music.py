#!/usr/bin/env python3

from __future__ import annotations

import argparse
import json
import re
from dataclasses import dataclass
from pathlib import Path
from typing import Any


DEFAULT_INPUT_DIR = Path("src/assets/music")
DEFAULT_OUTPUT_FILE = Path("src/audio/generated_music.c")

WAVE_MAP = {
    "square": "SOUND_WAVE_SQUARE",
    "triangle": "SOUND_WAVE_TRIANGLE",
    "sine": "SOUND_WAVE_SINE",
    "noise": "SOUND_WAVE_NOISE",
}


@dataclass(frozen=True)
class MusicInstrument:
    instrument_id: str
    wave: str
    volume: int
    attack_ms: int
    decay_ms: int


@dataclass(frozen=True)
class MusicNote:
    instrument_index: int
    start_tick: int
    duration_ticks: int
    midi_note: int
    volume: int


@dataclass(frozen=True)
class MusicPattern:
    music_id: str
    bpm: int
    ticks_per_beat: int
    length_ticks: int
    instruments: list[MusicInstrument]
    notes: list[MusicNote]
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


def optional_int(data: dict[str, Any], key: str, default: int, label: str) -> int:
    value = data.get(key, default)
    return as_int(value, f"{label}.{key}")


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
        symbol = f"MUSIC_{symbol}"

    return symbol


def parse_music_file(path: Path) -> MusicPattern:
    data = as_dict(json.loads(path.read_text(encoding="utf-8")), str(path))

    file_type = as_str(data.get("type"), f"{path}: type")
    if file_type != "music":
        raise ValueError(f"{path}: type must be 'music'")

    music_id = as_str(data.get("id"), f"{path}: id")
    validate_id(music_id, f"{path}: id")

    bpm = as_int(data.get("bpm"), f"{path}: bpm")
    if bpm <= 0 or bpm > 255:
        raise ValueError(f"{path}: bpm must be in range 1..255")

    ticks_per_beat = as_int(data.get("ticksPerBeat"), f"{path}: ticksPerBeat")
    if ticks_per_beat <= 0 or ticks_per_beat > 255:
        raise ValueError(f"{path}: ticksPerBeat must be in range 1..255")

    length_ticks = as_int(data.get("lengthTicks"), f"{path}: lengthTicks")
    if length_ticks <= 0 or length_ticks > 65535:
        raise ValueError(f"{path}: lengthTicks must be in range 1..65535")

    raw_instruments = as_list(data.get("instruments"), f"{path}: instruments")
    instruments: list[MusicInstrument] = []

    for index, raw_instrument in enumerate(raw_instruments):
        label = f"{path}: instruments[{index}]"
        instrument = as_dict(raw_instrument, label)

        instrument_id = as_str(instrument.get("id"), f"{label}.id")
        validate_id(instrument_id, f"{label}.id")

        wave = as_str(instrument.get("wave"), f"{label}.wave")
        if wave not in WAVE_MAP:
            if wave == "saw":
                raise ValueError(f"{label}: unsupported wave 'saw' because SoundWaveKind has no SOUND_WAVE_SAW")
            raise ValueError(f"{label}: unsupported wave '{wave}'")

        volume = as_int(instrument.get("volume"), f"{label}.volume")
        validate_u8(volume, f"{label}.volume")

        attack_ms = optional_int(instrument, "attackMs", 0, label)
        decay_ms = optional_int(instrument, "decayMs", 0, label)

        validate_u16(attack_ms, f"{label}.attackMs")
        validate_u16(decay_ms, f"{label}.decayMs")

        instruments.append(
            MusicInstrument(
                instrument_id=instrument_id,
                wave=wave,
                volume=volume,
                attack_ms=attack_ms,
                decay_ms=decay_ms,
            )
        )

    raw_notes = as_list(data.get("notes"), f"{path}: notes")
    notes: list[MusicNote] = []

    for index, raw_note in enumerate(raw_notes):
        label = f"{path}: notes[{index}]"
        note = as_dict(raw_note, label)

        instrument_index = as_int(note.get("instrument"), f"{label}.instrument")
        if instrument_index < 0 or instrument_index >= len(instruments):
            raise ValueError(f"{label}: instrument index is out of range")

        midi_note = as_int(note.get("note"), f"{label}.note")
        if midi_note < 0 or midi_note > 127:
            raise ValueError(f"{label}: note must be in range 0..127")

        start_tick = as_int(note.get("startTick"), f"{label}.startTick")
        duration_ticks = as_int(note.get("durationTicks"), f"{label}.durationTicks")
        volume = as_int(note.get("volume"), f"{label}.volume")

        validate_u16(start_tick, f"{label}.startTick")
        validate_u16(duration_ticks, f"{label}.durationTicks")
        validate_u8(volume, f"{label}.volume")

        if duration_ticks == 0:
            raise ValueError(f"{label}: durationTicks must be greater than 0")

        if start_tick >= length_ticks:
            raise ValueError(f"{label}: startTick must be inside song length")

        if start_tick + duration_ticks > length_ticks:
            raise ValueError(f"{label}: note exceeds song length")

        notes.append(
            MusicNote(
                instrument_index=instrument_index,
                start_tick=start_tick,
                duration_ticks=duration_ticks,
                midi_note=midi_note,
                volume=volume,
            )
        )

    notes.sort(key=lambda item: (item.start_tick, item.instrument_index, item.midi_note))

    return MusicPattern(
        music_id=music_id,
        bpm=bpm,
        ticks_per_beat=ticks_per_beat,
        length_ticks=length_ticks,
        instruments=instruments,
        notes=notes,
        source_path=path,
    )


def emit_instruments(pattern: MusicPattern) -> str:
    symbol = make_symbol(pattern.music_id)
    name = f"{symbol}_MUSIC_INSTRUMENTS"

    lines: list[str] = []
    lines.append(f"// Source: {pattern.source_path.as_posix()}")
    lines.append(f"static const MusicInstrument {name}[] = {{")

    for instrument in pattern.instruments:
        lines.append("    {")
        lines.append(f"        .wave = {WAVE_MAP[instrument.wave]},")
        lines.append(f"        .volume = {instrument.volume},")
        lines.append("    },")

    lines.append("};")
    return "\n".join(lines)


def emit_notes(pattern: MusicPattern) -> str:
    symbol = make_symbol(pattern.music_id)
    name = f"{symbol}_MUSIC_NOTES"

    lines: list[str] = []
    lines.append(f"static const MusicNote {name}[] = {{")

    for note in pattern.notes:
        lines.append("    {")
        lines.append(f"        .instrument = {note.instrument_index},")
        lines.append(f"        .midi_note = {note.midi_note},")
        lines.append(f"        .start_tick = {note.start_tick},")
        lines.append(f"        .duration_ticks = {note.duration_ticks},")
        lines.append(f"        .volume = {note.volume},")
        lines.append("    },")

    lines.append("};")
    return "\n".join(lines)


def emit_music_registry(patterns: list[MusicPattern]) -> str:
    lines: list[str] = []

    lines.append("const MusicDefinition PACKED_MUSIC_DEFINITIONS[] = {")

    for pattern in patterns:
        symbol = make_symbol(pattern.music_id)
        instruments_name = f"{symbol}_MUSIC_INSTRUMENTS"
        notes_name = f"{symbol}_MUSIC_NOTES"

        lines.append("    {")
        lines.append(f'        .id = "{pattern.music_id}",')
        lines.append(f"        .bpm = {pattern.bpm},")
        lines.append(f"        .ticks_per_beat = {pattern.ticks_per_beat},")
        lines.append(f"        .loop_ticks = {pattern.length_ticks},")
        lines.append(f"        .instruments = {instruments_name},")
        lines.append(f"        .instrument_count = sizeof({instruments_name}) / sizeof({instruments_name}[0]),")
        lines.append(f"        .notes = {notes_name},")
        lines.append(f"        .note_count = sizeof({notes_name}) / sizeof({notes_name}[0]),")
        lines.append("    },")

    lines.append("};")
    lines.append("")
    lines.append(
        "const size_t PACKED_MUSIC_COUNT = "
        "sizeof(PACKED_MUSIC_DEFINITIONS) / sizeof(PACKED_MUSIC_DEFINITIONS[0]);"
    )

    return "\n".join(lines)


def emit_music_c(patterns: list[MusicPattern]) -> str:
    chunks: list[str] = []

    chunks.append("// Generated by tools/pack_music.py. Do not edit manually.")
    chunks.append('#include "generated_music.h"')
    chunks.append("")

    for pattern in patterns:
        chunks.append(emit_instruments(pattern))
        chunks.append("")
        chunks.append(emit_notes(pattern))
        chunks.append("")

    chunks.append(emit_music_registry(patterns))
    chunks.append("")

    return "\n".join(chunks)


def load_patterns(input_dir: Path) -> list[MusicPattern]:
    paths = sorted(input_dir.glob("*.music.json"))
    patterns = [parse_music_file(path) for path in paths]

    used_ids: set[str] = set()

    for pattern in patterns:
        if pattern.music_id in used_ids:
            raise ValueError(f"Duplicate music id: {pattern.music_id}")

        used_ids.add(pattern.music_id)

    patterns.sort(key=lambda item: item.music_id)
    return patterns


def convert_all(input_dir: Path, output_file: Path) -> None:
    if not input_dir.exists():
        raise FileNotFoundError(f"Input directory does not exist: {input_dir}")

    patterns = load_patterns(input_dir)

    if not patterns:
        print(f"No *.music.json files found in {input_dir}")
        return

    output_file.parent.mkdir(parents=True, exist_ok=True)
    output_file.write_text(emit_music_c(patterns), encoding="utf-8")

    print(f"Generated {output_file}")
    print(f"Packed {len(patterns)} music pattern(s)")


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Pack Little One music JSON files into one C source file."
    )

    parser.add_argument(
        "--input-dir",
        type=Path,
        default=DEFAULT_INPUT_DIR,
        help="Directory with *.music.json files.",
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
