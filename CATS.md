# Little One Cats

This document describes the planned cat characters for **Little One**.

The goal is to keep every cat visually recognizable, but simple enough to recreate with procedural primitives such as circles, ellipses, triangles, rectangles, and lines.

Each cat should have:

- a unique base color palette;
- a readable silhouette;
- one or two strong visual traits;
- simple accessories;
- no complex legs;
- no overly detailed weapons or tiny decorative parts.

Animation should mostly rely on simple squash and stretch scaling on the X/Y axes.

---

## Cat List

| ID              | Character     | Main Cat Color                               | Belly / Muzzle        | Eyes                         | Accessories / Accent Colors                          |
| --------------- | ------------- | -------------------------------------------- | --------------------- | ---------------------------- | ---------------------------------------------------- |
| `agent00cat`    | Agent 007     | dark gray `#1F2326`                          | white `#F2F0E8`       | black / white                | black bow tie, white shirt                           |
| `carrambacat`   | Jack Sparrow  | orange `#D87918` with dark stripes `#8A3F12` | cream `#F3D6A2`       | brown / gold                 | brown pirate hat, red bandana, gold accents          |
| `catbuster`     | Ghost Hunter  | blue-gray `#5E6F73`                          | light beige `#D9C9A8` | acid green `#8DFF4A`         | beige belt, green indicator, black backpack          |
| `darthcat`      | Darth Vader   | almost black `#0B0B0D`                       | dark gray `#25252A`   | red `#D71920`                | black helmet/mask, red chest panel                   |
| `impawsiblecat` | Ethan Hunt    | graphite `#2E3338`                           | gray `#B8BEC2`        | blue-gray `#7CA7C7`          | black tactical harness, small red laser/accent       |
| `pinkpawther`   | Pink Panther  | pink `#F28BB8`                               | light pink `#FFD1E3`  | yellow `#F7D64A`             | minimal accessories, black whiskers, sly face        |
| `scarcat`       | Tony Montana  | white `#F1EEE2`                              | light gray `#D8D4CA`  | dark brown `#3B2415`         | white suit, red shirt, gold chain                    |
| `doomcat`       | Doomguy       | dark green `#354B2E`                         | bronze-gray `#9A8B6A` | red / orange `#FF4A1C`       | green armor, gray helmet, red accents                |
| `indicat`       | Indiana Jones | sandy brown `#A86D32`                        | cream `#E8C895`       | green-brown `#6C7A3D`        | brown hat, dark belt, whip                           |
| `robocat`       | Robocop       | steel blue `#5E7180`                         | silver `#C7D0D6`      | cyan `#5ED7FF`               | metallic mask, black visor, blue accents             |
| `termicator`    | Terminator    | dark gray `#3B3B3B`                          | silver `#AEB4B8`      | one red `#FF1E1E`, one black | half-metal face, red eye                             |
| `harrypurrter`  | Harry Potter  | brown `#6B4428`                              | beige `#D9B98F`       | green `#4CAF50`              | round glasses, burgundy/yellow scarf, lightning mark |
| `jawscat`       | Jaws          | blue-gray `#4E6F82`                          | white `#F1F5F5`       | black `#111111`              | top fin, white toothy muzzle                         |
| `rockpaw`       | Rocky Balboa  | light gray `#8C8C86`                         | white `#EEE7D8`       | dark brown `#2B1B12`         | red boxing gloves, black headband                    |

---

## Visual Rules

Each cat should have a unique “signature color” so the player can recognize it quickly, even at a very small size.

Examples:

- `catbuster` — blue-gray body with green accents.
- `doomcat` — dark green body with red/orange accents.
- `robocat` — steel-blue metallic body with cyan accents.
- `termicator` — dark gray body with one red eye.
- `pinkpawther` — pink body with sly yellow eyes.

The design must stay readable at around 48–64 px.

---

## Primitive-Friendly Design Rules

Avoid:

- realistic weapons;
- complex hands or legs;
- detailed clothing folds;
- many tiny decorations;
- curved shapes that cannot be approximated with simple primitives;
- character designs that require too many unique parts.

Prefer:

- big simple body shape;
- triangle ears;
- simple oval belly;
- simple eye shapes;
- one strong accessory;
- flat colors;
- bold silhouette;
- low command count.

---

## Recommended Command Budget

Target command count per cat:

```text
base cat:       12–18 commands
accessories:     6–12 commands
total:          18–30 commands
```

If a cat needs more than 30 commands, the design is probably too complex and should be simplified.

---

## Base Cat Strategy

Most cats should be built as:

```text
base_cat
+ palette
+ accessories
```

This keeps the art pipeline fast and makes it easier to add many cats without redrawing every character from scratch.

Recommended shared base parts:

- body;
- ears;
- inner ears;
- belly;
- eyes;
- pupils;
- nose;
- mouth;
- whiskers;
- tail.

Accessories should be modular and reusable where possible.

---

## Notes

The goal is not to make perfect parody art.

The goal is to make tiny, funny, recognizable cats that fit the Little One style and can be generated from compact procedural sprite data.
