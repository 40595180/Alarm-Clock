#include <Wire.h>

int latchPin=8;
int clockPin=12;
int dataPin=11;
byte minute1, minute0, hour1, hour0;

void setup() {
  pinMode(latchPin,OUTPUT);
  pinMode(clockPin,OUTPUT);
  pinMode(dataPin, OUTPUT);
  //for debugging purposes...
  Serial.begin(9600);
  //set clock pin output to 1hz
  byte command=B11000011;
  Wire.begin();
  Wire.beginTransmission(0x51);
  Wire.write(0x0D);
  Wire.write(B10000011);
  Wire.endTransmission();
  //also for debugging
  Wire.beginTransmission(0x51);
  Wire.write(0x02);
  Wire.write(0x00);
  Wire.write(B00100000);
  Wire.write(B00000100);
  Wire.endTransmission();
  
  //set up timer and interrupts
  noInterrupts();
  TCCR1A=0;
  TCCR1A=0;
  TCNT1=0;
  
  OCR1A=250; //16MHz/256 prescaler/250Hz (.004 s period);
  TCCR1B |= (1<<WGM12); //CTC
  TCCR1B |= (1<<CS12); //256 prescaler
  TIMSK1 |= (1<<OCIE1A); //enable timer compare interrupt
  interrupts();
}

void loop() {
  byte time_raw[2];
  //each byte contains a ready to send code
  byte time_formatted[4];
  //set address to timer registers
  Wire.beginTransmission(0x51);
  Wire.write(0x03);
  Wire.endTransmission();
  //read timer registers
  Wire.requestFrom(0x51,2);
  int index = 0;
  while(Wire.available()) {
    time_raw[index]=Wire.read();
    index++;
  };
  //format the time correctly by picking apart register contents
  //first four bits determine which integer lights up. The final byte
  //resembles lightSelection<<4 | numberInBinary
  //Minute 0
  time_formatted[0]=(B10000000) | (time_raw[0]&B00001111);
  //Minute 1
  time_formatted[1]=(B01000000) | ((time_raw[0]&B01110000)>>4);
  //Hour 0
  time_formatted[2]=(B00100000) | (time_raw[1]&B00001111);
  //Hour 1
  time_formatted[3]=(B00010000) | ((time_raw[1]&B00110000)>>4);
  //shift each formatted bit out to display, 4ms in between.
  //later, this will be moved to timer-based itnerrupts.
  for(int i=0;i<10;i++) {
    for(int displaySelect=0;displaySelect<4;displaySelect++) {
      digitalWrite(latchPin, LOW);
      shiftOut(dataPin, clockPin, MSBFIRST, time_formatted[displaySelect]);
      digitalWrite(latchPin,HIGH);
      delay(4);
    };
  };
}
