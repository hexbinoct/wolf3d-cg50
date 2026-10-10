# Wolfenstein 3D for the fx-CG50

Kelli's *Last Wolfenstein 3D* for the fx-9860 series, running on the fx-CG50 / Graph 90+E.

**Convert it in your browser: https://hexbinoct.github.io/wolf3d-cg50/**

1. Download file 2 (the SH4 version) of the game from
   [Planète Casio](https://www.planet-casio.com/Fr/programmes/programme1967-1-wolfenstein-3d-Kelly-.html).
2. Drop it on the page above. You get `Wolf3D.g3a`, made in your browser; nothing is uploaded.
3. Copy `Wolf3D.g3a` to the calculator in USB Flash mode.

Keys: arrows move and turn, SHIFT shoots, ALPHA opens doors, F1/F2 knife/gun, F6/F5 hide/show the
status bar, MENU pauses, EXIT quits, + and − change the speed.

## How it works

The game's source was never published, so it isn't rewritten: its own machine code runs on the
fx-CG50. Both calculators use SuperH processors, and both operating systems load an add-in at
`0x00300000`, so the code sits at the address it was built for. A shim (`shim/`) handles what
differs:

- the fx-9860 system calls (timers, files, keyboard) are redirected to the fx-CG50's;
- the routine that sent the 128×64 screen to the monochrome LCD now draws it three times larger
  in colour VRAM;
- quitting returns to the main menu instead of resetting;
- the game has no frame limiter, so the shim slows it to fx-9860 pace (adjustable with + and −).

The page (`index.html`, `convert.js`) contains no part of the game. `payload.js` is a complete
`.g3a` with zeros where the game goes, plus the patches that redirect it to the shim. The page
copies your copy of the game into it and recomputes the checksum.

## Building

Needs the [fxSDK](https://git.planet-casio.com/Lephenixnoir/fxsdk) toolchain (`sh-elf-gcc`,
`fxgxa`), Python 3 and Node.

```
make GAME=WOLF.g1a          # payload.js
make check GAME=WOLF.g1a    # converter output must equal the fxgxa build
node tools/convert.cjs WOLF.g1a Wolf3D.g3a
```

## Credits

The game is by Kelli. This repository only holds the shim and the converter, under the GPL-3.0
(`LICENSE`). Not affiliated with Casio or id Software.
