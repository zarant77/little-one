#!/usr/bin/env python3

from __future__ import annotations

import argparse
import json
import re
from dataclasses import dataclass
from pathlib import Path
from typing import Any


DEFAULT_INPUT_DIR = Path("src/sprites/animations")
DEFAULT_OUTPUT_FILE = DEFAULT_INPUT_DIR / "animations.c"

PROPERTY_MAP = {
    "offset_x": "ANIM_PROP_OFFSET_X",
    "offset_y": "ANIM_PROP_OFFSET_Y",
    "scale_x": "ANIM_PROP_SCALE_X",
    "scale_y": "ANIM_PROP_SCALE_Y",
    "rotation": "ANIM_PROP_ROTATION",
    "alpha": "ANIM_PROP_ALPHA",
}

EASING_MAP = {
    "linear": "ANIM_EASE_LINEAR",
    "ease_in": "ANIM_EASE_IN",
    "ease_out": "ANIM_EASE_OUT",
    "ease_in_out": "ANIM_EASE_IN_OUT",
    "step": "ANIM_EASE_STEP",
}


@dataclass(frozen=True)
class AnimationKey:
    time_ms: int
    value: int
    easing: str


@dataclass(frozen=True)
class AnimationTrack:
    property_name: str
    keys: list[AnimationKey]


@dataclass(frozen=True)
class AnimationClip:
    animation_id: str
    duration_ms: int
    loop: bool
    tracks: list[AnimationTrack]
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


def as_bool(value: Any, label: str) -> bool:
    if not isinstance(value, bool):
        raise ValueError(f"{label} must be a boolean")
    return value


def validate_id(value: str, label: str) -> None:
    if not re.fullmatch(r"[a-z0-9_-]+", value):
        raise ValueError(f"{label} must match [a-z0-9_-]+")


def make_symbol(value: str) -> str:
    symbol = re.sub(r"[^A-Za-z0-9]+", "_", value).strip("_").upper()

    if not symbol:
        raise ValueError("Generated C symbol is empty")

    if symbol[0].isdigit():
        symbol = f"ANIM_{symbol}"

    return symbol


def parse_animation_file(path: Path) -> AnimationClip:
    data = as_dict(json.loads(path.read_text(encoding="utf-8")), str(path))

    version = as_int(data.get("version"), f"{path}: version")
    if version != 1:
        raise ValueError(f"{path}: version must be 1")

    animation_id = as_str(data.get("id"), f"{path}: id")
    validate_id(animation_id, f"{path}: id")

    duration_ms = as_int(data.get("durationMs"), f"{path}: durationMs")
    if duration_ms <= 0 or duration_ms > 32767:
        raise ValueError(f"{path}: durationMs must be in range 1..32767")

    loop = as_bool(data.get("loop"), f"{path}: loop")

    raw_tracks = as_list(data.get("tracks"), f"{path}: tracks")
    tracks: list[AnimationTrack] = []
    used_properties: set[str] = set()

    for track_index, raw_track in enumerate(raw_tracks):
        track_label = f"{path}: tracks[{track_index}]"
        track = as_dict(raw_track, track_label)

        property_name = as_str(track.get("property"), f"{track_label}.property")

        if property_name not in PROPERTY_MAP:
            raise ValueError(f"{track_label}: unsupported property '{property_name}'")

        if property_name in used_properties:
            raise ValueError(f"{track_label}: duplicate property '{property_name}'")

        used_properties.add(property_name)

        raw_keys = as_list(track.get("keys"), f"{track_label}.keys")
        keys: list[AnimationKey] = []
        used_times: set[int] = set()

        for key_index, raw_key in enumerate(raw_keys):
            key_label = f"{track_label}.keys[{key_index}]"
            key = as_dict(raw_key, key_label)

            time_ms = as_int(key.get("timeMs"), f"{key_label}.timeMs")
            value = as_int(key.get("value"), f"{key_label}.value")
            easing = as_str(key.get("easing"), f"{key_label}.easing")

            if easing not in EASING_MAP:
                raise ValueError(f"{key_label}: unsupported easing '{easing}'")

            if time_ms < 0 or time_ms > duration_ms:
                raise ValueError(
                    f"{key_label}: timeMs must be inside 0..{duration_ms}"
                )

            if time_ms in used_times:
                raise ValueError(
                    f"{key_label}: duplicate key time {time_ms} in property '{property_name}'"
                )

            if value < -32768 or value > 32767:
                raise ValueError(f"{key_label}: value must fit int16_t")

            used_times.add(time_ms)

            keys.append(
                AnimationKey(
                    time_ms=time_ms,
                    value=value,
                    easing=easing,
                )
            )

        keys.sort(key=lambda item: item.time_ms)

        tracks.append(
            AnimationTrack(
                property_name=property_name,
                keys=keys,
            )
        )

    return AnimationClip(
        animation_id=animation_id,
        duration_ms=duration_ms,
        loop=loop,
        tracks=tracks,
        source_path=path,
    )


def emit_keys(clip: AnimationClip, track: AnimationTrack) -> str:
    clip_symbol = make_symbol(clip.animation_id)
    property_symbol = make_symbol(track.property_name)
    keys_name = f"{clip_symbol}_{property_symbol}_KEYS"

    lines: list[str] = []
    lines.append(f"static const AnimKey {keys_name}[] = {{")

    for key in track.keys:
        lines.append(
            f"    {{ {key.time_ms}, {key.value}, {EASING_MAP[key.easing]} }},"
        )

    lines.append("};")

    return "\n".join(lines)


def emit_tracks(clip: AnimationClip) -> str:
    clip_symbol = make_symbol(clip.animation_id)
    tracks_name = f"{clip_symbol}_TRACKS"

    lines: list[str] = []
    lines.append(f"static const AnimTrack {tracks_name}[] = {{")

    for track in clip.tracks:
        property_symbol = make_symbol(track.property_name)
        keys_name = f"{clip_symbol}_{property_symbol}_KEYS"

        lines.append("    {")
        lines.append(f"        .property = {PROPERTY_MAP[track.property_name]},")
        lines.append(f"        .keys = {keys_name},")
        lines.append(
            f"        .key_count = sizeof({keys_name}) / sizeof({keys_name}[0]),"
        )
        lines.append("    },")

    lines.append("};")

    return "\n".join(lines)


def emit_clips(clips: list[AnimationClip]) -> str:
    lines: list[str] = []

    lines.append("const AnimationClip ANIMATION_CLIPS[] = {")

    for clip in clips:
        clip_symbol = make_symbol(clip.animation_id)
        tracks_name = f"{clip_symbol}_TRACKS"
        loop_value = "true" if clip.loop else "false"

        lines.append("    {")
        lines.append(f'        .id = "{clip.animation_id}",')
        lines.append(f"        .duration_ms = {clip.duration_ms},")
        lines.append(f"        .loop = {loop_value},")
        lines.append(f"        .tracks = {tracks_name},")
        lines.append(
            f"        .track_count = sizeof({tracks_name}) / sizeof({tracks_name}[0]),"
        )
        lines.append("    },")

    lines.append("};")
    lines.append("")
    lines.append(
        "const uint8_t ANIMATION_CLIP_COUNT = "
        "sizeof(ANIMATION_CLIPS) / sizeof(ANIMATION_CLIPS[0]);"
    )

    return "\n".join(lines)


def emit_animation_c(clips: list[AnimationClip]) -> str:
    chunks: list[str] = []

    chunks.append("// Generated by tools/pack_animations.py. Do not edit manually.")
    chunks.append('#include "animation_definition.h"')
    chunks.append("")

    for clip in clips:
        chunks.append(f"// Source: {clip.source_path.as_posix()}")

        for track in clip.tracks:
            chunks.append(emit_keys(clip, track))
            chunks.append("")

        chunks.append(emit_tracks(clip))
        chunks.append("")

    chunks.append(emit_clips(clips))
    chunks.append("")

    return "\n".join(chunks)


def collect_animation_files(input_dir: Path, output_file: Path) -> list[Path]:
    paths = sorted(input_dir.glob("*.anim.json"))

    output_stem = output_file.stem
    ignored_names = {
        f"{output_stem}.anim.json",
    }

    return [
        path
        for path in paths
        if path.name not in ignored_names
    ]


def load_clips(input_dir: Path, output_file: Path) -> list[AnimationClip]:
    paths = collect_animation_files(input_dir, output_file)

    clips = [parse_animation_file(path) for path in paths]

    used_ids: set[str] = set()

    for clip in clips:
        if clip.animation_id in used_ids:
            raise ValueError(f"Duplicate animation id: {clip.animation_id}")

        used_ids.add(clip.animation_id)

    clips.sort(key=lambda clip: clip.animation_id)

    return clips


def convert_all(input_dir: Path, output_file: Path) -> None:
    if not input_dir.exists():
        raise FileNotFoundError(f"Input directory does not exist: {input_dir}")

    clips = load_clips(input_dir, output_file)

    if not clips:
        print(f"No *.anim.json files found in {input_dir}")
        return

    output_file.parent.mkdir(parents=True, exist_ok=True)
    output_file.write_text(emit_animation_c(clips), encoding="utf-8")

    print(f"Generated {output_file}")
    print(f"Packed {len(clips)} animation(s)")


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Pack Little One animation JSON files into one C source file."
    )

    parser.add_argument(
        "--input-dir",
        type=Path,
        default=DEFAULT_INPUT_DIR,
        help="Directory with *.anim.json files.",
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