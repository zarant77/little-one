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
    "00000000",
    "00000022",
    "00000044",
    "00000055",
    "00000066",
    "000000ff",
    "050505ff",
    "080808ff",
    "ffffffff",
}

COLOR_RE = re.compile(r"#?([0-9a-fA-F]{8})")


@dataclass(frozen=True)
class Skip:
    old_color: str
    new_color: str
    path: Optional[Path]
    uses: int
    reason: str


@dataclass
class Sprite:
    path: Path
    data: Dict[str, Any]
    colors: Counter[str]
    normalization_changes: int


@dataclass(frozen=True)
class PaletteState:
    colors: Set[str]
    counts: Counter[str]
    file_counts: Counter[str]


@dataclass(frozen=True)
class MergeCandidate:
    source: str
    target: str
    source_uses: int
    distance: float
    low_usage: bool


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


def luminance(color: str) -> float:
    r, g, b, _ = parse_rgba(color)
    return 0.2126 * r + 0.7152 * g + 0.0722 * b


def is_outline_like(color: str) -> bool:
    r, g, b, a = parse_rgba(color)
    return a == 0xff and max(r, g, b) <= 16 and max(r, g, b) - min(r, g, b) <= 2


def is_fully_transparent(color: str) -> bool:
    return parse_rgba(color)[3] == 0


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
        except ValueError as error:
            raise ValueError(f"{path}: primitive {index}: {error}") from error

        colors[normalized] += 1
        normalization_changes += primitive["color"] != normalized

    return Sprite(
        path=path,
        data=data,
        colors=colors,
        normalization_changes=normalization_changes,
    )


def find_sprite_files(path: Path) -> List[Path]:
    if path.is_file():
        if not path.name.endswith(".sprite.json"):
            raise ValueError(f"{path}: expected a .sprite.json file")
        return [path]

    if not path.is_dir():
        raise ValueError(f"{path}: path does not exist or is not a directory")

    return sorted(path.rglob("*.sprite.json"))


def collect_colors(sprites: Iterable[Sprite]) -> Tuple[Counter[str], Counter[str]]:
    global_counts: Counter[str] = Counter()
    file_counts: Counter[str] = Counter()

    for sprite in sprites:
        global_counts.update(sprite.colors)
        file_counts.update(sprite.colors.keys())

    return global_counts, file_counts


def build_protected_colors(global_counts: Counter[str]) -> Set[str]:
    return {
        color
        for color in global_counts
        if color in PRESERVED_COLORS or is_outline_like(color) or is_fully_transparent(color)
    }


def build_clusters(colors: Iterable[str], threshold: float) -> List[List[str]]:
    by_alpha: Dict[int, List[str]] = defaultdict(list)
    for color in colors:
        by_alpha[parse_rgba(color)[3]].append(color)

    clusters: List[List[str]] = []
    for alpha in sorted(by_alpha):
        alpha_clusters: List[List[str]] = []
        for color in sorted(by_alpha[alpha]):
            for cluster in alpha_clusters:
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
    file_counts: Counter[str],
    protected_colors: Set[str],
) -> str:
    members = list(cluster)
    protected_members = [color for color in members if color in protected_colors]
    candidates = protected_members or members
    return min(
        candidates,
        key=lambda color: (-global_counts[color], -file_counts[color], color),
    )


def build_conservative_global_remap(
    clusters: Iterable[List[str]],
    global_counts: Counter[str],
    file_counts: Counter[str],
    protected_colors: Set[str],
    skipped: List[Skip],
) -> Dict[str, str]:
    remap: Dict[str, str] = {}

    for cluster in clusters:
        canonical = choose_canonical(cluster, global_counts, file_counts, protected_colors)
        for color in sorted(cluster):
            if color == canonical or color in protected_colors:
                continue

            if parse_rgba(color)[3] != parse_rgba(canonical)[3]:
                skipped.append(Skip(color, canonical, None, global_counts[color], "different alpha"))
                continue

            if is_outline_like(color) != is_outline_like(canonical):
                skipped.append(Skip(color, canonical, None, global_counts[color], "outline protected"))
                continue

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
        if other == new_color or remap.get(other, other) == new_color:
            if abs(luminance(old_color) - luminance(other)) > threshold:
                return True
    return False


def proposed_sprite_remap(
    sprite: Sprite,
    global_remap: Dict[str, str],
    local_contrast_threshold: float,
    max_local_loss: float,
    force: bool,
) -> Tuple[Dict[str, str], List[Skip]]:
    applied: Dict[str, str] = {}
    skipped: List[Skip] = []

    for old_color in sorted(sprite.colors):
        new_color = global_remap.get(old_color)
        if new_color is None:
            continue

        if local_contrast_protected(
            old_color,
            new_color,
            sprite.colors,
            global_remap,
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
            continue

        applied[old_color] = new_color

    before = len(sprite.colors)
    after = len({applied.get(color, color) for color in sprite.colors})
    loss = (before - after) / before if before else 0.0

    if not force and loss > max_local_loss:
        skipped.extend(
            Skip(old_color, new_color, sprite.path, sprite.colors[old_color], "max local loss exceeded")
            for old_color, new_color in sorted(applied.items())
        )
        applied.clear()

    return applied, skipped


def effective_color(color: str, sprite_remap: Dict[str, str]) -> str:
    current = color
    seen: Set[str] = set()
    while current in sprite_remap and current not in seen:
        seen.add(current)
        current = sprite_remap[current]
    return current


def build_palette_state(sprites: Iterable[Sprite], sprite_remaps: Dict[Path, Dict[str, str]]) -> PaletteState:
    colors: Set[str] = set()
    counts: Counter[str] = Counter()
    file_counts: Counter[str] = Counter()

    for sprite in sprites:
        remap = sprite_remaps[sprite.path]
        file_colors: Set[str] = set()
        for color, uses in sprite.colors.items():
            mapped = effective_color(color, remap)
            colors.add(mapped)
            counts[mapped] += uses
            file_colors.add(mapped)
        file_counts.update(file_colors)

    return PaletteState(colors=colors, counts=counts, file_counts=file_counts)


def nearest_target(
    source: str,
    colors: Iterable[str],
    protected_colors: Set[str],
    max_distance: float,
) -> Optional[Tuple[str, float]]:
    source_alpha = parse_rgba(source)[3]
    best: Optional[Tuple[str, float]] = None

    for target in sorted(colors):
        if target == source:
            continue
        if parse_rgba(target)[3] != source_alpha:
            continue
        if is_outline_like(source) != is_outline_like(target):
            continue

        distance = rgb_distance(source, target)
        if distance > max_distance:
            continue

        score = (distance, 0 if target in protected_colors else 1, target)
        if best is None:
            best = (target, distance)
            best_score = score
        else:
            best_score = (best[1], 0 if best[0] in protected_colors else 1, best[0])
            if score < best_score:
                best = (target, distance)

    return best


def build_forced_candidates(
    state: PaletteState,
    protected_colors: Set[str],
    max_uses: int,
    force_max_distance: float,
    force_low_usage: bool,
) -> List[MergeCandidate]:
    candidates: List[MergeCandidate] = []

    for source in sorted(state.colors):
        if source in protected_colors:
            continue

        source_uses = state.counts[source]
        low_usage = source_uses <= max_uses
        if force_low_usage and not low_usage:
            continue

        target = nearest_target(source, state.colors, protected_colors, force_max_distance)
        if target is None:
            continue

        target_color, distance = target
        candidates.append(
            MergeCandidate(
                source=source,
                target=target_color,
                source_uses=source_uses,
                distance=distance,
                low_usage=low_usage,
            )
        )

    return sorted(
        candidates,
        key=lambda item: (
            0 if item.low_usage else 1,
            item.source_uses,
            item.distance,
            item.source,
            item.target,
        ),
    )


def apply_effective_merge(
    sprites: Iterable[Sprite],
    sprite_remaps: Dict[Path, Dict[str, str]],
    source: str,
    target: str,
) -> int:
    changed = 0

    for sprite in sprites:
        remap = sprite_remaps[sprite.path]
        for original_color in sorted(sprite.colors):
            if effective_color(original_color, remap) == source:
                if remap.get(original_color, original_color) != target:
                    remap[original_color] = target
                    changed += sprite.colors[original_color]

    return changed


def force_palette_to_target(
    sprites: List[Sprite],
    sprite_remaps: Dict[Path, Dict[str, str]],
    protected_colors: Set[str],
    target_colors: Optional[int],
    force_low_usage: bool,
    max_uses: int,
    force_max_distance: float,
    skipped: List[Skip],
) -> Tuple[int, List[Tuple[str, str, int, float]]]:
    if target_colors is None:
        state = build_palette_state(sprites, sprite_remaps)
        return len(state.colors), []

    forced_remaps: List[Tuple[str, str, int, float]] = []

    while True:
        state = build_palette_state(sprites, sprite_remaps)
        if len(state.colors) <= target_colors:
            return len(state.colors), forced_remaps

        candidates = build_forced_candidates(
            state,
            protected_colors,
            max_uses,
            force_max_distance,
            force_low_usage,
        )

        if not candidates:
            skipped.append(
                Skip(
                    "*",
                    "*",
                    None,
                    len(state.colors),
                    "target unreachable with current force limits",
                )
            )
            return len(state.colors), forced_remaps

        candidate = candidates[0]
        changed_uses = apply_effective_merge(
            sprites,
            sprite_remaps,
            candidate.source,
            candidate.target,
        )
        if changed_uses <= 0:
            skipped.append(
                Skip(
                    candidate.source,
                    candidate.target,
                    None,
                    candidate.source_uses,
                    "candidate produced no changes",
                )
            )
            return len(state.colors), forced_remaps

        forced_remaps.append((candidate.source, candidate.target, changed_uses, candidate.distance))


def apply_remap_to_sprite(sprite: Sprite, remap: Dict[str, str]) -> int:
    changed = 0

    for primitive in sprite.data["primitives"]:
        old_value = primitive["color"]
        normalized = normalize_color(old_value)
        new_value = effective_color(normalized, remap)
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
        file_keys: Set[Tuple[str, str]] = set()
        for old_color in sorted(sprite.colors):
            new_color = effective_color(old_color, sprite_remaps[sprite.path])
            if old_color == new_color:
                continue
            key = (old_color, new_color)
            uses[key] += sprite.colors[old_color]
            file_keys.add(key)
        files.update(file_keys)

    return {key: (uses[key], files[key]) for key in uses}


def per_file_after_count(sprite: Sprite, remap: Dict[str, str]) -> int:
    return len({effective_color(color, remap) for color in sprite.colors})


def make_report(
    sprites: List[Sprite],
    clusters: List[List[str]],
    global_counts: Counter[str],
    file_counts: Counter[str],
    sprite_remaps: Dict[Path, Dict[str, str]],
    skipped: List[Skip],
    protected_colors: Set[str],
    conservative_color_count: int,
    final_color_count: int,
    target_colors: Optional[int],
    forced_remaps: List[Tuple[str, str, int, float]],
) -> str:
    aggregate = aggregate_remaps(sprites, sprite_remaps)
    changed_files = sum(
        bool(sprite_remaps[sprite.path]) or bool(sprite.normalization_changes)
        for sprite in sprites
    )

    lines = [
        f"files scanned: {len(sprites)}",
        f"files changed: {changed_files}",
        f"total unique colors before: {len(global_counts)}",
        f"total unique colors after conservative remap: {conservative_color_count}",
        f"total unique colors after forced remap: {final_color_count}",
        f"target colors: {target_colors if target_colors is not None else '(none)'}",
        f"forced remaps applied: {len(forced_remaps)}",
        "",
        "top 20 largest color clusters:",
    ]

    largest = sorted(
        (cluster for cluster in clusters if len(cluster) > 1),
        key=lambda cluster: (-len(cluster), -sum(global_counts[color] for color in cluster), cluster),
    )[:20]

    if largest:
        for cluster in largest:
            canonical = choose_canonical(cluster, global_counts, file_counts, protected_colors)
            lines.append(
                f"{len(cluster):3} colors | {sum(global_counts[color] for color in cluster):5} uses"
                f" | canonical: {canonical} | {', '.join(cluster)}"
            )
    else:
        lines.append("(none)")

    lines.extend(["", "per-file color count before/after:"])
    for sprite in sprites:
        before = len(sprite.colors)
        after = per_file_after_count(sprite, sprite_remaps[sprite.path])
        marker = " changed" if sprite_remaps[sprite.path] or sprite.normalization_changes else ""
        normalization = f" | normalized values: {sprite.normalization_changes}" if sprite.normalization_changes else ""
        lines.append(f"{sprite.path}: {before} -> {after}{marker}{normalization}")

    lines.extend(["", "forced remaps:"])
    if forced_remaps:
        for old_color, new_color, uses, distance in forced_remaps:
            lines.append(f"{old_color} -> {new_color} | uses: {uses} | distance: {distance:.2f}")
    else:
        lines.append("(none)")

    lines.extend(["", "remaps:"])
    if aggregate:
        for (old_color, new_color), (uses, files) in sorted(aggregate.items()):
            lines.append(f"{old_color} -> {new_color} | uses: {uses} | files: {files}")
    else:
        lines.append("(none)")

    lines.extend(["", "protected colors:"])
    for color in sorted(protected_colors):
        if color in global_counts:
            lines.append(f"{color} | uses: {global_counts[color]} | files: {file_counts[color]}")

    lines.extend(["", "skipped remaps:"])
    if skipped:
        for item in sorted(
            skipped,
            key=lambda item: (str(item.path or ""), item.old_color, item.new_color, item.reason),
        ):
            location = f" | file: {item.path}" if item.path is not None else ""
            lines.append(
                f"{item.old_color} -> {item.new_color} | uses: {item.uses}{location}"
                f" | reason: {item.reason}"
            )
    else:
        lines.append("(none)")

    alpha_groups = len({parse_rgba(color)[3] for color in global_counts})
    lines.append(
        f"different alpha | alpha groups kept separate: {alpha_groups} | reason: different alpha"
    )

    return "\n".join(lines) + "\n"


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Unify near-duplicate colors in .sprite.json files.",
    )
    parser.add_argument("path", type=Path, help="Sprite JSON file or directory to scan")

    mode = parser.add_mutually_exclusive_group()
    mode.add_argument("--dry-run", action="store_true", help="Preview changes")
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
        help="Maximum fraction of unique colors a sprite may lose during conservative remap",
    )
    parser.add_argument("--force", action="store_true", help="Allow conservative pass to exceed max local loss")
    parser.add_argument("--target-colors", type=int, help="Target maximum global color count")
    parser.add_argument(
        "--force-low-usage",
        action="store_true",
        help="Allow aggressive remapping of rare colors in target-colors pass",
    )
    parser.add_argument(
        "--max-uses",
        type=int,
        default=2,
        help="Maximum global usage for forced low-usage colors",
    )
    parser.add_argument(
        "--force-max-distance",
        type=float,
        default=36.0,
        help="Maximum RGB distance for forced low-usage remaps",
    )

    args = parser.parse_args()

    if args.threshold < 0 or args.local_contrast_threshold < 0 or args.force_max_distance < 0:
        parser.error("thresholds must be non-negative")
    if not 0 <= args.max_local_loss <= 1:
        parser.error("--max-local-loss must be between 0 and 1")
    if args.target_colors is not None and args.target_colors <= 0:
        parser.error("--target-colors must be greater than zero")
    if args.max_uses < 0:
        parser.error("--max-uses must be non-negative")

    return args


def main() -> int:
    args = parse_args()

    try:
        files = find_sprite_files(args.path)
        sprites = [load_sprite(path) for path in files]
    except ValueError as error:
        print(f"[ERROR] {error}")
        return 1

    global_counts, file_counts = collect_colors(sprites)
    protected_colors = build_protected_colors(global_counts)

    clusters = build_clusters(global_counts, args.threshold)
    skipped: List[Skip] = []
    conservative_global_remap = build_conservative_global_remap(
        clusters,
        global_counts,
        file_counts,
        protected_colors,
        skipped,
    )

    sprite_remaps: Dict[Path, Dict[str, str]] = {}
    for sprite in sprites:
        sprite_remap, sprite_skips = proposed_sprite_remap(
            sprite,
            conservative_global_remap,
            args.local_contrast_threshold,
            args.max_local_loss,
            args.force,
        )
        sprite_remaps[sprite.path] = sprite_remap
        skipped.extend(sprite_skips)

    conservative_state = build_palette_state(sprites, sprite_remaps)
    conservative_color_count = len(conservative_state.colors)

    final_color_count, forced_remaps = force_palette_to_target(
        sprites,
        sprite_remaps,
        protected_colors,
        args.target_colors,
        args.force_low_usage,
        args.max_uses,
        args.force_max_distance,
        skipped,
    )

    report = make_report(
        sprites,
        clusters,
        global_counts,
        file_counts,
        sprite_remaps,
        skipped,
        protected_colors,
        conservative_color_count,
        final_color_count,
        args.target_colors,
        forced_remaps,
    )
    print(report, end="")

    if args.report:
        args.report.write_text(report, encoding="utf-8")

    if args.target_colors is not None and final_color_count > args.target_colors:
        print(
            f"[ERROR] Could not reduce palette to {args.target_colors} colors. "
            f"Current colors: {final_color_count}. "
            "Try increasing --force-max-distance or --max-uses."
        )
        return 1

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
