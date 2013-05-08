#include <Wire.h>

int latchPin=8;
int clockPin=12;
int dataPin=11;
byte minute1, minute0, hour1, hour0;

void setup() {
  pinMode(latchPin,OUTPUT);
  pinMode(clockPin,OUTPUT);
  pinMode(dataPin, OUTPUT);
  Serial.begin(9600);
  //set clock out t0 1hz
  byte command=B11000011;
  Wire.begin();
  Wire.beginTransmission(0x51);
  Wire.write(0x0D);
  Wire.write(B10000011);
  Wire.endTransmission();
  Wire.beginTransmission(0x51);
  Wire.write(0x02);
  Wire.write(0x00);
  Wire.write(B00010000);
  Wire.write(B00000100);
  Wire.endTransmission();
}

void loop() {
  byte time_raw[3];
  //each byte contains a ready to send code
  byte time_formatted[4];
  //set to timer registers
  Wire.beginTransmission(0x51);
  Wire.write(0x03);
  Wire.endTransmission();
  //read timer registers
  Wire.requestFrom(0x51,2);
  int index = 0;
  while(Wire.available()) {
    time_raw[index]=Wire.read();
    index++
  };
  time_formatted[0]=(B00010000)&(time_raw[0]&B00001111);
  for (byte displayNumber=0;displayNumber<10;displayNumber++) {
    for(int i=0;i<100;i++) {
      for(byte displaySelect=0;displaySelect<4;displaySelect++) {
        byte output=1;
        output = output << displaySelect+4;
        //Serial.println("outputShifted");
        //Serial.println(output,BIN);
        output = output | displayNumber;
        digitalWrite(latchPin, LOW);
        shiftOut(dataPin, clockPin, MSBFIRST, output);
        digitalWrite(latchPin,HIGH);
        //Serial.println("final output");
        //Serial.println(output, BIN);
        delay(4);
      };
    };
  };
}
