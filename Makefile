# Wolfenstein 3D (fx-9860, by Kelli) on the fx-CG50.
#   make GAME=WOLF.g1a         payload.js (the shim and the .g3a around it) for the page
#   make check GAME=WOLF.g1a   the page's converter must give the same .g3a as fxgxa
# Building needs the fxSDK toolchain (sh-elf-gcc, fxgxa) and node; using the page needs neither.

GAME    ?= WOLF.g1a
CC      := sh-elf-gcc
CFLAGS  := -m4-nofpu -mb -Os -ffreestanding -nostdlib -fno-builtin -Wall -Wextra \
           -ffunction-sections -fdata-sections

all: payload.js

build/%.o: shim/%.c
	@mkdir -p build
	$(CC) $(CFLAGS) -c $< -o $@
build/%.o: shim/%.S
	@mkdir -p build
	$(CC) $(CFLAGS) -c $< -o $@

build/shim.elf: build/entry.o build/shim.o build/cg.o shim/shim.ld
	$(CC) $(CFLAGS) -T shim/shim.ld -Wl,--gc-sections -Wl,-Map=build/shim.map \
	      build/entry.o build/shim.o build/cg.o -lgcc -o $@

build/shim.bin: build/shim.elf
	sh-elf-objcopy -O binary $< $@

payload.js: $(GAME) build/shim.elf build/shim.bin tools/mkpayload.py icon/icon-uns.png icon/icon-sel.png
	python3 tools/mkpayload.py $(GAME) build/shim.elf build/shim.bin

build/Wolf3D.g3a: $(GAME) payload.js convert.js
	node tools/convert.cjs $(GAME) $@

check: build/Wolf3D.g3a
	cmp build/Wolf3D.g3a build/reference.g3a && echo "converter output = fxgxa reference"

clean:
	rm -rf build

.PHONY: all check clean
