# ledpijp

efficiently drive an APA102 LED strip over SPI with pretty animations

## code map

- `config.h` user-configurable compile-time settings
- `ledpijp.c` contains the entry point, interrupt registration, and overall coordinating code
- `apa102.c` generates and send animation frames to the LED strip
- `spi.c` mid-level interface to the linux SPI driver at `/dev/spidev*`
- `fake102.c` fake version of `led_blit` from `apa102.c` for animation development, displays frames in a webpage

## building & running

- install a c compiler
- run `./build.sh` (uses `gcc` by default)
- run `./ledpijp`
