#include <Arduino.h>
#include <U8g2lib.h>
#include "make_logo.h"

#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif
#ifdef U8X8_HAVE_HW_I2C
#include <Wire.h>
#endif

U8G2_SSD1306_64X48_ER_F_4W_SW_SPI u8g2(U8G2_R0, 9, 8, 11, 10, 12);

int PH1, PH2, PH3, PH4;
int lightSum = 0;
int PHEN = 38;
int menu = 1;
int curr_menu = 1;
int prog_start = 0;
int menu_items = 3;

#define APHASE 4
#define APWM 3
#define BPHASE 5
#define BPWM 6
#define left_b 7
#define right_b 2

void setup() {
  // put your setup code here, to run once:
  pinMode(A1, INPUT);
  pinMode(A2, INPUT);
  pinMode(A3, INPUT);
  pinMode(A4, INPUT);
  pinMode(left_b, INPUT_PULLUP);
  pinMode(right_b, INPUT_PULLUP);

  pinMode(PHEN, OUTPUT);
  digitalWrite(PHEN, LOW);

  SerialUSB.begin(115200);
  delay(100);
  u8g2.begin();

  u8g2.setFlipMode(0);
  u8g2.setFont(u8g2_font_6x12_tr);
  u8g2.setColorIndex(1);

  u8g2.firstPage();
  do {
    drawLogo();
  } while ( u8g2.nextPage() );
  delay(2000);
  u8g2.firstPage();
  do {
    intro();
  } while ( u8g2.nextPage() );
  attachInterrupt(left_b, menu_change, LOW);
  attachInterrupt(right_b, load_prog, LOW);
  delay(1000);
  while (prog_start == 0) {
    u8g2.firstPage();
    do {
      menu_select();
    } while ( u8g2.nextPage() );
  }
  u8g2.clear();
  delay(1000);

}

void loop() {

  //motorTesting() //Run this to test the direction of your motors;

  switch (curr_menu) {
    case 1:
      simpleLightAvoid();
      break;
    case 2:
      
      break;
    case 3:
    
      break;
    default:
      break;

  }
}

void simpleLightAvoid() {
  PH1 = analogRead(A1);
  PH2 = analogRead(A2);
  PH3 = analogRead(A3);
  PH4 = analogRead(A4);
  lightSum = PH1 + PH2 + PH3 + PH4;
  PH1 = map(PH1, 400, 1024, 0, 48);
  PH2 = map(PH2, 400, 1024, 0, 64);
  PH3 = map(PH3, 400, 1024, 0, 64);
  PH4 = map(PH4, 400, 1024, 0, 48);
  //motorTesting();
  u8g2.firstPage();
  do {
    //drawBars(PH1,PH2,PH3,PH4);
    drawBallDir(PH1, PH2, PH3, PH4);
  } while ( u8g2.nextPage() );
}

void drawBallDir(int P1, int P2, int P3, int P4) {
  int pull_x = 0;
  int pull_y = 0;
  pull_x = ((P2 - P3) / 2) + 32;
  pull_y = ((P1 - P4) / 2) + 24;
  u8g2.drawCircle(pull_x, pull_y, 4);
  ballMotorControl(pull_x, pull_y);
}

void ballMotorControl(int mxd, int myd) {
  int  Aspeed = 50;
  int  Bspeed = 50;
  int power = map(myd, 48, 0, 0, 100);
  int offset = map(mxd, 0, 64, 50, -50);
  Aspeed = power - offset;
  Bspeed = power + offset;
  if (Aspeed < 60 && Aspeed > 40) {
    Aspeed = 50;
  }
  if (Bspeed < 60 && Bspeed > 40) {
    Bspeed = 50;
  }
  if (lightSum >3500){
    motorA(65);
    motorB(65);
  }
  else{
  motorA(Aspeed);
  motorB(Bspeed);
  }
}

void motorTesting() {
  SerialUSB.println("Driving Forward");
  motorA(70);
  motorB(70);
  delay(1000);
  SerialUSB.println("Stopping");
  motorA(50);
  motorB(50);
  delay(1000);
  SerialUSB.println("Turning");
  motorA(70);
  motorB(20);

  delay(500);
  motorA(30);
  motorB(30);
  delay(1000);
  motorA(50);
  motorB(50);
  while (1);
}

void drawLogo(void)
{
  uint8_t mdy = 0;
  if ( u8g2.getDisplayHeight() < 59 )
    mdy = 5;
  u8g2.drawBitmap(0, 0, logo_width, logo_height, makelogo_bits);
}

void drawBars(int P1, int P2, int P3, int P4) {
  u8g2.drawBox(1, 1, 10, P1);
  u8g2.drawBox(15, 1, 10, P2);
  u8g2.drawBox(30, 1, 10, P3);
  u8g2.drawBox(45, 1, 10, P4);
  //u8g.print("m/s");

}



void motorA(int percent) {
  int maxSpeed = 90;
  int minSpeed = 10;
  int dir = 0;
  if (percent < 50) {
    dir = 0;
  }
  if (percent > 50) {
    dir = 1;
  }
  if (dir == 1) {
    pinMode(APHASE, INPUT);
    pinMode(APWM, INPUT);
    pinMode(APHASE, OUTPUT);
    pinMode(APWM, OUTPUT);
    digitalWrite(APHASE, LOW);
    int drive = map(percent, 51, 100, 0, 255);
    drive = constrain(drive, 0, 1023);
    //SerialUSB.print("Driving Fore: ");
    //SerialUSB.println(drive);
    analogWrite(APWM, drive);
  }
  if (dir == 0) {
    pinMode(APHASE, INPUT);
    pinMode(APWM, INPUT);
    pinMode(APHASE, OUTPUT);
    pinMode(APWM, OUTPUT);
    digitalWrite(APHASE, HIGH);
    int drive = map(percent, 49, 0, 0, 255);
    drive = constrain(drive, 0, 1023);
    SerialUSB.print("Driving Back: ");
    SerialUSB.println(drive);
    analogWrite(APWM, drive);
  }
  if (percent == 50) {
    pinMode(APHASE, INPUT);
    pinMode(APWM, INPUT);
  }
}

void motorB(int percent) {
  int maxSpeed = 85;
  int minSpeed = 45;
  int dir = 0;
  if (percent < 50) {
    dir = 0;
  }
  if (percent > 50) {
    dir = 1;
  }
  if (dir == 1) {
    pinMode(BPHASE, INPUT);
    pinMode(BPWM, INPUT);
    pinMode(BPHASE, OUTPUT);
    pinMode(BPWM, OUTPUT);
    digitalWrite(BPHASE, HIGH);
    int drive = map(percent, 51, 100, 0, 255);
    drive = constrain(drive, 0, 1023);
    SerialUSB.print("Driving Fore: ");
    SerialUSB.println(drive);
    analogWrite(BPWM, drive);
  }
  if (dir == 0) {
    pinMode(BPHASE, INPUT);
    pinMode(BPWM, INPUT);
    pinMode(BPHASE, OUTPUT);
    pinMode(BPWM, OUTPUT);
    digitalWrite(BPHASE, LOW);
    int drive = map(percent, 49, 0, 0, 255);
    drive = constrain(drive, 0, 1023);
    analogWrite(BPWM, drive);
  }
  if (percent == 50) {
    pinMode(BPHASE, INPUT);
    pinMode(BPWM, INPUT);
  }
}


void intro() {

  u8g2.drawStr( 1, 10, "ANN BOT");
  u8g2.drawStr( 1, 25, "V0.1");
}


void menu_select() {
  menu_circle();
  u8g2.drawStr( 7, 10, "1)Simple");
  u8g2.drawStr( 7, 25, "2)Blank");
  u8g2.drawStr( 7, 40, "3)Blank");
}

void menu_circle() {
  int yloc = curr_menu * 15;
  u8g2.drawCircle(3, yloc - 9, 2);
}

void menu_change(void)
{
  static unsigned long last_interrupt_time = 0;
  unsigned long interrupt_time = millis();
  // If interrupts come faster than 200ms, assume it's a bounce and ignore
  if (interrupt_time - last_interrupt_time > 50)
  {
    int buttState = digitalRead(left_b);
    if (buttState == 0) {
      curr_menu++;
      if (curr_menu > menu_items) {
        curr_menu = 1;
      }
    }
  }
  last_interrupt_time = interrupt_time;
}

void load_prog() {
  prog_start = 1;
}

