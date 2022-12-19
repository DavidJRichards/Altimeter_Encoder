/* Gillham altitude decoder
 * conversion functions from https://drive.google.com/drive/folders/0B6ytVYfsfAiNNzE0Njk3ZTItODMxYS00NThlLTk4ZGEtOWE5Y2E1OTRhYjk1
 * test vectors, see https://www.avionictools.com/graycalc.php
 * djrm 15 Dec 2022
 * 
 */

#include <max7219.h>
#define LEFT 0
#define RIGHT 1
MAX7219 max7219;

#include <light_CD74HC4067.h>
           // s0 s1 s2 s3: select pins
CD74HC4067 mux(4, 5, 6, 7);  // create a new CD74HC4067 object with its four select lines
const int signal_pin = 8; // Pin Connected to Sig pin of CD74HC4067

#define int32 long
#define int16 int

//#define GILLHAM_VALUE 0b1 // -1200
//#define GILLHAM_VALUE 0b11011 // -100
//#define GILLHAM_VALUE 0b11010 //0
//#define GILLHAM_VALUE 0b10010 //500
#define GILLHAM_VALUE 0b1010010 // 5000
//#define GILLHAM_VALUE 0b1100000001 // 30800

unsigned int GrayToBinary(unsigned int num)
{
  unsigned int temp;

  temp = num ^ (num>>8);
  temp ^= (temp>>4);
  temp ^= (temp>>2);
  temp ^= (temp>>1);
  return temp;
}

signed int32 GillhamToAltitude( int16 GillhamValue )
// Data must be in following order (MSB to LSB)
// D1 D2 D4 A1 A2 A4 B1 B2 B4 C1 C2 C4

{ 
  signed int32 Result; 
  int16 FiveHundreds;
  int16 OneHundreds;

  // Convert Gillham value using gray code to binary conversion algorithm. 
  // Get rid of Hundreds (lower 3 bits).
  FiveHundreds = GillhamValue >> 3;
  
  // Strip off Five Hundreds leaving lower 3 bits.  
  OneHundreds = GillhamValue & 0x07;

  FiveHundreds = GrayToBinary(FiveHundreds);
  OneHundreds  = GrayToBinary(OneHundreds);

  // Check for invalid codes.
  if (OneHundreds == 5 || OneHundreds == 6 || OneHundreds == 0)
  {
    Result = -9;
    return Result;
  }
  
  // Remove 7s from OneHundreds. 
  if (OneHundreds == 7) OneHundreds = 5;
  
  // Correct order of OneHundreds.
  if (FiveHundreds % 2) OneHundreds = 6 - OneHundreds;

    // Convert to feet and apply altitude datum offset.
    Result = (signed int32)((FiveHundreds * 500) + (OneHundreds *100)) - 1300; 

    return Result; 
}

int16 read_mpx(void)
{
  int16 val=0;
  for (int i = 0; i < 16; i++) {
    mux.channel(i);
    val = val | (digitalRead(signal_pin)?0:(1<<i));   // Read bit into result value
  }
  return val;
}

void setup() {
  Serial.begin(9600);
  max7219.Begin();
  max7219.Clear();
  max7219.DisplayText("Boro alt", LEFT);
  pinMode(signal_pin, INPUT_PULLUP); // Set as input for reading through signal pin
  delay(1000);
}

void loop() {
 char dispbuf[12];
 long altitude;
 long gillham;

// gillham=GILLHAM_VALUE;
 gillham=read_mpx();
 Serial.print("0b");
 Serial.print(gillham, BIN);
 Serial.print(",");
 altitude=GillhamToAltitude(gillham);
 Serial.print(altitude);
 Serial.println("ft");

  for (int i = 7; i >= 0; i--) {
    dispbuf[i] =  (gillham&(1<<i)?'1':'0');   // Read bit into result value
  }
  dispbuf[8]='\0';
  Serial.println(dispbuf);

#ifndef DEBUG
  sprintf(dispbuf,"Alt%5d",altitude);
#endif

  //Display decimals right justified
  max7219.Clear();
  
  max7219.DisplayText(dispbuf, RIGHT);
  delay(1000);

  //Display decimals left justified
}
