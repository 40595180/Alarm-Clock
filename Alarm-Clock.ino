int latchPin=8;
int clockPin=12;
int dataPin=11;

void setup() {
  pinMode(latchPin,OUTPUT);
  pinMode(clockPin,OUTPUT);
  pinMode(dataPin, OUTPUT);
  Serial.begin(9600);
}

void loop() {
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
        delay(1);
      };
    };
  };
}
