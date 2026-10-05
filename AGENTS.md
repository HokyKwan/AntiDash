<!--
Copyright (c) 2026 HokyKwan <hokykwan0@gmail.com>

SPDX-License-Identifier: Apache-2.0
-->

# antidash build and flash

Run these from this repository after activating the workspace virtualenv. `west` is installed in that environment, not on the system path.

```bash
source ~/zephyr-os/.venv/bin/activate
```

## Build

Board target is `esp32s3_devkitc/esp32s3/procpu`.

```bash
west build -b esp32s3_devkitc/esp32s3/procpu
```

Use a clean rebuild when the build directory is stale:

```bash
west build -b esp32s3_devkitc/esp32s3/procpu --pristine
```

## Flash

The board enumerates as `/dev/ttyUSB0`.

```bash
west flash -d build/ --esp-device /dev/ttyUSB0
```
