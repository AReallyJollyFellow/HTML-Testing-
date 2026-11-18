from pathlib import Path

from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "assets"
OUTPUT = SOURCE / "gfx"

PALETTE_BASES = (
    (0, 0, 0),
    (255, 215, 60),
    (40, 110, 200),
    (110, 170, 60),
    (90, 90, 110),
    (210, 30, 30),
    (150, 150, 155),
    (235, 235, 225),
    (110, 75, 50),
    (255, 255, 255),
    (20, 22, 34),
    (30, 40, 34),
    (230, 170, 40),
)
PALETTE_SHADES = (95, 150, 200, 255)
RESAMPLE = Image.Resampling.LANCZOS


def load(name):
    return Image.open(SOURCE / name).convert("RGBA")


def save(image, name, size=None, resample=RESAMPLE):
    if size:
        image = image.resize(size, resample)
    if "A" in image.getbands():
        image.putalpha(image.getchannel("A").point(lambda value: 255 if value >= 128 else 0))
    image.save(OUTPUT / name, optimize=True)


def main():
    OUTPUT.mkdir(parents=True, exist_ok=True)

    save(load("Jason_Intro.png"), "intro_banner.png", (220, 110))
    for source, target in (
        ("JasonSlash_Icon.png", "slash_icon.png"),
        ("JasonBehead_Icon.png", "behead_icon.png"),
        ("JasonGashingWound_Icon.png", "gashing_icon.png"),
        ("JasonRagingPace_Icon.png", "rage_icon.png"),
    ):
        save(load(source), target, (22, 22))

    noob = load("N00b1Mtl_baseColor.png")
    save(noob.crop((896, 0, 1024, 128)), "noob_face.png", (20, 20), Image.Resampling.NEAREST)

    head = load("Head1_diff.png")
    save(head.crop((80, 160, 192, 256)), "slasher_mask.png", (20, 18))
    hood = load("Hood1_diff.png")
    save(hood.crop((32, 24, 160, 152)), "slasher_hood.png", (18, 18))
    save(load("Machete1_diff.png"), "machete.png", (12, 24))

    icon = load("JasonSlash_Icon.png")
    save(icon, "program_icon.png", (16, 16))

    palette = Image.new("RGB", (52, 1))
    for base_index, base in enumerate(PALETTE_BASES):
        for shade_index, shade in enumerate(PALETTE_SHADES):
            color = tuple(component * shade >> 8 for component in base)
            palette.putpixel((base_index * 4 + shade_index, 0), color)
    palette.save(OUTPUT / "game_palette.png", optimize=True)


if __name__ == "__main__":
    main()
