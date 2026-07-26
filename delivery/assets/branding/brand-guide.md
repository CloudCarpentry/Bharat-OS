# Bharat-OS Brand Guide

## Tagline

**"Verified. Sovereign. Open."**

---

## Logo

The Bharat-OS logo is a stylised **"B"** composed of two C-bracket shapes:

- **Top arc** — Saffron Orange, representing courage and technological ambition.
- **Inner gap** — White, representing transparency and openness.
- **Bottom arc** — India Green, representing growth and the land it was built for.
- **Centre chip** — Dark slate-blue microchip with PCB circuit traces; the Ashoka Chakra's 24-spoke motif is subtly etched into the die.

### Available Files

| File              | Use                                          |
| ----------------- | -------------------------------------------- |
| `logo-icon.png`   | Favicon, app icons, square embeds            |
| `banner-dark.png` | GitHub README header, website, presentations |

---

## Color Palette

| Token             | Hex       | Usage                                         |
| ----------------- | --------- | --------------------------------------------- |
| `--color-saffron` | `#FF9933` | Primary accent, logo top, highlights          |
| `--color-green`   | `#138808` | Secondary accent, logo bottom, success states |
| `--color-white`   | `#FFFFFF` | Background, logo gap, text on dark            |
| `--color-navy`    | `#0A0F1E` | Dark background (preferred)                   |
| `--color-slate`   | `#2D3A4A` | Chip/chip-trace color, subtle UI elements     |
| `--color-gray`    | `#8A9BB0` | Secondary text, captions                      |

---

## Typography

| Role            | Font                                                 | Weight        |
| --------------- | ---------------------------------------------------- | ------------- |
| Headings        | [Inter](https://fonts.google.com/specimen/Inter)     | 700 (Bold)    |
| Body            | Inter                                                | 400 (Regular) |
| Code / Terminal | [JetBrains Mono](https://www.jetbrains.com/lp/mono/) | 400           |

---

## Shell Prompt Style

```
[bharat-os kernel]$ _
```

Colors: `[bharat-os` in saffron `#FF9933`, `kernel]$` in green `#138808`, cursor in white.

---

## Architecture Visual Language

Architecture diagrams should use the following semantic palette to ensure consistency and clarity across all engineering and platform visual materials:

### Semantic Color Usage

| Meaning / Target | Color / Token | Hex |
|---|---|---|
| Primary Accent / Core Mechanism | Saffron Orange | `#FF9933` |
| Completed / Trusted / Success State | India Green | `#138808` |
| Primary Dark Background | Navy Blue | `#0A0F1E` |
| Neutral / Internal Structures / Cards | Dark Slate | `#2D3A4A` |
| Supporting Text / Metadata | Secondary Gray | `#8A9BB0` |
| General Software / Interface Blue | Blue | `#3B82F6` |
| Transitional State | Amber / Orange | `#FF6F00` |
| Target State / Production-Grade | Teal / Emerald Green | `#00BFA5` |

### Diagram Consistency Rules

1. **Dark Backgrounds by Default:** Prefer dark-background (`#0A0F1E`) diagrams for all GitHub README and documentation assets to match the cohesive developer aesthetic.
2. **Minimal Styling Complexity:** Favor flat or minimal gradients, minimal/subtle shadows, and rounded but restrained container cards. Avoid ornamental circuitry or excessive decorative elements that reduce technical scannability.
3. **SVG as the Canonical Source:** Always check in and reference SVG files in documentation. SVGs are easily version-controlled and scale perfectly.
4. **Readable at GitHub README Width:** Keep text sizes, labels, and line/stroke weights large enough to remain fully readable when scaled to a standard GitHub README view (approx. 1100–1400px wide). Avoid embedding tiny explanatory paragraph text inside the SVGs.
5. **No Color-Only Maturity Representation:** Do not encode critical architectural or component maturity status using color alone. Always provide explicit semantic labels (e.g., `[BASELINE]`, `[PARTIAL]`, `[TRANSITIONAL]`, `[SCAFFOLD]`, or `[TARGET]`) so that the diagrams are accessible and clear without color.
6. **No Clutter or National Motifs:** Respect the brand identity. The Bharat identity should come through cleanly via consistent typography and saffron/green accent colors—do **not** include repeated flag motifs or decorate every engineering diagram with the Ashoka Chakra.

### Accessibility

- **Contrast:** Ensure all text-on-background combinations in diagrams meet accessible contrast standards (e.g., high-contrast white or saffron text on dark navy cards).
- **Titles and Descriptions:** Include semantic `<title>` and `<desc>` tags inside SVG files to enable screen-reader parsing where practical.
- **Alt Text:** When embedding SVGs or images in Markdown, always provide descriptive `alt` text explaining what the diagram represents.

---

## Usage Rules

1. Do **not** stretch or distort the logo.
2. Maintain a clear-space margin of at least **1× the chip icon width** around the logo.
3. On light backgrounds, use the icon variant (`logo-icon.png`).
4. On dark backgrounds, use the banner variant (`banner-dark.png`).
5. All brand assets are released under **CC BY 4.0** — attribution required: _"Bharat-OS by Divyang Panchasara"_.

---

## Asset License

Brand assets are licensed under [Creative Commons Attribution 4.0 International (CC BY 4.0)](https://creativecommons.org/licenses/by/4.0/).  
The source code itself remains under the MIT License (see root `LICENSE`).
