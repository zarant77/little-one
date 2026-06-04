#!/usr/bin/env python3

import argparse
import json
import math
import re
from collections import Counter, defaultdict
from dataclasses import dataclass
from pathlib import Path
from typing import Any, Dict, Iterable, List, Optional, Set, Tuple


PRESERVED_COLORS = {
    "050505ff",
    "080808ff",
    "ffffffff",
    "00000000",
    "00000022",
    "00000044",
    "00000055",
    "00000066",
}

COLOR_RE = re.compile(r"#?([0-9a-fA-F]{8})")


@dataclass
class Sprite:
    path: Path
    data: Dict[str, Any]
    colors: Counter[str]
    normalization_changes: int


@dataclass
class Skip:
    old_color: str
    new_color: str
    path: Optional[Path]
    uses: int
    reason: str


def parse_rgba(color: str) -> Tuple[int, int, int, int]:
    if not isinstance(color, str):
        raise ValueError("color must be a string")

    match = COLOR_RE.fullmatch(color)
    if match is None:
        raise ValueError(f"expected rrggbbaa hex, got {color!r}")

    value = match.group(1).lower()
    return tuple(int(value[index:index + 2], 16) for index in range(0, 8, 2))  # type: ignore[return-value]


def format_rgba(r: int, g: int, b: int, a: int) -> str:
    return f"{r:02x}{g:02x}{b:02x}{a:02x}"


def normalize_color(color: str) -> str:
    return format_rgba(*parse_rgba(color))


def rgb_distance(a: str, b: str) -> float:
    ar, ag, ab, _ = parse_rgba(a)
    br, bg, bb, _ = parse_rgba(b)
    return math.sqrt((ar - br) ** 2 + (ag - bg) ** 2 + (ab - bb) ** 2)


def luminance(rgb: Any) -> float:
    if isinstance(rgb, str):
        r, g, b, _ = parse_rgba(rgb)
    else:
        r, g, b = rgb
    return 0.2126 * r + 0.7152 * g + 0.0722 * b


def is_outline_like(color: str) -> bool:
    r, g, b, a = parse_rgba(color)
    return a == 0xff and max(r, g, b) <= 16 and max(r, g, b) - min(r, g, b) <= 2


def load_sprite(path: Path) -> Sprite:
    try:
        data = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as error:
        raise ValueError(f"{path}: could not load JSON: {error}") from error

    if not isinstance(data, dict):
        raise ValueError(f"{path}: root must be an object")

    primitives = data.get("primitives")
    if not isinstance(primitives, list):
        raise ValueError(f"{path}: primitives must be an array")

    colors: Counter[str] = Counter()
    normalization_changes = 0
    for index, primitive in enumerate(primitives):
        if not isinstance(primitive, dict):
            raise ValueError(f"{path}: primitive {index} must be an object")
        if "color" not in primitive:
            raise ValueError(f"{path}: primitive {index} has no color")
        try:
            normalized = normalize_color(primitive["color"])
            colors[normalized] += 1
            normalization_changes += primitive["color"] != normalized
        except ValueError as error:
            raise ValueError(f"{path}: primitive {index}: {error}") from error

    return Sprite(
        path=path,
        data=data,
        colors=colors,
        normalization_changes=normalization_changes,
    )


def collect_colors(
    sprites: Iterable[Sprite],
) -> Tuple[Counter[str], Counter[str], Dict[Path, Counter[str]]]:
    global_counts: Counter[str] = Counter()
    file_counts: Counter[str] = Counter()
    per_file: Dict[Path, Counter[str]] = {}

    for sprite in sprites:
        per_file[sprite.path] = sprite.colors.copy()
        global_counts.update(sprite.colors)
        file_counts.update(sprite.colors.keys())

    return global_counts, file_counts, per_file


def build_clusters(colors: Iterable[str], threshold: float) -> List[List[str]]:
    by_alpha: Dict[int, List[str]] = defaultdict(list)
    for color in colors:
        by_alpha[parse_rgba(color)[3]].append(color)

    clusters: List[List[str]] = []
    for alpha in sorted(by_alpha):
        alpha_clusters: List[List[str]] = []
        for color in sorted(by_alpha[alpha]):
            for cluster in alpha_clusters:
                # Complete-link grouping avoids long chains joining distant shades.
                if all(rgb_distance(color, member) <= threshold for member in cluster):
                    cluster.append(color)
                    break
            else:
                alpha_clusters.append([color])
        clusters.extend(alpha_clusters)

    return clusters


def choose_canonical(
    cluster: Iterable[str],
    global_counts: Counter[str],
    preserved_colors: Set[str],
    file_counts: Optional[Counter[str]] = None,
) -> str:
    members = list(cluster)
    protected = [color for color in members if color in preserved_colors]
    candidates = protected or members
    file_counts = file_counts or Counter()
    return min(
        candidates,
        key=lambda color: (-global_counts[color], -file_counts[color], color),
    )


def build_remap(
    clusters: Iterable[List[str]],
    global_counts: Counter[str],
    file_counts: Counter[str],
    preserved_colors: Set[str],
    skipped: Optional[List[Skip]] = None,
) -> Dict[str, str]:
    remap: Dict[str, str] = {}
    for cluster in clusters:
        canonical = choose_canonical(cluster, global_counts, preserved_colors, file_counts)
        for color in cluster:
            if color == canonical or color in preserved_colors:
                continue
            if is_outline_like(color) != is_outline_like(canonical):
                if skipped is not None:
                    skipped.append(
                        Skip(
                            color,
                            canonical,
                            None,
                            global_counts[color],
                            "outline protected",
                        )
                    )
            else:
                remap[color] = canonical
    return remap


def local_contrast_protected(
    old_color: str,
    new_color: str,
    colors: Iterable[str],
    remap: Dict[str, str],
    threshold: float,
) -> bool:
    for other in colors:
        if other == old_color:
            continue
        if (other == new_color or remap.get(other) == new_color) and (
            abs(luminance(old_color) - luminance(other)) > threshold
        ):
            return True
    return False


def proposed_sprite_remap(
    sprite: Sprite,
    remap: Dict[str, str],
    local_contrast_threshold: float,
    max_local_loss: float,
    force: bool,
) -> Tuple[Dict[str, str], List[Skip]]:
    applied: Dict[str, str] = {}
    skipped: List[Skip] = []

    for old_color in sorted(sprite.colors):
        new_color = remap.get(old_color)
        if new_color is None:
            continue
        if local_contrast_protected(
            old_color,
            new_color,
            sprite.colors,
            remap,
            local_contrast_threshold,
        ):
            skipped.append(
                Skip(
                    old_color,
                    new_color,
                    sprite.path,
                    sprite.colors[old_color],
                    "local contrast protected",
                )
            )
        else:
            applied[old_color] = new_color

    before = len(sprite.colors)
    after = len({applied.get(color, color) for color in sprite.colors})
    loss = (before - after) / before if before else 0.0
    if not force and loss > max_local_loss:
        skipped.extend(
            Skip(
                old_color,
                new_color,
                sprite.path,
                sprite.colors[old_color],
                "max local loss exceeded",
            )
            for old_color, new_color in sorted(applied.items())
        )
        applied.clear()

    return applied, skipped


def apply_remap_to_sprite(sprite: Sprite, remap: Dict[str, str]) -> int:
    changed = 0
    for primitive in sprite.data["primitives"]:
        old_value = primitive["color"]
        normalized = normalize_color(old_value)
        new_value = remap.get(normalized, normalized)
        if old_value != new_value:
            primitive["color"] = new_value
            changed += 1
    return changed


def aggregate_remaps(
    sprites: Iterable[Sprite],
    sprite_remaps: Dict[Path, Dict[str, str]],
) -> Dict[Tuple[str, str], Tuple[int, int]]:
    uses: Counter[Tuple[str, str]] = Counter()
    files: Counter[Tuple[str, str]] = Counter()
    for sprite in sprites:
        for old_color, new_color in sprite_remaps[sprite.path].items():
            key = (old_color, new_color)
            uses[key] += sprite.colors[old_color]
            files[key] += 1
    return {key: (uses[key], files[key]) for key in uses}


def make_report(
    sprites: List[Sprite],
    clusters: List[List[str]],
    global_counts: Counter[str],
    file_counts: Counter[str],
    sprite_remaps: Dict[Path, Dict[str, str]],
    skipped: List[Skip],
    preserved_colors: Set[str],
) -> str:
    aggregate = aggregate_remaps(sprites, sprite_remaps)
    final_colors = {
        sprite_remaps[sprite.path].get(color, color)
        for sprite in sprites
        for color in sprite.colors
    }
    changed_files = sum(
        bool(sprite_remaps[sprite.path]) or bool(sprite.normalization_changes)
        for sprite in sprites
    )
    lines = [
        f"files scanned: {len(sprites)}",
        f"files changed: {changed_files}",
        f"total unique colors before: {len(global_counts)}",
        f"total unique colors after proposed remap: {len(final_colors)}",
        "",
        "top 20 largest color clusters:",
    ]

    largest = sorted(
        (cluster for cluster in clusters if len(cluster) > 1),
        key=lambda cluster: (-len(cluster), -sum(global_counts[color] for color in cluster), cluster),
    )[:20]
    if largest:
        for cluster in largest:
            canonical = choose_canonical(cluster, global_counts, preserved_colors, file_counts)
            lines.append(
                f"{len(cluster):3} colors | {sum(global_counts[color] for color in cluster):5} uses"
                f" | canonical: {canonical} | {', '.join(cluster)}"
            )
    else:
        lines.append("(none)")

    lines.extend(["", "per-file color count before/after:"])
    for sprite in sprites:
        before = len(sprite.colors)
        after = len({sprite_remaps[sprite.path].get(color, color) for color in sprite.colors})
        marker = " changed" if sprite_remaps[sprite.path] or sprite.normalization_changes else ""
        normalization = (
            f" | normalized values: {sprite.normalization_changes}"
            if sprite.normalization_changes
            else ""
        )
        lines.append(f"{sprite.path}: {before} -> {after}{marker}{normalization}")

    lines.extend(["", "remaps:"])
    if aggregate:
        for (old_color, new_color), (uses, files) in sorted(aggregate.items()):
            lines.append(f"{old_color} -> {new_color} | uses: {uses} | files: {files}")
    else:
        lines.append("(none)")

    lines.extend(["", "skipped remaps:"])
    for color in sorted(global_counts):
        if color in preserved_colors:
            lines.append(
                f"{color} -> {color} | uses: {global_counts[color]} | files: {file_counts[color]}"
                " | reason: preserved color"
            )
    for item in sorted(
        skipped,
        key=lambda item: (str(item.path or ""), item.old_color, item.new_color, item.reason),
    ):
        location = f" | file: {item.path}" if item.path is not None else ""
        lines.append(
            f"{item.old_color} -> {item.new_color} | uses: {item.uses}{location}"
            f" | reason: {item.reason}"
        )

    alpha_groups = len({parse_rgba(color)[3] for color in global_counts})
    lines.append(
        f"different alpha | alpha groups kept separate: {alpha_groups}"
        " | reason: different alpha"
    )
    return "\n".join(lines) + "\n"


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Unify near-duplicate colors in .sprite.json files.",
    )
    parser.add_argument("path", type=Path, help="Sprite JSON file or directory to scan")
    mode = parser.add_mutually_exclusive_group()
    mode.add_argument("--dry-run", action="store_true", help="Preview changes (default)")
    mode.add_argument("--write", action="store_true", help="Update sprite JSON files in place")
    parser.add_argument("--report", type=Path, help="Write the full diagnostic report to a file")
    parser.add_argument("--threshold", type=float, default=12.0, help="Maximum RGB cluster distance")
    parser.add_argument(
        "--local-contrast-threshold",
        type=float,
        default=8.0,
        help="Maximum local luminance difference allowed to collapse",
    )
    parser.add_argument(
        "--max-local-loss",
        type=float,
        default=0.30,
        help="Maximum fraction of unique colors a sprite may lose",
    )
    parser.add_argument("--force", action="store_true", help="Allow sprites to exceed max local loss")
    args = parser.parse_args()

    if args.threshold < 0 or args.local_contrast_threshold < 0:
        parser.error("thresholds must be non-negative")
    if not 0 <= args.max_local_loss <= 1:
        parser.error("--max-local-loss must be between 0 and 1")
    return args


def find_sprite_files(path: Path) -> List[Path]:
    if path.is_file():
        if not path.name.endswith(".sprite.json"):
            raise ValueError(f"{path}: expected a .sprite.json file")
        return [path]
    if not path.is_dir():
        raise ValueError(f"{path}: path does not exist or is not a directory")
    return sorted(path.rglob("*.sprite.json"))


def main() -> int:
    args = parse_args()
    try:
        files = find_sprite_files(args.path)
        sprites = [load_sprite(path) for path in files]
    except ValueError as error:
        print(f"[ERROR] {error}")
        return 1

    global_counts, file_counts, _ = collect_colors(sprites)
    protected_colors = PRESERVED_COLORS | {
        color for color in global_counts if is_outline_like(color)
    }
    clusters = build_clusters(global_counts, args.threshold)
    skipped: List[Skip] = []
    global_remap = build_remap(
        clusters,
        global_counts,
        file_counts,
        protected_colors,
        skipped,
    )

    sprite_remaps: Dict[Path, Dict[str, str]] = {}
    for sprite in sprites:
        sprite_remaps[sprite.path], sprite_skips = proposed_sprite_remap(
            sprite,
            global_remap,
            args.local_contrast_threshold,
            args.max_local_loss,
            args.force,
        )
        skipped.extend(sprite_skips)

    report = make_report(
        sprites,
        clusters,
        global_counts,
        file_counts,
        sprite_remaps,
        skipped,
        protected_colors,
    )
    print(report, end="")

    if args.report:
        args.report.write_text(report, encoding="utf-8")

    if args.write:
        for sprite in sprites:
            changed = apply_remap_to_sprite(sprite, sprite_remaps[sprite.path])
            if changed:
                sprite.path.write_text(
                    json.dumps(sprite.data, ensure_ascii=False, indent=2) + "\n",
                    encoding="utf-8",
                )
        print("Write mode complete.")
    else:
        print("Dry-run only. Pass --write to update sprite files.")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
