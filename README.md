STM32 Firmware Loader
=====================

These C sources can be used to burn .bin (no hex support) firmware images to STM32 microcontrollers using the built-in serial bootloader.

> Usage: stm32ld <port> <baud> <binary image name>
  

Building
--------

Linux/Mac OS X:
> gcc -o stm32ld main.c stm32ld.c serial_posix.c


Original source author: Bogdan Marinescu <bogdan.marinescu@gmail.com>
