/*
 GBCartRead
 Version: 1.4.1
 Author: Alex from insideGadgets (http://www.insidegadgets.com)
 Created: 18/03/2011
 Last Modified: 3/01/2014
 
 Read ROM, Read RAM or Write RAM from/to a Gameboy Cartridge.
 
 */
 
int latchPin = 10;
int dataPin = 11;
int clockPin = 13;
int rdPin = A5;
int wrPin = A3;
int mreqPin = A4;

uint8_t nintendoLogo[] = {0xCE, 0xED, 0x66, 0x66, 0xCC, 0x0D, 0x00, 0x0B, 0x03, 0x73, 0x00, 0x83, 0x00, 0x0C, 0x00, 0x0D,
                  0x00, 0x08, 0x11, 0x1F, 0x88, 0x89, 0x00, 0x0E, 0xDC, 0xCC, 0x6E, 0xE6, 0xDD, 0xDD, 0xD9, 0x99,
                  0xBB, 0xBB, 0x67, 0x63, 0x6E, 0x0E, 0xEC, 0xCC, 0xDD, 0xDC, 0x99, 0x9F, 0xBB, 0xB9, 0x33, 0x3E};
char gameTitle[17];
uint16_t cartridgeType = 0;
uint16_t romSize = 0;
uint16_t romBanks = 0;
uint16_t ramSize = 0;
uint16_t ramBanks = 0;
uint16_t ramEndAddress = 0;

#include "pindeclarations.h"
// #include <SPI.h>

void setup() {
  Serial.begin(460800);
  pinMode(latchPin, OUTPUT);
  pinMode(clockPin, OUTPUT);
  pinMode(dataPin, OUTPUT);
  pinMode(rdPin, OUTPUT);
  pinMode(wrPin, OUTPUT);
  pinMode(mreqPin, OUTPUT);
  
  // Set pins as inputs
  DDRB &= ~((1<<PB0) | (1<<PB1)); // D8 & D9
  DDRD &= ~((1<<PD2) | (1<<PD3) | (1<<PD4) | (1<<PD5) | (1<<PD6) | (1<<PD7)); // D2 to D7
}

void loop() {
  // Serial.println("Loop begin");
  
  // Wait for serial input
  while (Serial.available() <= 0) {
    delay(200);
  }
 
  // Decode input
  char readInput[10];
  uint8_t readCount = 0;
  while (Serial.available() > 0) {
    char c = Serial.read();
    readInput[readCount] = c;
    readCount++;
  }
  readInput[readCount] = '\0';

  if (strstr(readInput, "HEADER")) {
    // Turn everything off
    rd_wr_mreq_reset();
    
    // Read Cartridge Header
    gameTitle[17];
    for (int addr = 0x0134; addr <= 0x143; addr++) {
      // gameTitle[(addr-0x0134)] = (char) readbank0Address(addr);
      // Swap line above ^^^ with section below if you have issues with non roman characters--below code doesn't support Japanese title
      char headerChar = (char) readByte(addr);
      if ((headerChar >= 0x30 && headerChar <= 0x57) || // 0-9
            (headerChar >= 0x41 && headerChar <= 0x5A) || // A-Z
            (headerChar >= 0x61 && headerChar <= 0x7A)) { // a-z
              gameTitle[(addr-0x0134)] = headerChar;
        } else {
          gameTitle[(addr-0x0134)] = '0';
        }
    }
    gameTitle[16] = '\0';

    // Nintendo Logo Check
    uint8_t logoCheck = 1;
    uint8_t x = 0;
    for (uint16_t romAddress = 0x0104; romAddress <= 0x133; romAddress++) {
      if (nintendoLogo[x] != readByte(romAddress)) {
        logoCheck = 0;
        break;
      }
      x++;
    }

    cartridgeType = readByte(0x0147);
    romSize = readByte(0x0148);
    ramSize = readByte(0x0149);

    // romBanks = 2; // Default 32K
    // if (romSize == 1) { romBanks = 4; } 
    // if (romSize == 2) { romBanks = 8; } 
    // if (romSize == 3) { romBanks = 16; } 
    // if (romSize == 4) { romBanks = 32; } 
    // if (romSize == 5 && (cartridgeType == 1 || cartridgeType == 2 || cartridgeType == 3)) { romBanks = 63; } 
    // else if (romSize == 5) { romBanks = 64; } 
    // if (romSize == 6 && (cartridgeType == 1 || cartridgeType == 2 || cartridgeType == 3)) { romBanks = 125; } 
    // else if (romSize == 6) { romBanks = 128; }
    // if (romSize == 7) { romBanks = 256; }
    // if (romSize == 82) { romBanks = 72; }
    // if (romSize == 83) { romBanks = 80; }
    // if (romSize == 84) { romBanks = 96; }
    // ramBanks = 1; // Default 8K RAM
    // if (ramSize == 3) { ramBanks = 4; }

    // Can swap ram setup above with below if you hit any issues using new format and want to revert to old way

    // ROM banks
    romBanks = 2; // Default 32K
    if (romSize >= 1) { // Calculate rom size
      romBanks = 2 << romSize;
    }
    
    // RAM banks
    ramBanks = 0; // Default 0K RAM
    if (cartridgeType == 6) { ramBanks = 1; }
    if (ramSize == 2) { ramBanks = 1; }
    if (ramSize == 3) { ramBanks = 4; }
    if (ramSize == 4) { ramBanks = 16; }
    if (ramSize == 5) { ramBanks = 8; }
    
    // RAM end address
    if (cartridgeType == 6) { ramEndAddress = 0xA1FF; } // MBC2 512bytes (nibbles)
    if (ramSize == 1) { ramEndAddress = 0xA7FF; } // 2K RAM
    if (ramSize > 1) { ramEndAddress = 0xBFFF; } // 8K RAM

    // Cartridge Header
    Serial.println(gameTitle);
    Serial.println(cartridgeType);
    Serial.println(romSize);
    Serial.println(ramSize);
    Serial.println(logoCheck);
  }
  
  // Dump ROM
  else if (strstr(readInput, "READROM")) {
    // Serial.println("START"); <-- Should kill
    rd_wr_mreq_reset(); 
    unsigned int addr = 0;
    
    // Read x number of banks
    for (int bank = 1; bank < romBanks; bank++) {
      writeByte(0x2100, bank); // Set ROM bank;

      if (bank > 1) {
        addr = 0x4000;
      }
    
      // for (; addr <= 0x7FFF; addr++) {  
      //   shiftoutAddress(addr); // Shift out
      //   digitalWrite(rdPin, LOW); // RD on
      //   byte bval = readdataPins(); // Read data
      //   digitalWrite(rdPin, HIGH); // RD off 
      //   Serial.println(bval, DEC);
      // }

      // Old method above

      // Read up to 7FFF per bank
      while (addr <= 0x7FFF) {
        uint8_t readData[64];
        for (uint8_t i = 0; i < 64; i++) {
          readData[i] = readByte(addr+i);
        }
        
        Serial.write(readData, 64); // Send the 64 byte chunk
        addr += 64;
      }

    }
    //Serial.println("END"); <-- Should kill
  }
  
  // Read RAM
  else if (strstr(readInput, "READRAM")) {
    // MBC2 Fix (unknown why this fixes it, maybe has to read ROM before RAM?)
    readByte(0x0134);

    unsigned int addr = 0;
    unsigned int endaddr = 0;
    if (cartridgeType == 6 && ramSize == 0) { endaddr = 0xA1FF; } // MBC2 512bytes (nibbles)
    if (ramSize == 1) { endaddr = 0xA7FF; } // 2K RAM
    if (ramSize > 1) { endaddr = 0xBFFF; } // 8K RAM
    
    // Does cartridge have RAM
    if (endaddr > 0) {
      // Initialise MBC
      writeByte(0x0000, 0x0A);

      // Switch RAM banks
      for (int bank = 0; bank < ramBanks; bank++) {
        writeByte(0x4000, bank);

        // Read RAM
        for (addr = 0xA000; addr <= endaddr; addr = addr+64) {  
          uint8_t readData[64];
          for(int i = 0; i < 64; i++){
            readData[i] = readByte(addr+i);
          }
          Serial.write(readData, 64);
        }
      }
      
      // Disable RAM
      writeByte(0x0000, 0x00);
    }
  }
  
  // Write RAM
  else if (strstr(readInput, "WRITERAM")) {
    Serial.println("START");
    
    // MBC2 Fix (unknown why this fixes it, maybe has to read ROM before RAM?)
    readByte(0x0134);
    unsigned int addr = 0;
    unsigned int endaddr = 0;
    if (cartridgeType == 6 && ramSize == 0) { endaddr = 0xA1FF; } // MBC2 512bytes (nibbles)
    if (ramSize == 1) { endaddr = 0xA7FF; } // 2K RAM
    if (ramSize > 1) { endaddr = 0xBFFF; } // 8K RAM
    
    // Does cartridge have RAM
    if (endaddr > 0) {
      // Initialise MBC
      writeByte(0x0000, 0x0A);
      
      // Switch RAM banks
      for (int bank = 0; bank < ramBanks; bank++) {
        writeByte(0x4000, bank);
        
        // Write RAM
        for (addr = 0xA000; addr <= endaddr; addr=addr+64) {  
          Serial.println("NEXT"); // Tell Python script to send next 64bytes
          
          // Wait for serial input
          for (int i = 0; i < 64; i++) {
            while (Serial.available() <= 0) {
              delay(1);
            }
            
            // Read input
            uint8_t bval = 0;
            if (Serial.available() > 0) {
              char c = Serial.read();
              bval = (int) c;
            }
            
            // Write to RAM
            mreqPin_low;
       	    writeByte(addr+i, bval);
            asm volatile("nop");
            asm volatile("nop");
            asm volatile("nop");
            mreqPin_high;
          }
        }
      }
      
      // Disable RAM
      writeByte(0x0000, 0x00);
      Serial.flush(); // Flush any serial data that wasn't processed
    }
    Serial.println("END");
  }
}

uint8_t readByte(int address) {
  shiftoutAddress(address); // Shift out address

  mreqPin_low;
  rdPin_low;
  asm volatile("nop"); // Delay a little (minimum is 2 nops, using 3 to be sure)
  asm volatile("nop");
  asm volatile("nop");
  uint8_t bval = ((PINB << 6) | (PIND >> 2)); // Read data
  rdPin_high;
  mreqPin_high;
  
  return bval;
}

void writeByte(int address, uint8_t data) {
  // Set pins as outputs
  DDRB |= ((1<<PB0) | (1<<PB1)); // D8 & D9
  DDRD |= ((1<<PD2) | (1<<PD3) | (1<<PD4) | (1<<PD5) | (1<<PD6) | (1<<PD7)); // D2 to D7
  
  shiftoutAddress(address); // Shift out address
  
  // Clear outputs and set them to the data variable
  PORTB &= ~((1<<PB0) | (1<<PB1)); // D8 & D9
  PORTD &= ~((1<<PD2) | (1<<PD3) | (1<<PD4) | (1<<PD5) | (1<<PD6) | (1<<PD7)); // D2 to D7
  PORTD |= (data << 2);
  PORTB |= (data >> 6);
  
  // Pulse WR
  wrPin_low;
  asm volatile("nop");
  wrPin_high;
  
  // Set pins as inputs
  DDRB &= ~((1<<PB0) | (1<<PB1)); // D8 & D9
  DDRD &= ~((1<<PD2) | (1<<PD3) | (1<<PD4) | (1<<PD5) | (1<<PD6) | (1<<PD7)); // D2 to D7
}

// Use the shift registers to shift out the address
void shiftoutAddress(unsigned int shiftAddress) {
  for (int8_t i = 15; i >= 0; i--) {
    if (shiftAddress & (1<<i)) {
      dataPin_high;
    } 
    else {
      dataPin_low;
    }
    clockPin_high;
    clockPin_low;
  }
  
  latchPin_low;
  asm volatile("nop");
  latchPin_high;
}

// Turn RD, WR and MREQ to high so they are deselected (reset state)
void rd_wr_mreq_reset(void) {
  rdPin_high; // RD on
  wrPin_high; // WR on
  mreqPin_high; // MREQ on
}
