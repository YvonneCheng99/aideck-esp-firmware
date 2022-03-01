# AI-Deck ESP32 firmware [![CI](https://github.com/bitcraze/aideck-esp-firmware/workflows/CI/badge.svg)](https://github.com/bitcraze/aideck-esp-firmware/actions?query=workflow%3ACI)

This firmware is running on the ESP32 Nina W102 module of the AI-Deck. It
implements communication with the GAP8 chip, the Crazyflie's STM32 main MCU as
well as implementing Wifi communication.

**NOTE**: This firmware is still under heavy development and will probably change radically before
it's ready for public use.

**NOTE**: The default configuration now uses the NINA_SYSBOOT as log UART TX.

## Compile and flash

The firmware is currently designed to be flashed and run from the serial port.

It has been developed using the version 4.3.1 of the [Esp32 IDF].

To flash and run:
```
$ source ~/path/to/idf/export.sh
$ idf.py build
$ idf.py flash
$ idf.py monitor
```

[Esp32 IDF]: https://github.com/espressif/esp-idf.git

### Flash using the builder image

docker run --rm -it -v $PWD:/module/ --device /dev/ttyUSB0 --privileged -P bitcraze/builder /bin/bash -c "openocd -f interface/ftdi/olimex-arm-usb-tiny-h.cfg -f board/esp-wroom-32.cfg -c 'adapter_khz 20000' -c 'program_esp build/bootloader/bootloader.bin 0x1000 verify' -c 'program_esp build/aideck_esp.bin 0x10000 verify reset exit'"

### Build with the [toolbelt](https://github.com/bitcraze/toolbelt)

When using the [toolbelt](https://github.com/bitcraze/toolbelt) the required toolchain is running in a docker container
and no tools have to be installed on the machine.
Just pre-pend make with tb, for instance

`tb make`

`tb make clean`
