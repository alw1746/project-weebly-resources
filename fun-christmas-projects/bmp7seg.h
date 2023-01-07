/*
7 segment char bitmap in flash. 7 segment display pins must be soldered to mcu
PDx/PBx pins as shown below as each bit corresponds to an led segment. PD0-1
are not used as they are the TX,RX pins of the serial port. PORTD and PORTB
registers are used to set/clear multiple pins instead of digitalWrites for speed.

    segchr.segpin.mcupin
    
          a.7.PB0
         +--------+
f.9.PD7) |        | b.6.PB1
         |g.10.PD6|
         +--------+
e.1.PD2  |        | c.4.PD4
         |        |
         +--------+  *dp.5.PD5
          d.2.PD3
          
 Note: ATMEGA328P mcu is little-endian ie bit 0 is at the right-most position in a byte.
 Bit 7 is at the left-most position. This is importatnt for bit operations like shifting,
 masking and rotation where the direction and position is critical.
   1 byte  |_ _ _ _ _ _ _ _|
   bit pos  7 6 5 4 3 2 1 0
*/

#define NOTDEF  B00100000      //undefined char = '.'
const char bmpAscii[] PROGMEM =  {
NOTDEF,
NOTDEF,
NOTDEF,
NOTDEF,
NOTDEF,
NOTDEF,
NOTDEF,
NOTDEF,
NOTDEF,
NOTDEF,
NOTDEF,
NOTDEF,
NOTDEF,
NOTDEF,
NOTDEF,
NOTDEF,
NOTDEF,
NOTDEF,
NOTDEF,
NOTDEF,
NOTDEF,
NOTDEF,
NOTDEF,
NOTDEF,
NOTDEF,
NOTDEF,
NOTDEF,
NOTDEF,
NOTDEF,
NOTDEF,
NOTDEF,
NOTDEF,
B00000000, //  SPACE
NOTDEF,    // !
NOTDEF,    // "
NOTDEF,    // #
NOTDEF,    // $
NOTDEF,    // %
NOTDEF,    // &
NOTDEF,    // '
NOTDEF,    // (
NOTDEF,    // )
NOTDEF,    // *
NOTDEF,    // +
NOTDEF,    // ,
B01000000, // -
B00100000, // .
NOTDEF,    // /
B10011111, // 0
B00010010, // 1
B01001111, // 2
B01011011, // 3
B11010010, // 4
B11011001, // 5
B11011101, // 6
B00010011, // 7
B11011111, // 8
B11011011, // 9
NOTDEF,    // :
NOTDEF,    // ;
NOTDEF,    // <
B01001000, // =
NOTDEF,    // >
NOTDEF,    // ?
NOTDEF,    // @
B11010111, // A
B11011100, // B
B10001101, // C
B01011110, // D
B11001101, // E
B11000101, // F
B11011011, // G
B11010110, // H
B10000100, // I
B00011110, // J
NOTDEF,    // K
B10001100, // L
NOTDEF,    // M
B10010111, // N
B10011111, // O
B11000111, // P
NOTDEF,    // Q
B01000100, // R
B11011001, // S
B10000101, // T
B10011110, // U
NOTDEF,    // V
NOTDEF,    // W
NOTDEF,    // X
B11011010, // Y
NOTDEF,    // Z
B10001101, // [
NOTDEF,    // backslash
B00011011, // ]
NOTDEF,    // ^
B00001000, // _
NOTDEF,    // `
B11010111, // a
B11011100, // b
B01001100, // c
B01011110, // d
B11001111, // e
B11000101, // f
B11011011, // g
B11010100, // h
B00000100, // i
B00011110, // j
NOTDEF,    // k
B10000100, // l
NOTDEF,    // m
B01010100, // n
B01011100, // o
B11000111, // p
NOTDEF,    // q
B01000100, // r
B11011001, // s
B11000100, // t
B00011100, // u
NOTDEF,    // v
NOTDEF,    // w
NOTDEF,    // x
B11011010, // y
NOTDEF,    // z
B10001101, // {
B10000100, // |
B00011011, // }
NOTDEF     // ~
};
int bmpAsciiLen=sizeof(bmpAscii)/sizeof(bmpAscii[0]);

const char bmpSnake[] PROGMEM =  {
/*
//double snake
B00011000,
B01010000,
B11000000,
B10000001,
B00000011,
B01000010,
B01000100,
B00001100
*/
//single snake
B00001000,
B00010000,
B00000010,
B00000001,
B10000000,
B00000100,
};
int bmpSnakeLen=sizeof(bmpSnake)/sizeof(bmpSnake[0]);

//Output Ascii char. Bitmap 0-1 = PORTB 0-1, 2-7 = PORTD 2-7.
//usage: printChar('H');
void printChar(char chr) {
  uint8_t pdreg=pgm_read_byte(bmpAscii+(byte)chr);    //get char bitmap
  PORTB=PORTB & B11111100;          //clear PB0-1
  PORTD=PORTD & B00000011;          //clear PD2-7
  PORTB=PORTB | pdreg & B00000011;  //set PB0-1 from bitmap 0-1,mask off 2-7
  PORTD=PORTD | pdreg & B11111100;  //set PD2-7 from bitmap 2-7,mask off 0-1
}

//Output numeric char. Bitmap 0-1 = PORTB 0-1, 2-7 = PORTD 2-7.
//usage: printChar(6);
void printDigit(uint8_t num) {
  if (num >= 0 && num <= 9) {
    uint8_t pdreg=pgm_read_byte(bmpAscii+(byte)('0')+num);
    PORTB=PORTB & B11111100;
    PORTD=PORTD & B00000011;
    PORTB=PORTB | pdreg & B00000011;
    PORTD=PORTD | pdreg & B11111100;
  }
}

//Output Ascii by array index. Bitmap 0-1 = PORTB 0-1, 2-7 = PORTD 2-7.
//usage: printChar(i);
void printIndex(uint8_t idx) {
  uint8_t pdreg=pgm_read_byte(bmpAscii+idx);
  PORTB=PORTB & B11111100;
  PORTD=PORTD & B00000011;
  PORTB=PORTB | pdreg & B00000011;
  PORTD=PORTD | pdreg & B11111100;
}

//Output snake by array index. Bitmap 0-1 = PORTB 0-1, 2-7 = PORTD 2-7.
//usage: printChar(i);
void printSnake(uint8_t idx) {
  uint8_t pdreg=pgm_read_byte(bmpSnake+idx);
  PORTB=PORTB & B11111100;
  PORTD=PORTD & B00000011;
  PORTB=PORTB | pdreg & B00000011;
  PORTD=PORTD | pdreg & B11111100;
}
