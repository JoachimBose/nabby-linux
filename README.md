# Nabby linux
This contains the buildroot, uboot & linux configurations for a hobby project. To get started, clone a new buildroot repository outside this one.
Then, from within the buildroot repository, execute:
```bash
BR2_EXTERNAL=../nabby-linux make nabby-linux_defconfig
```
This will import all the files from this repository. If you are new to buildroot, consider taking a look around this repository to see what you just imported.

after you compiled your buildroot, you should get a `flash.bin` image as described in the genimage.cfg. To flash it, put the chip in FEL-mode.
This is done by shorting chipselect of the spiflash to 3.3 volts and powering on the device, after powerup remove the short. Now you can use:
```bash
sunxi-fel -p spiflash-write 0 flash.bin
```
to flash the board while its in FEL mode.

# VUCTF-2026 board

The VUCTF26 board has support for the w5500 dev boards like the w850io or the cheaper USR-ES1. There exist also the Spookystats vulnerable website and the portalctl privilege escalation 
challenge which can be compiled as a buildroot package. Any questions are welcome towards joa0405 on discord!

![board.png](pics/board.png?raw=true "VUCTF26 board")
![board.png](pics/schematic.png?raw=true "VUCTF26 schematic")
![schematic.pdf](pics/schematic.pdf?raw=true "VUCTF26 schematic")


## Acknowledgements
When we built the nabby board, we forked the repository from: [BasicCode](https://github.com/BasicCode/F1C100-Business-Card/tree/main/board/f1c100-business-card)
and we are very thankful for their contribution & GPL-2.0 license. In BasicCodes repository, they acknowledged help from the following authors:

This project is based heavily on the work of others; I would like to acknowledge these project in particular:
* https://github.com/thirtythreeforty/businesscard-linux/tree/master
* https://github.com/florpor/licheepi-nano/tree/master
* [F1C100 Datasheet](https://whycan.com/files/members/3/F1C100s_Datasheet_V1_0.pdf)
* [Lichee-Pi Nano](https://wiki.sipeed.com/hardware/en/lichee/Nano/Nano.html)

* 
