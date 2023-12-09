GBCartRead is an Arduino based Gameboy Cartridge Reader which uses a python script to interface with 
the Arduino. GBCartRead allows you to dump your ROM, save the RAM and write to the RAM.

Works with Arduino Uno. 

This is an adaptation from the original GBCartReader from here: https://github.com/insidegadgets/GBCartRead


REQUIREMENTS
=============
- Arduino
- Breadboard or similar
- 74HC595 Shift Registers (2)
- 470 Ohm Resistors – optional, the resistors would be for safety purposes only to prevent accidental shorts.
- Gameboy Cartridge Adapter (or similar) - http://dx.com/p/repair-parts-replacement-gba-game-cart-slot-for-nds-lite-37787


HOW TO USE
=================================
1. Open the \GBCartRead_v(xxx)_Arduino\GBCartRead_v(xxx).ino file and program it to your Arduino. 
   Note down the COM port number in use; you can find this out from the Arduino software by going to Tools -> Serial port.
---Python Program---
2. Download and install Python 3.2 (http://www.python.org/download/) and pySerial (http://pypi.python.org/pypi/pyserial)
3. Open up the “GBCartRead_v(xxx)_Python_Interface.py” script by right clicking it and selecting “Edit with IDLE”.
4. Change the “COM5” serial port to the serial port that your Arduino is connected on, save the file and press F5 
     to run it.
5. A new window will appear, after 2-3 seconds you’ll have some options available. 

6. At this stage you should insert your Gameboy cartridge and press the power button, the power LED should light up.
7. Press 0 to read the header and verify that it looks correct. If it doesn’t look correct, press the power button 
   to power off the Gameboy cartridge, remove and re-insert it and power it up again.
8. Press 1 to Dump the ROM, 2 to Backup your RAM or 3 to Load your RAM file. Hashes (#) will start printing every 
   few seconds and a file called <gametitle>.gb or .sav will be created if you chose option 1 or 2. If you choose 
   option 3, it will load the save from <gametitle>.sav.

We recommended verifying your Gameboy ROM using BGB (a Gameboy emulator) or "xgbfix.exe -v -d <your_rom.rom>" 
found in the project called "ASMotor" (we've included it). It’s a good idea to verify your save files too by 
running the ROM when the save file is present in BGB.


CuSTOM CARTRIDGE FILES
=================================
There are some cartridges which don't quite conform to cartridge header standards or require something out of the ordinary.
Below is the listing of custom cartridge folders, if you wish to access these cartridges, you will need to use GBCartRead 
files found on those folders.

Wisdom Tree - /Custom_Cartridge_Files/Wisdom_Tree/ - Provided by Benjamin E


REVISION HISTORY
=================================
v1.2
- Speed increase by using SPI.transfer for shift registers
-----> 1min 14sec run time (10.7x faster than original v1.0 method)

v1.1
- Speed increases by using direct PORT writing/reading 
- Compacting functions
- Delays when pulsing WR/RD/MREQ pins reduced to a few NOP cycles
---> 2min 43sec run time

v1.0 
- Made initial working adaption to meet my setup
---> 13min 10sec run time


----------------------------------------------------------------------------
This work is licensed under a Creative Commons Attribution-NonCommercial 3.0 Unported License.
http://creativecommons.org/licenses/by-nc/3.0/