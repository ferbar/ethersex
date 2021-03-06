ferbar/ethersex
===============
original repo see: https://github.com/ethersex/ethersex

This repo contains the following patches / addons
* Fixed TCP reset handling: Aborted TCP connections are handled properly
* reduced size of some global variables
* Added home-assistant-mqtt: read the IO pins either as analog or digital pin (as a HA sensor), configure a pin as switch.
  The pin-settings are configurable with ecmd commands. HA Auto-discovery is implemented, too. (https://www.home-assistant.io/docs/mqtt/discovery/)
  
The included .config-home-assistant-mqtt is ready to use for a avr-netio with an Atmega32
* ecmd: serial, udp (tcp would take too much ram)
* dhcp with a packet length of 600bytes (to allow dhcp with Fritz!Box-es which send 590byte dhcp ack packets)
* debug enabled (reduced buffer length)
* dns enabled (cache size reduced to 2)
* mqtt
* home-assistant-mqtt (mqtt server = "home-assistant")


About Ethersex
==============
Ethersex, originally developed to provide an alternative firmware for the [etherrape hardware](http://www.lochraster.org/etherrape),
evolved into a full-featured still light-weight firmware for the Atmel megaAVR processors.  
For more information and a comprehensive documentation  consult [http://www.ethersex.de](http://www.ethersex.de)!

How to configure the firmware
=============================
Make sure that you meet the requirements.  
Use `make menuconfig` to configure and `make` to compile the firmware.
The final hex file is named `ethersex.hex`.

[See the Quick Start Guide in the wiki for more information](http://ethersex.de/index.php/Quick_Start_Guide)

How to add a new hardware pinning
=================================
Use the script at `scripts/add-hardware` to add a new pinning.


Used 3rd party software 
=======================
This program contains software by other authors:

* [the uIP tcp/ip stack](http://www.sics.se/~adam/uip) in the directory `/protocols/uip/`, written by Adam Dunkels
* [usb-software stack from obdev](http://www.obdev.at/products/vusb/index.html) in `/protocols/usb/usbdrv/`
* [sd card reader](http://www.roland-riegel.de/sd-reader/index.html) in `/hardware/storage/sd_reader`
* [IRMP - Infrared Multi Protocol Decoder](https://www.mikrocontroller.net/articles/IRMP) in `/hardware/ir/irmp/lib` written by Frank Meyer

License
=======
All ethersex related code is licensed under GPLv3, unless otherwise noted. See COPYING in the main
directory, but in doubt check the file header. Usually every file contains a
header, stating all contributing authors and the specific license used.

Various make targets
====================

* `make show-config` -- Shows the activated modules
