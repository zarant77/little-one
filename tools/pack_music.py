#!/usr/bin/env python3

from __future__ import annotations

import argparse
import json
import math
import re
import sys
from dataclasses import dataclass, replace
from pathlib import Path
from typing import Any


DEFAULT_INPUT_DIR = Path("src/assets/music")
DEFAULT_OUTPUT_FILE = Path("src/audio/generated_music.c")

WAVE_MAP = {
    "square": "SOUND_WAVE_SQUARE",
    "sine": "SOUND_WAVE_SINE",
    "triangle": "SOUND_WAVE_TRIANGLE",
    "noise": "SOUND_WAVE_NOISE",
}

INSTRUMENT_FLAG_BASS = 1
INSTRUMENT_FLAG_LEAD = 2
INSTRUMENT_FLAG_PAD = 4
INSTRUMENT_FLAG_SPARK = 8

NOTE_VOLUME_MIN = 0
NOTE_VOLUME_MAX = 100

DEFAULT_TRACK_VOLUME = 1.0
DEFAULT_NORMALIZE_VOLUME = False
DEFAULT_TARGET_AVERAGE_VOLUME = 80.0
DEFAULT_MAX_VOLUME_GAIN = 3.0

TRACK_VOLUME_MIN = 0.0
TRACK_VOLUME_MAX = 1.0

MAX_VOLUME_GAIN_MIN = 1.0
MAX_VOLUME_GAIN_MAX = 16.0


@dataclass(frozen=True)
class MusicInstrument:
    instrument_id: str
    wave: str
    volume: int
    attack_ms: int
    decay_ms: int
    sustain: int
    release_ms: int
    flags: int


@dataclass(frozen=True)
class MusicLoop:
    enabled: bool
    start_tick: int
    end_tick: int


@dataclass(frozen=True)
class MusicLoudness:
    volume: float
    normalize_volume: bool
    target_average_volume: float
    max_volume_gain: float


@dataclass(frozen=True)
class MusicNote:
    instrument: int
    note: int
    start_tick: int
    duration_ticks: int
    volume: int


@dataclass(frozen=True)
class PackedStats:
    original_note_bytes: int
    packed_bytes: int
    note_count: int
    average_volume_before: float
    average_volume_after: float
    normalization_gain: float
    track_volume: float
    normalize_volume: bool


@dataclass(frozen=True)
class PackedMusic:
    music_id: str
    symbol: str
    bpm: int
    ticks_per_beat: int
    length_ticks: int
    loop: MusicLoop
    loudness: MusicLoudness
    instruments: list[MusicInstrument]
    notes: list[MusicNote]
    source_path: Path
    stats: PackedStats


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
    if not isinstance(value, int) or isinstance(value, bool):
        raise ValueError(f"{label} must be an integer")
    return value


def as_number(value: Any, label: str) -> float:
    if isinstance(value, bool) or not isinstance(value, (int, float)):
        raise ValueError(f"{label} must be a number")

    number = float(value)

    if not math.isfinite(number):
        raise ValueError(f"{label} must be finite")

    return number


def as_bool(value: Any, label: str) -> bool:
    if not isinstance(value, bool):
        raise ValueError(f"{label} must be a boolean")
    return value


def optional_int(data: dict[str, Any], key: str, default: int, label: str) -> int:
    return as_int(data.get(key, default), f"{label}.{key}")


def optional_number(data: dict[str, Any], key: str, default: float, label: str) -> float:
    return as_number(data.get(key, default), f"{label}.{key}")


def optional_bool(data: dict[str, Any], key: str, default: bool, label: str) -> bool:
    return as_bool(data.get(key, default), f"{label}.{key}")


def validate_id(value: str, label: str) -> None:
    if not re.fullmatch(r"[a-z0-9_-]+", value):
        raise ValueError(f"{label} must match [a-z0-9_-]+")


def validate_u8(value: int, label: str) -> None:
    if value < 0 or value > 255:
        raise ValueError(f"{label} must be in range 0..255")


def validate_u16(value: int, label: str) -> None:
    if value < 0 or value > 65535:
        raise ValueError(f"{label} must be in range 0..65535")


def clamp_int(value: int, minimum: int, maximum: int) -> int:
    return min(maximum, max(minimum, value))


def clamp_float(value: float, minimum: float, maximum: float) -> float:
    return min(maximum, max(minimum, value))


def infer_instrument_flags(instrument: MusicInstrument, average_note: float | None) -> int:
    instrument_id = instrument.instrument_id.lower()

    if "bass" in instrument_id:
        return INSTRUMENT_FLAG_BASS
    if "lead" in instrument_id:
        return INSTRUMENT_FLAG_LEAD
    if "harmony" in instrument_id or "pad" in instrument_id:
        return INSTRUMENT_FLAG_PAD
    if "spark" in instrument_id:
        return INSTRUMENT_FLAG_SPARK
    if average_note is not None and average_note <= 45:
        return INSTRUMENT_FLAG_BASS
    if average_note is not None and average_note >= 78:
        return INSTRUMENT_FLAG_SPARK
    if average_note is not None and average_note < 52:
        return INSTRUMENT_FLAG_BASS

    return INSTRUMENT_FLAG_LEAD


def assign_instrument_flags(instruments: list[MusicInstrument], notes: list[MusicNote]) -> list[MusicInstrument]:
    note_totals = [0] * len(instruments)
    note_counts = [0] * len(instruments)

    for note in notes:
        note_totals[note.instrument] += note.note
        note_counts[note.instrument] += 1

    flagged: list[MusicInstrument] = []

    for index, instrument in enumerate(instruments):
        average_note = None if note_counts[index] == 0 else note_totals[index] / note_counts[index]
        flagged.append(replace(instrument, flags=infer_instrument_flags(instrument, average_note)))

    return flagged


def make_symbol(value: str) -> str:
    symbol = re.sub(r"[^A-Za-z0-9]+", "_", value).strip("_").upper()

    if not symbol:
        raise ValueError("Generated C symbol is empty")

    if symbol[0].isdigit():
        symbol = f"MUSIC_{symbol}"

    return symbol


def music_id_symbol(value: str) -> str:
    return f"MUSIC_ID_{make_symbol(value)}"


def c_array_count(name: str) -> str:
    return f"sizeof({name}) / sizeof({name}[0])"


def parse_loop(data: dict[str, Any], path: Path, length_ticks: int) -> MusicLoop:
    raw_loop = data.get("loop")

    if raw_loop is None:
        return MusicLoop(enabled=False, start_tick=0, end_tick=0)

    label = f"{path}: loop"
    loop = as_dict(raw_loop, label)
    enabled = as_bool(loop.get("enabled", False), f"{label}.enabled")
    start_tick = optional_int(loop, "startTick", 0, label)
    end_tick = optional_int(loop, "endTick", 0, label)

    validate_u16(start_tick, f"{label}.startTick")
    validate_u16(end_tick, f"{label}.endTick")

    if not enabled:
        return MusicLoop(enabled=False, start_tick=0, end_tick=0)

    if end_tick <= start_tick:
        raise ValueError(f"{label}: endTick must be greater than startTick")

    if end_tick > length_ticks:
        raise ValueError(f"{label}: endTick must be <= lengthTicks")

    return MusicLoop(enabled=True, start_tick=start_tick, end_tick=end_tick)


def parse_loudness(data: dict[str, Any], path: Path) -> MusicLoudness:
    label = f"{path}: loudness"

    volume = optional_number(data, "volume", DEFAULT_TRACK_VOLUME, label)
    normalize_volume = optional_bool(data, "normalizeVolume", DEFAULT_NORMALIZE_VOLUME, label)
    target_average_volume = optional_number(data, "targetAverageVolume", DEFAULT_TARGET_AVERAGE_VOLUME, label)
    max_volume_gain = optional_number(data, "maxVolumeGain", DEFAULT_MAX_VOLUME_GAIN, label)

    volume = clamp_float(volume, TRACK_VOLUME_MIN, TRACK_VOLUME_MAX)
    target_average_volume = clamp_float(target_average_volume, NOTE_VOLUME_MIN, NOTE_VOLUME_MAX)
    max_volume_gain = clamp_float(max_volume_gain, MAX_VOLUME_GAIN_MIN, MAX_VOLUME_GAIN_MAX)

    return MusicLoudness(
        volume=volume,
        normalize_volume=normalize_volume,
        target_average_volume=target_average_volume,
        max_volume_gain=max_volume_gain,
    )


def calculate_average_note_volume(notes: list[MusicNote]) -> float:
    volumes = [note.volume for note in notes if note.volume > 0]

    if not volumes:
        return 0.0

    return sum(volumes) / len(volumes)


def calculate_normalization_gain(notes: list[MusicNote], loudness: MusicLoudness) -> float:
    if not loudness.normalize_volume:
        return 1.0

    average_volume = calculate_average_note_volume(notes)

    if average_volume <= 0:
        return 1.0

    gain = loudness.target_average_volume / average_volume

    return clamp_float(gain, 1.0, loudness.max_volume_gain)


def apply_loudness_to_notes(notes: list[MusicNote], loudness: MusicLoudness) -> tuple[list[MusicNote], float, float, float]:
    average_before = calculate_average_note_volume(notes)
    normalization_gain = calculate_normalization_gain(notes, loudness)

    effective_notes: list[MusicNote] = []

    for note in notes:
        effective_volume = round(note.volume * normalization_gain)
        effective_volume = clamp_int(effective_volume, NOTE_VOLUME_MIN, NOTE_VOLUME_MAX)

        effective_notes.append(
            replace(
                note,
                volume=effective_volume,
            )
        )

    average_after = calculate_average_note_volume(effective_notes)

    return effective_notes, average_before, average_after, normalization_gain


def parse_music_file(
    path: Path,
) -> tuple[str, int, int, int, MusicLoop, MusicLoudness, list[MusicInstrument], list[MusicNote]]:
    data = as_dict(json.loads(path.read_text(encoding="utf-8")), str(path))

    file_type = as_str(data.get("type"), f"{path}: type")

    if file_type != "music":
        raise ValueError(f"{path}: type must be 'music'")

    music_id = as_str(data.get("id"), f"{path}: id")
    validate_id(music_id, f"{path}: id")

    bpm = as_int(data.get("bpm"), f"{path}: bpm")

    if bpm <= 0 or bpm > 65535:
        raise ValueError(f"{path}: bpm must be in range 1..65535")

    ticks_per_beat = as_int(data.get("ticksPerBeat"), f"{path}: ticksPerBeat")

    if ticks_per_beat <= 0 or ticks_per_beat > 65535:
        raise ValueError(f"{path}: ticksPerBeat must be in range 1..65535")

    length_ticks = as_int(data.get("lengthTicks"), f"{path}: lengthTicks")

    if length_ticks <= 0 or length_ticks > 65535:
        raise ValueError(f"{path}: lengthTicks must be in range 1..65535")

    loudness = parse_loudness(data, path)
    loop = parse_loop(data, path, length_ticks)

    raw_instruments = as_list(data.get("instruments"), f"{path}: instruments")

    if not raw_instruments:
        raise ValueError(f"{path}: instruments must not be empty")

    instruments: list[MusicInstrument] = []

    for index, raw_instrument in enumerate(raw_instruments):
        label = f"{path}: instruments[{index}]"
        instrument = as_dict(raw_instrument, label)

        instrument_id = as_str(instrument.get("id"), f"{label}.id")
        validate_id(instrument_id, f"{label}.id")

        wave = as_str(instrument.get("wave"), f"{label}.wave")

        if wave not in WAVE_MAP:
            raise ValueError(f"{label}: unsupported wave '{wave}'")

        volume = as_int(instrument.get("volume"), f"{label}.volume")
        attack_ms = optional_int(instrument, "attackMs", 0, label)
        decay_ms = optional_int(instrument, "decayMs", 0, label)
        sustain = optional_int(instrument, "sustain", 255, label)
        release_ms = optional_int(instrument, "releaseMs", 0, label)

        if volume < 0 or volume > 100:
            raise ValueError(f"{label}.volume must be in range 0..100")

        validate_u8(attack_ms, f"{label}.attackMs")
        validate_u8(decay_ms, f"{label}.decayMs")
        validate_u8(sustain, f"{label}.sustain")
        validate_u16(release_ms, f"{label}.releaseMs")

        instruments.append(
            MusicInstrument(
                instrument_id=instrument_id,
                wave=wave,
                volume=volume,
                attack_ms=attack_ms,
                decay_ms=decay_ms,
                sustain=sustain,
                release_ms=release_ms,
                flags=0,
            )
        )

    if len(instruments) > 255:
        raise ValueError(f"{path}: instrument count must be <= 255")

    raw_notes = as_list(data.get("notes"), f"{path}: notes")
    raw_note_dicts = [as_dict(item, f"{path}: notes[{index}]") for index, item in enumerate(raw_notes)]
    notes: list[MusicNote] = []

    for index, note in enumerate(raw_note_dicts):
        label = f"{path}: notes[{index}]"

        instrument = as_int(note.get("instrument"), f"{label}.instrument")
        midi_note = as_int(note.get("note"), f"{label}.note")
        start_tick = as_int(note.get("startTick"), f"{label}.startTick")
        duration_ticks = as_int(note.get("durationTicks"), f"{label}.durationTicks")
        volume = as_int(note.get("volume"), f"{label}.volume")

        if instrument < 0 or instrument >= len(instruments):
            raise ValueError(f"{label}: instrument index is out of range")

        if midi_note < 0 or midi_note > 127:
            raise ValueError(f"{label}: note must be in range 0..127")

        validate_u16(start_tick, f"{label}.startTick")
        validate_u16(duration_ticks, f"{label}.durationTicks")

        if volume < NOTE_VOLUME_MIN or volume > NOTE_VOLUME_MAX:
            raise ValueError(f"{label}.volume must be in range {NOTE_VOLUME_MIN}..{NOTE_VOLUME_MAX}")

        if duration_ticks == 0:
            raise ValueError(f"{label}: durationTicks must be > 0")

        if start_tick >= length_ticks:
            raise ValueError(f"{label}: startTick must be < lengthTicks")

        note_end_tick = start_tick + duration_ticks

        if note_end_tick > length_ticks:
            raise ValueError(
                f"{label}: note crosses music length: "
                f"startTick={start_tick} durationTicks={duration_ticks} lengthTicks={length_ticks}"
            )

        notes.append(
            MusicNote(
                instrument=instrument,
                note=midi_note,
                start_tick=start_tick,
                duration_ticks=duration_ticks,
                volume=volume,
            )
        )

    notes.sort(key=lambda item: (item.start_tick, item.instrument, item.note, item.duration_ticks, item.volume))
    instruments = assign_instrument_flags(instruments, notes)

    return music_id, bpm, ticks_per_beat, length_ticks, loop, loudness, instruments, notes


def pack_music(path: Path) -> PackedMusic:
    music_id, bpm, ticks_per_beat, length_ticks, loop, loudness, instruments, source_notes = parse_music_file(path)
    symbol = make_symbol(music_id)

    if len(source_notes) > 65535:
        raise ValueError(f"{path}: note count must be <= 65535")

    effective_notes, average_before, average_after, normalization_gain = apply_loudness_to_notes(source_notes, loudness)

    original_note_bytes = len(effective_notes) * 7
    instrument_bytes = len(instruments) * 5
    note_bytes = len(effective_notes) * 7
    definition_estimate = 64
    packed_bytes = instrument_bytes + note_bytes + definition_estimate

    return PackedMusic(
        music_id=music_id,
        symbol=symbol,
        bpm=bpm,
        ticks_per_beat=ticks_per_beat,
        length_ticks=length_ticks,
        loop=loop,
        loudness=loudness,
        instruments=instruments,
        notes=effective_notes,
        source_path=path,
        stats=PackedStats(
            original_note_bytes=original_note_bytes,
            packed_bytes=packed_bytes,
            note_count=len(effective_notes),
            average_volume_before=average_before,
            average_volume_after=average_after,
            normalization_gain=normalization_gain,
            track_volume=loudness.volume,
            normalize_volume=loudness.normalize_volume,
        ),
    )


def emit_generated_header() -> str:
    return """// Generated by tools/pack_music.py. Do not edit manually.
#include "generated_music.h"
#include <stddef.h>
#include <stdint.h>

"""


def emit_instruments(pattern: PackedMusic) -> str:
    name = f"{pattern.symbol}_MUSIC_INSTRUMENTS"
    lines = [
        f"// Source: {pattern.source_path.as_posix()}",
        f"static const PackedMusicInstrument {name}[] = {{",
    ]

    for instrument in pattern.instruments:
        lines.extend(
            [
                "    {",
                f"        .wave = {WAVE_MAP[instrument.wave]},",
                f"        .volume = {instrument.volume},",
                f"        .attack_ms = {instrument.attack_ms},",
                f"        .decay_ms = {instrument.decay_ms},",
                f"        .flags = {instrument.flags},",
                "    },",
            ]
        )

    lines.append("};")

    return "\n".join(lines)


def emit_notes(pattern: PackedMusic) -> str:
    name = f"{pattern.symbol}_MUSIC_NOTES"
    lines = [f"static const PackedMusicNote {name}[] = {{"]

    for note in pattern.notes:
        lines.append(
            f"    {{ {note.instrument}, {note.note}, {note.volume}, "
            f"{note.start_tick}, {note.duration_ticks} }},"
        )

    lines.append("};")

    return "\n".join(lines)


def emit_pattern(pattern: PackedMusic) -> str:
    chunks = [
        emit_instruments(pattern),
        "",
        emit_notes(pattern),
        "",
    ]

    return "\n".join(chunks)


def emit_registry(patterns: list[PackedMusic]) -> str:
    lines = ["const PackedMusicDefinition PACKED_MUSIC_DEFINITIONS[] = {"]

    for pattern in patterns:
        instruments_name = f"{pattern.symbol}_MUSIC_INSTRUMENTS"
        notes_name = f"{pattern.symbol}_MUSIC_NOTES"
        loop_enabled = 1 if pattern.loop.enabled else 0
        track_volume = clamp_int(round(pattern.loudness.volume * 100), 0, 100)

        lines.extend(
            [
                "    {",
                f"        .id = {music_id_symbol(pattern.music_id)},",
                f"        .bpm = {pattern.bpm},",
                f"        .ticks_per_beat = {pattern.ticks_per_beat},",
                f"        .length_ticks = {pattern.length_ticks},",
                f"        .volume = {track_volume},",
                "        .loop = {",
                f"            .enabled = {loop_enabled},",
                f"            .start_tick = {pattern.loop.start_tick},",
                f"            .end_tick = {pattern.loop.end_tick},",
                "        },",
                f"        .instruments = {instruments_name},",
                f"        .instrument_count = {c_array_count(instruments_name)},",
                f"        .notes = {notes_name},",
                f"        .note_count = {c_array_count(notes_name)},",
                "    },",
            ]
        )

    lines.append("};")
    lines.append("")
    lines.append("const size_t PACKED_MUSIC_COUNT = sizeof(PACKED_MUSIC_DEFINITIONS) / sizeof(PACKED_MUSIC_DEFINITIONS[0]);")
    lines.append("")

    return "\n".join(lines)


def emit_music_c(patterns: list[PackedMusic]) -> str:
    chunks = [emit_generated_header()]

    for pattern in patterns:
        chunks.append(emit_pattern(pattern))

    chunks.append(emit_registry(patterns))

    return "\n".join(chunks)


def load_patterns(input_dir: Path) -> list[PackedMusic]:
    paths = sorted(input_dir.glob("*.music.json"))
    patterns = [pack_music(path) for path in paths]
    used_ids: set[str] = set()

    for pattern in patterns:
        if pattern.music_id in used_ids:
            raise ValueError(f"Duplicate music id: {pattern.music_id}")

        used_ids.add(pattern.music_id)

    patterns.sort(key=lambda item: item.music_id)

    return patterns


def print_stats(patterns: list[PackedMusic]) -> None:
    total_old = 0
    total_new = 0

    for pattern in patterns:
        total_old += pattern.stats.original_note_bytes
        total_new += pattern.stats.packed_bytes
        saved = pattern.stats.original_note_bytes - pattern.stats.packed_bytes
        ratio = (pattern.stats.packed_bytes / pattern.stats.original_note_bytes) if pattern.stats.original_note_bytes else 0

        print(
            f"{pattern.music_id}: notes={pattern.stats.note_count}, "
            f"avgVol={pattern.stats.average_volume_before:.1f}->{pattern.stats.average_volume_after:.1f}, "
            f"normalize={str(pattern.stats.normalize_volume).lower()}, "
            f"gain={pattern.stats.normalization_gain:.2f}, "
            f"volume={pattern.stats.track_volume:.2f}, "
            f"old_notes={pattern.stats.original_note_bytes} B, "
            f"packed≈{pattern.stats.packed_bytes} B, "
            f"saved≈{saved} B, ratio≈{ratio:.2f}"
        )

    if total_old:
        print(f"Total: old_notes={total_old} B, packed≈{total_new} B, ratio≈{total_new / total_old:.2f}")


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
    print_stats(patterns)


def main() -> None:
    parser = argparse.ArgumentParser(description="Pack Little One music JSON files into compressed C source.")
    parser.add_argument("--input-dir", type=Path, default=DEFAULT_INPUT_DIR, help="Directory with *.music.json files.")
    parser.add_argument("--output", type=Path, default=DEFAULT_OUTPUT_FILE, help="Generated C output file.")
    args = parser.parse_args()

    try:
        convert_all(args.input_dir, args.output)
    except (OSError, json.JSONDecodeError, ValueError) as error:
        print(f"Error: {error}", file=sys.stderr)
        raise SystemExit(1) from None


if __name__ == "__main__":
    main()
