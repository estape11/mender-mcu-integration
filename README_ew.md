# EW2025-NA Instructions

## Setup
Clone the repository and create the workspace
```
west init ew2025-na --manifest-url https://github.com/estape11/mender-mcu-integration --manifest-rev EW2025-NA
```

Update and retrieve resources
```
cd ew2025-na && west update && west blobs fetch hal_espressif
```

## Building

### Bootloader
Enter the bootloader folder
```
cd bootloader/mcuboot/boot/zephyr
```

Setup a venv
```
python3 -m venv myenv
```

Source the env
```
source myenv/bin/activate
```

Install the esptool library
```
pip install esptool
```

Build the bootloader
```
west build -p --board esp32s3_devkitc/esp32s3/procpu
```

Erase and flash the bootloader
```
python -m esptool --chip esp32-s3 erase_flash  && west flash
```

### OS/application
Change the directory to the main ew2025na-new folder
```
cd ew2025na-new
```

Build the binary
```
 west build -p --board esp32s3_devkitc/esp32s3/procpu mender-mcu-integration -- -DEXTRA_DTC_OVERLAY_FILE=boards/shields/ew2025.overlay -DEXTRA_CONF_FILE=boards/shields/ew2025.conf
```

Flash the board
```
west flash
```

Monitor the serial output
```
west espressif monitor
```