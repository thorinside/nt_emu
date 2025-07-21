# Disting NT VCV Rack Plugin

VCV Rack plugin emulation of the Expert Sleepers Disting NT eurorack module.

## Features

- Complete emulation of Disting NT multi-algorithm architecture
- 256x64 OLED display simulation
- All hardware controls: 3 pots, 2 encoders, 4 buttons
- 4 audio inputs/outputs + CV inputs/outputs
- Algorithm switching via parameter or context menu
- Preset management and state saving
- Bus-based internal routing system (28 buses)

## Building

This plugin requires the VCV Rack SDK v2.x.

```bash
export RACK_DIR=~/Rack-SDK
make clean
make
make install
```

## License

MIT License - see LICENSE.txt