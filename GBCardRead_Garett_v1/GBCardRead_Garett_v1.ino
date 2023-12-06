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

void setup() {
  Serial.begin(460800);
  pinMode(latchPin, OUTPUT);
  pinMode(clockPin, OUTPUT);
  pinMode(dataPin, OUTPUT);
  pinMode(rdPin, OUTPUT);
  pinMode(wrPin, OUTPUT);
  pinMode(mreqPin, OUTPUT);
  for (int i = 2; i <= 9; i++) {
    pinMode(i, INPUT);
  }
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
      char headerChar = (char) readbank0Address(addr);
      if ((headerChar >= 0x30 && headerChar <= 0x57) || // 0-9
            (headerChar >= 0x41 && headerChar <= 0x5A) || // A-Z
            (headerChar >= 0x61 && headerChar <= 0x7A)) { // a-z
              gameTitle[(addr-0x0134)] = headerChar;
        }
    }
    gameTitle[16] = '\0';

    // Nintendo Logo Check
    uint8_t logoCheck = 1;
    uint8_t x = 0;
    for (uint16_t romAddress = 0x0104; romAddress <= 0x133; romAddress++) {
      if (nintendoLogo[x] != readbank0Address(romAddress)) {
        logoCheck = 0;
        break;
      }
      x++;
    }

    cartridgeType = readbank0Address(0x0147);
    romSize = readbank0Address(0x0148);
    ramSize = readbank0Address(0x0149);

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
      selectROMbank(bank);

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
          readData[i] = read_byte(addr+i);
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
    shiftoutAddress(0x0134);
    
    Serial.println("START");
    unsigned int addr = 0;
    unsigned int endaddr = 0;
    if (cartridgeType == 6 && ramSize == 0) { endaddr = 0xA1FF; } // MBC2 512bytes (nibbles)
    if (ramSize == 1) { endaddr = 0xA7FF; } // 2K RAM
    if (ramSize == 2 || ramSize == 3) { endaddr = 0xBFFF; } // 8K RAM
    
    // Does cartridge have RAM
    if (endaddr > 0) {
    
      // Initialise MBC
      shiftoutAddress(0x0000);
      bankSelect(0x0A);
      digitalWrite(wrPin, LOW); // WR on
      digitalWrite(wrPin, HIGH); // WR off
      
      // Switch RAM banks
      for (int bank = 0; bank < ramBanks; bank++) {
        shiftoutAddress(0x4000);// Shift out
        bankSelect(bank); // Select bank
        digitalWrite(wrPin, LOW); // WR on
        digitalWrite(wrPin, HIGH); // WR off
        
        // Turn outputs off and change back to inputs
        for (int l = 2; l <= 9; l++) {
          digitalWrite(l, LOW);
          pinMode(l, INPUT);
        }
        
        // Read RAM
        for (addr = 0xA000; addr <= endaddr; addr++) {  
          shiftoutAddress(addr); // Shift out
          digitalWrite(mreqPin, LOW); // MREQ on
          digitalWrite(rdPin, LOW); // RD on
          byte bval = readdataPins(); // Read data pins
          if (ramSize == 0) { bval |= 0x0F<<4; } // For MBC2
          digitalWrite(mreqPin, HIGH); // MREQ off
          digitalWrite(rdPin, HIGH); // RD off
          Serial.println(bval, DEC);
        }
      }
      
      // Disable RAM
      shiftoutAddress(0x0000);
      bankSelect(0x00);
      digitalWrite(wrPin, LOW); // WR on
      digitalWrite(wrPin, HIGH); // WR off
    
      // Set pins back to inputs
      for (int l = 2; l <= 9; l++) {
        pinMode(l, INPUT);
      }
    }
    Serial.println("END");
  }
  
  // Write RAM
  else if (strstr(readInput, "WRITERAM")) {
  
    // MBC2 Fix (unknown why this fixes it, maybe has to read ROM before RAM?)
    shiftoutAddress(0x0134);
    
    Serial.println("START");
    unsigned int addr = 0;
    unsigned int endaddr = 0;
    if (cartridgeType == 6 && ramSize == 0) { endaddr = 0xA1FF; } // MBC2 512bytes (nibbles)
    if (ramSize == 1) { endaddr = 0xA7FF; } // 2K RAM
    if (ramSize == 2 || ramSize == 3) { endaddr = 0xBFFF; } // 8K RAM
    
    // Does cartridge have RAM
    if (endaddr > 0) {
    
      // Initialise MBC
      shiftoutAddress(0x0000);
      bankSelect(0x0A);
      digitalWrite(wrPin, LOW); // WR on
      digitalWrite(wrPin, HIGH); // WR off
      
      // Switch RAM banks
      for (int bank = 0; bank < ramBanks; bank++) {
        shiftoutAddress(0x4000);// Shift out
        bankSelect(bank); // Select bank
        digitalWrite(wrPin, LOW); // WR on
        digitalWrite(wrPin, HIGH); // WR off
        
        // Write RAM
        for (addr = 0xA000; addr <= endaddr; addr++) {  
          shiftoutAddress(addr); // Shift out
          digitalWrite(mreqPin, LOW); // MREQ on
          digitalWrite(wrPin, LOW); // WR on
          
          // Wait for serial input
          int waiting = 0;
          while (Serial.available() <= 0) {
            delay(1);
            if (waiting == 0) {  
              Serial.println("NEXT"); // Tell Python script to send next 64bytes
              waiting = 1;
            }
          }
    
          // Decode input
          byte bval = 0;
          if (Serial.available() > 0) {
            char c = Serial.read();
            bval = (int) c;
          }
          
          // Read the bits in the received character and turn on the 
          // corresponding D0-D7 pins
          for (int z = 9; z >= 2; z--) {
            if (bitRead(bval, z-2) == HIGH) {
              digitalWrite(z, HIGH);
            }
            else {
              digitalWrite(z, LOW);
            }
          }
          
          digitalWrite(mreqPin, HIGH); // MREQ off
          digitalWrite(wrPin, HIGH); // WR off
        }
      }
      
      // Disable RAM
      shiftoutAddress(0x0000);
      bankSelect(0x00);
      digitalWrite(wrPin, LOW); // WR on
      digitalWrite(wrPin, HIGH); // WR off
      
      // Set pins back to inputs
      for (int l = 2; l <= 9; l++) {
        pinMode(l, INPUT);
      }
      
      Serial.flush(); // Flush any serial data that wasn't processed
    }
    Serial.println("END");
  }
}

// Read_byte
uint8_t read_byte(uint16_t address) {
  shiftoutAddress(address); // Shift out address
  
  digitalWrite(mreqPin, LOW); // MREQ on
  digitalWrite(rdPin, LOW); // RD on
  asm volatile("nop"); // Delay a little (minimum is 2 nops, using 3 to be sure)
  asm volatile("nop");
  asm volatile("nop");
  byte bval = readdataPins(); // Read data
  digitalWrite(rdPin, HIGH); // RD off 
  digitalWrite(mreqPin, HIGH); // MREQ off
  
  return bval;
}

// Select the ROM bank by writing the bank number on the data pins
void selectROMbank(int bank) {
  shiftoutAddress(0x2100); // Shift out
  bankSelect(bank); // Select the bank
  digitalWrite(wrPin, LOW); // WR on
  digitalWrite(wrPin, HIGH); // WR off
   
  // Reset outputs to LOW and change back to inputs
  for (int i = 2; i <= 9; i++) {
    digitalWrite(i, LOW);
    pinMode(i, INPUT);
  }
}

// Read bank 0 address
int readbank0Address(unsigned int address) {
  shiftoutAddress(address); // Shift out address
  digitalWrite(rdPin, LOW); // RD on
  byte bval = readdataPins(); // Read data
  digitalWrite(rdPin, HIGH); // RD off 
  return bval;
}

// Use the shift registers to shift out the address
void shiftoutAddress(unsigned int shiftAddress) {
  digitalWrite(latchPin, LOW);
  shiftOut(dataPin, clockPin, MSBFIRST, (shiftAddress >> 8));
  shiftOut(dataPin, clockPin, MSBFIRST, (shiftAddress & 0xFF));
  digitalWrite(latchPin, HIGH);
  delayMicroseconds(50);
}

// Turn RD, WR and MREQ to high so they are deselected (reset state)
void rd_wr_mreq_reset(void) {
  digitalWrite(rdPin, HIGH); // RD off
  digitalWrite(wrPin, HIGH); // WR off
  digitalWrite(mreqPin, HIGH); // MREQ off
}

// Turn RD, WR and MREQ off as no power should be applied to GB Cart
void rd_wr_mreq_off(void) {
  digitalWrite(rdPin, LOW); // RD off
  digitalWrite(wrPin, LOW); // WR off
  digitalWrite(mreqPin, LOW); // MREQ off
}

// Turn on the data lines corresponding to the bank number
void bankSelect(int bank) {
  
  // Change to outputs
  for (int i = 2; i <= 9; i++) {
    pinMode(i, OUTPUT);
  }
  
  // Read bits in bank variable
  for (int z = 9; z >= 2; z--) {
    if (bitRead(bank, z-2) == HIGH) {
      digitalWrite(z, HIGH);
    }
    else {
      digitalWrite(z, LOW);
    }
  }
}

// Read data pins
byte readdataPins() {
  
  // Might need this but don't think so. Seems like a waste of time since
  // pins SHOULD already be set to inputs....
  //
  // Change to inputs
  // for (int i = 2; i <= 9; i++) {
  //   pinMode(i, INPUT);
  // }
  
  // Read data pins
  byte bval = 0;
  for (int z = 9; z >= 2; z--) {
    if (digitalRead(z) == HIGH) {
      bitWrite(bval, (z-2), HIGH);
    }
  }
  
  return bval;
}