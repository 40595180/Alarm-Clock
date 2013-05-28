#include <Wire.h>

//rtc defines
#define AIE 1
#define TIE 0

//pin definitions
int latchPin=8;
int clockPin=12;
int dataPin=11;
int alarmPin=4;
int minutePin=3;
int hourPin=5;
int snoozePin=6;

//global bytes
byte time_raw[2]; //raw read from Real Time Clock
byte time_formatted[4]; //high 4 bits contains digit to activate, low 4 bits contains number
short unsigned int currentDigit=0; //current digit to display, it is incremented during the timer 1 ISR

//testing
unsigned int count1=0;
unsigned int count2=0;


void alarm() {
  interrupts();
  byte registercontents;
  Wire.beginTransmission(0x51);
  Wire.write(0x01);
  Wire.endTransmission();
  Wire.requestFrom(0x51,1);
  while(Wire.available()) {
      registercontents = Wire.read();
  }
  registercontents &= B11110111;
  Wire.beginTransmission(0x51);
  Wire.write(0x01);
  Wire.write(registercontents);
  Wire.endTransmission();

  digitalWrite(alarmPin, HIGH);
  while(digitalRead(snoozePin)==HIGH) delay(100);
  digitalWrite(alarmPin, LOW);
}

void setup() {
  pinMode(latchPin,OUTPUT);
  pinMode(clockPin,OUTPUT);
  pinMode(dataPin, OUTPUT);
  pinMode(alarmPin,OUTPUT);
  //minute and hour setting things.
  pinMode(minutePin,INPUT_PULLUP);
  pinMode(hourPin,INPUT_PULLUP);
  //"snooze" button
  pinMode(snoozePin,INPUT_PULLUP);
  //this is attached to the INT pin on the rtc
  attachInterrupt(0,alarm,FALLING);
  //for debugging purposes...
  Serial.begin(9600);
  //set clock pin output to 1hz
  byte command=B11000011;
  Wire.begin();
  Wire.beginTransmission(0x51);
  Wire.write(0x01);
  //ennable alarm
  Wire.write(1<<AIE);
  Wire.endTransmission();
  Wire.beginTransmission(0x51);
  Wire.write(0x0D);
  Wire.write(B10000011);
  Wire.endTransmission();
  //Set Time
  Wire.beginTransmission(0x51);
  Wire.write(0x02);
  Wire.write(B01010101);
  Wire.write(B00100000);
  Wire.write(B00000100);
  Wire.endTransmission();
  //set alarm
  Wire.beginTransmission(0x51);
  Wire.write(0x09);
  Wire.write(B00100001);
  Wire.write(B00000100);
  Wire.endTransmission();
  //do initial read of Real Time Clock and registers
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
  
  //set up timer and interrupts
  noInterrupts();
  //this timer will control the periodic display updates
  TCCR1A=0;
  TCCR1B=0;
  TCNT1=0;
  
  OCR1A=250; //16MHz/256 prescaler/250Hz (.004 s period);
  TCCR1B |= (1<<WGM12); //set to CTC with OCR1A max
  TCCR1B |= (1<<CS12); //256 prescaler
  TIMSK1 |= (1<<OCIE1A); //enable timer compare interrupt
  
  //this timer will control periodic time polling
  TCCR2A=0;
  TCCR2B=0;
  TCNT2=0;
  
  OCR2A=6250; //16MHz/256 prescaler/10Hz (.1 s period)
  TCCR2A |= 2; //set to CTC mode
  TCCR2B |= (1<<CS22) | (1<<CS21); //set to 256 prescaler
  TIMSK2 |= (1<<OCIE2A);
  interrupts();
}

ISR(TIMER1_COMPA_vect) {
  count1++;
  digitalWrite(latchPin, LOW);
  shiftOut(dataPin, clockPin, MSBFIRST, time_formatted[currentDigit++]);
  if(currentDigit==4)currentDigit=0;
  digitalWrite(latchPin,HIGH);
};

ISR(TIMER2_COMPA_vect, ISR_NOBLOCK) {
  count2++;
  //each byte contains a ready to send code
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
};

void incrementMinute() {
  byte minute;
  Wire.beginTransmission(0x51);
  Wire.write(0x03);
  Wire.endTransmission();
  //get the minutes
  Wire.requestFrom(0x51,1);
  while(Wire.available()) {
    minute=Wire.read();
  };
  byte minuteLower= minute & B00001111;
  byte minuteUpper= (minute & B01110000)>>4;
  byte totalMinute= (minuteUpper*10) + minuteLower;
  totalMinute = (totalMinute + 1)%60;
  minute = ((totalMinute/10)<<4)|(totalMinute%10);
  Wire.beginTransmission(0x51);
  Wire.write(0x03);
  Wire.write(minute);
  Wire.endTransmission();
};

void incrementHour() {
  byte hour;
  Wire.beginTransmission(0x51);
  Wire.write(0x04);
  Wire.endTransmission();
  //get the hours
  Wire.requestFrom(0x51,1);
  while(Wire.available()) {
    hour=Wire.read();
  };
  byte minuteLower= hour & B00001111;
  byte minuteUpper= (hour & B00110000)>>4;
  byte totalMinute= (minuteUpper*10) + minuteLower;
  totalMinute = (totalMinute + 1)%24;
  if(totalMinute==0) {totalMinute=24;};
  hour = ((totalMinute/10)<<4)|(totalMinute%10);
  Wire.beginTransmission(0x51);
  Wire.write(0x04);
  Wire.write(hour);
  Wire.endTransmission();
}

void loop() {
  if(digitalRead(minutePin)==LOW) incrementMinute();
  if(digitalRead(hourPin)==LOW) incrementHour();
  delay(200);
}
