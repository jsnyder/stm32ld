STM32 Firmware Loader
=====================

These C sources can be used to burn .bin (no hex support) firmware images to STM32 microcontrollers using the built-in serial bootloader.

> Usage: stm32ld &lt;port&gt; &lt;baud&gt; &lt;binary image name|0 not to flash&gt; [&lt;0|1 to send Go command to new flashed app&gt;]
  

Building
--------

If lake is installed:
> lake

Otherwise:

Linux/Mac OS X:
> gcc -o stm32ld main.c stm32ld.c serial_posix.c


Original source author: Bogdan Marinescu <bogdan.marinescu@gmail.com>
