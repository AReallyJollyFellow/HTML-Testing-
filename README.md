# FORSKN3D

A compact TI-84 Plus CE chase-survival game with a Noob survivor and a Slasher
killer. It adapts Forsaken character roles and abilities into a single-player
calculator game; it is not a Roblox client or an exact game recreation.

## Build

Install the CE C/C++ Toolchain, add its `CEdev/bin` folder to `PATH`, and run:

```sh
make
```

The calculator program is written to `bin/FORSKN3D.8xp`. The reduced sprites in
`assets/gfx/` are already prepared for the build. After changing source PNGs,
run `python3 tools/prepare_assets.py` (requires Pillow), then rebuild.

## Controls

- Arrow keys: move and turn
- `2nd`: sprint
- `7`: Bloxy Cola speed boost
- `8`: Slateskin damage shield (press again to cancel)
- `9`: Ghostburger stealth and healing
- `Enter`: repair a nearby generator; reach the north gate after all three
- `Clear`: quit

The survivor and killer use procedural rigs with separate movement, hit,
repair, item-use, and attack animations. The supplied Noob diffuse map provides
the face; the Slasher head, hood, and machete maps are used on the killer rig.
The supplied ability icons and intro banner are also included in the game.

The supplied `assets/scene.gltf` references a `scene.bin` mesh buffer that was
not included, so the calculator build uses the procedural Noob rig instead.
Its Noob texture URI has been corrected to the supplied PNG. The GLTF metadata
credits vanyabro85 and CC BY 4.0.

The gameplay details were cross-checked against the [Forsaken Wiki pages for
Slasher](https://forsaken2024.fandom.com/wiki/Slasher) and
[Noob](https://forsaken2024.fandom.com/wiki/Noob); the CE build flow follows the
[CE Toolchain documentation](https://ce-programming.github.io/toolchain/static/getting-started.html).
