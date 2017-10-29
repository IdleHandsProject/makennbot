#include <Arduino.h>
#include <U8g2lib.h>
#include "make_logo.h"

#include <math.h>
#define DEBUG

#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif
#ifdef U8X8_HAVE_HW_I2C
#include <Wire.h>
#endif

U8G2_SSD1306_64X48_ER_F_4W_SW_SPI u8g2(U8G2_R0, 9, 8, 11, 10, 12);

int PH1, PH2, PH3, PH4;
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

#define LEDYEL 25
#define LEDRED 26

/******************************************************************
   Network Configuration - customized per network
 ******************************************************************/

const int PatternCount = 16;
const int InputNodes = 4;
const int HiddenNodes = 5;
const int OutputNodes = 2;
const float LearningRate = 0.3;
const float Momentum = 0.9;
const float InitialWeightMax = 0.5;
const float Success = 0.0015;

float Input[PatternCount][InputNodes] = {
  { 0, 1, 1, 0 },  // LIGHT ON LEFT AND RIGHT
  { 0, 1, 0, 0 },  // LIGHT ON LEFT
  { 1, 1, 1, 0 },  // LIGHT ON TOP, LEFT, and RIGHT
  { 1, 1, 0, 0 },  // LIGHT ON TOP and LEFT
  { 0, 0, 1, 0 },  // LIGHT ON RIGHT
  { 1, 0, 0, 0 },  // LIGHT ON TOP
  { 0, 0, 0, 0 },  // NO LIGHT
  { 0, 0, 0, 1 },  // LIGHT ON BOTTOM
  { 0, 1, 0, 1 },  // LIGHT ON BOTTOM AND LEFT
  { 0, 0, 1, 1 },  // LIGHT ON BOTTOM AND RIGHT
  { 0, 1, 1, 1 },  // LIGHT ON BOTTOM, LEFT, and RIGHT
  { 1, 0, 0, 1 },  // LIGHT ON TOP AND BOTTOM
  { 1, 1, 0, 1 },  // LIGHT ON TOP, BOTTOM, and LEFT
  { 1, 0, 1, 1 },  // LIGHT ON TOP, BOTTOM, and RIGHT
  { 1, 0, 1, 0 },  // LIGHT ON TOP AND RIGHT
  { 1, 1, 1, 1 },

};

const float Target[PatternCount][OutputNodes] = {
  { 0.75, 0.75 },
  { 0.75, 0.5 },     //BOTH MOTORS FULL FORWARD
  { 0.2, 0.2 },     //BOTH MOTORS FULL BACKWARDS
  { 1, 0.2 },   //MOTOR RIGHT FULL FORWARD, LEFT STOPPED
  { 0.5, 0.75 }, //MOTOR RIGHT BACKWARDS, LEFT STOPPED
  { 0.3, 0.3 }, //MOTOR LEFT BACKWARDS, RIGHT STOPPED
  { 0.5, 0.5 },   //BOTH MOTORS BACKWARDS
  { 0.75, 0.75 },
  { 1, 0.75 },
  { 0.75, 1 },
  { 1, 1 },
  { 1, 0 },
  { 1, 0.75 },
  { 0.75, 1 },
  { 0.2, 1 },
  { 0.65, 0.65},
};

/******************************************************************
   End Network Configuration
 ******************************************************************/

int i, j, p, q, r;
int ReportEvery1000;
int RandomizedIndex[PatternCount];
long  TrainingCycle;
float Rando;
float Error = 2;
float Accum;

float Hidden[HiddenNodes];
float Output[OutputNodes];

float HiddenWeights[InputNodes + 1][HiddenNodes];
float OutputWeights[HiddenNodes + 1][OutputNodes];
float HiddenDelta[HiddenNodes];
float OutputDelta[OutputNodes];
float ChangeHiddenWeights[InputNodes + 1][HiddenNodes];
float ChangeOutputWeights[HiddenNodes + 1][OutputNodes];

int ErrorGraph[64];

void setup() {
  // put your setup code here, to run once:
  for (int x = 0; x<64; x++){
    ErrorGraph[x] = 47;
  }
  
  pinMode(A1, INPUT);
  pinMode(A2, INPUT);
  pinMode(A3, INPUT);
  pinMode(A4, INPUT);
  pinMode(left_b, INPUT_PULLUP);
  pinMode(right_b, INPUT_PULLUP);

  pinMode(PHEN, OUTPUT);
  digitalWrite(PHEN, LOW);

  randomSeed(analogRead(A1));       //Collect a random ADC sample for Randomization.
  ReportEvery1000 = 1;
  for ( p = 0 ; p < PatternCount ; p++ ) {
    RandomizedIndex[p] = p ;
  }

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
  while (prog_start == 0) {
    u8g2.firstPage();
    do {
      menu_select();
    } while ( u8g2.nextPage() );
  }
  u8g2.clear();
  delay(1000);
  switch (curr_menu) {
    case 1:
      simpleLightAvoid();
      break;
    case 2:
      train_nn();
      break;
    case 3:
      drive_nn();
      break;
    default:
      break;

  }
}

void simpleLightAvoid() {
  while (1){
  PH1 = analogRead(A1);
  PH2 = analogRead(A2);
  PH3 = analogRead(A3);
  PH4 = analogRead(A4);
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
  if (Aspeed < 62 && Aspeed > 38) {
    Aspeed = 50;
  }
  if (Bspeed < 62 && Bspeed > 38) {
    Bspeed = 50;
  }
  motorA(Aspeed);
  motorB(Bspeed);

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
  u8g2.drawStr( 7, 25, "2)TrainNN");
  u8g2.drawStr( 7, 40, "3)RunNN");
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

void train_nn() {
  /******************************************************************
    Initialize HiddenWeights and ChangeHiddenWeights
  ******************************************************************/
  prog_start = 0;
  digitalWrite(LEDYEL, LOW);
  for ( i = 0 ; i < HiddenNodes ; i++ ) {
    for ( j = 0 ; j <= InputNodes ; j++ ) {
      ChangeHiddenWeights[j][i] = 0.0 ;
      Rando = float(random(100)) / 100;
      HiddenWeights[j][i] = 2.0 * ( Rando - 0.5 ) * InitialWeightMax ;
    }
  }
  digitalWrite(LEDYEL, HIGH);
  /******************************************************************
    Initialize OutputWeights and ChangeOutputWeights
  ******************************************************************/
  digitalWrite(LEDRED, LOW);
  for ( i = 0 ; i < OutputNodes ; i ++ ) {
    for ( j = 0 ; j <= HiddenNodes ; j++ ) {
      ChangeOutputWeights[j][i] = 0.0 ;
      Rando = float(random(100)) / 100;
      OutputWeights[j][i] = 2.0 * ( Rando - 0.5 ) * InitialWeightMax ;
    }
  }
  digitalWrite(LEDRED, HIGH);
  //SerialUSB.println("Initial/Untrained Outputs: ");
  //toTerminal();
  /******************************************************************
    Begin training
  ******************************************************************/

  for ( TrainingCycle = 1 ; TrainingCycle < 2147483647 ; TrainingCycle++) {

    /******************************************************************
      Randomize order of training patterns
    ******************************************************************/

    for ( p = 0 ; p < PatternCount ; p++) {
      q = random(PatternCount);
      r = RandomizedIndex[p] ;
      RandomizedIndex[p] = RandomizedIndex[q] ;
      RandomizedIndex[q] = r ;
    }
    Error = 0.0 ;
    /******************************************************************
      Cycle through each training pattern in the randomized order
    ******************************************************************/
    for ( q = 0 ; q < PatternCount ; q++ ) {
      p = RandomizedIndex[q];

      /******************************************************************
        Compute hidden layer activations
      ******************************************************************/
      digitalWrite(LEDYEL, LOW);
      for ( i = 0 ; i < HiddenNodes ; i++ ) {
        Accum = HiddenWeights[InputNodes][i] ;
        for ( j = 0 ; j < InputNodes ; j++ ) {
          Accum += Input[p][j] * HiddenWeights[j][i] ;
        }
        Hidden[i] = 1.0 / (1.0 + exp(-Accum)) ;
      }
      digitalWrite(LEDYEL, HIGH);

      /******************************************************************
        Compute output layer activations and calculate errors
      ******************************************************************/
      digitalWrite(LEDRED, LOW);
      for ( i = 0 ; i < OutputNodes ; i++ ) {
        Accum = OutputWeights[HiddenNodes][i] ;
        for ( j = 0 ; j < HiddenNodes ; j++ ) {
          Accum += Hidden[j] * OutputWeights[j][i] ;
        }
        Output[i] = 1.0 / (1.0 + exp(-Accum)) ;
        OutputDelta[i] = (Target[p][i] - Output[i]) * Output[i] * (1.0 - Output[i]) ;
        Error += 0.5 * (Target[p][i] - Output[i]) * (Target[p][i] - Output[i]) ;
      }
      //SerialUSB.println(Output[0]*100);
      digitalWrite(LEDRED, HIGH);
      /******************************************************************
        Backpropagate errors to hidden layer
      ******************************************************************/
      digitalWrite(LEDYEL, LOW);
      for ( i = 0 ; i < HiddenNodes ; i++ ) {
        Accum = 0.0 ;
        for ( j = 0 ; j < OutputNodes ; j++ ) {
          Accum += OutputWeights[i][j] * OutputDelta[j] ;
        }
        HiddenDelta[i] = Accum * Hidden[i] * (1.0 - Hidden[i]) ;
      }
      digitalWrite(LEDYEL, HIGH);

      /******************************************************************
        Update Inner-->Hidden Weights
      ******************************************************************/

      digitalWrite(LEDRED, LOW);
      for ( i = 0 ; i < HiddenNodes ; i++ ) {
        ChangeHiddenWeights[InputNodes][i] = LearningRate * HiddenDelta[i] + Momentum * ChangeHiddenWeights[InputNodes][i] ;
        HiddenWeights[InputNodes][i] += ChangeHiddenWeights[InputNodes][i] ;
        for ( j = 0 ; j < InputNodes ; j++ ) {
          ChangeHiddenWeights[j][i] = LearningRate * Input[p][j] * HiddenDelta[i] + Momentum * ChangeHiddenWeights[j][i];
          HiddenWeights[j][i] += ChangeHiddenWeights[j][i] ;
        }
      }
      digitalWrite(LEDRED, HIGH);
      /******************************************************************
        Update Hidden-->Output Weights
      ******************************************************************/
      digitalWrite(LEDYEL, LOW);
      for ( i = 0 ; i < OutputNodes ; i ++ ) {
        ChangeOutputWeights[HiddenNodes][i] = LearningRate * OutputDelta[i] + Momentum * ChangeOutputWeights[HiddenNodes][i] ;
        OutputWeights[HiddenNodes][i] += ChangeOutputWeights[HiddenNodes][i] ;
        for ( j = 0 ; j < HiddenNodes ; j++ ) {
          ChangeOutputWeights[j][i] = LearningRate * Hidden[j] * OutputDelta[i] + Momentum * ChangeOutputWeights[j][i] ;
          OutputWeights[j][i] += ChangeOutputWeights[j][i] ;
        }
      }
      digitalWrite(LEDYEL, HIGH);
    }

    /******************************************************************
      Every 1000 cycles send data to terminal for display
    ******************************************************************/
    ReportEvery1000 = ReportEvery1000 - 1;
    if (ReportEvery1000 == 0)
    {
      int graphNum = TrainingCycle/100;
      int graphE1 = Error * 1000;
      int graphE = map(graphE1, 3, 80, 47, 0); 
      ErrorGraph[graphNum] = graphE;
      u8g2.firstPage();
    do {
      drawGraph();
    } while ( u8g2.nextPage() );
  
      SerialUSB.println();
      SerialUSB.println();
      SerialUSB.print ("TrainingCycle: ");
      SerialUSB.print (TrainingCycle);
      SerialUSB.print ("  Error = ");
      SerialUSB.println (Error, 5);
      SerialUSB.print ("  Graph Num: ");
      SerialUSB.print (graphNum);
      SerialUSB.print ("  Graph Error1 = ");
      SerialUSB.print (graphE1);
      SerialUSB.print ("  Graph Error = ");
      SerialUSB.println (graphE);

      toTerminal();

      if (TrainingCycle == 1)
      {
        ReportEvery1000 = 99;
      }
      else
      {
        ReportEvery1000 = 100;
      }
    }


    /******************************************************************
      If error rate is less than pre-determined threshold then end
    ******************************************************************/

    if ( Error < Success ) break ;
  }
}


void drive_nn()
{
  SerialUSB.println("Running NN Drive Test");
  if (Success < Error){
      prog_start = 0;
      SerialUSB.println("NN not Trained");
  }
  while (Error < Success) {
    int num;
    int farDist = 35;
    int closeDist = 7;
    float TestInput[] = {0, 0, 0, 0};
    digitalWrite(LEDYEL, LOW);

    int LL1 = analogRead(A1);   // Collect sonar distances.
    int LL2 = analogRead(A2);   // Collect sonar distances.
    int LL3 = analogRead(A3);   // Collect sonar distances.
    int LL4 = analogRead(A4);   // Collect sonar distances.

#ifdef DEBUG
    SerialUSB.print("Light Level: ");
    SerialUSB.print(LL1);
    SerialUSB.print("\t");
    SerialUSB.print(LL2);
    SerialUSB.print("\t");
    SerialUSB.print(LL3);
    SerialUSB.print("\t");
    SerialUSB.println(LL4);
#endif

    digitalWrite(LEDYEL, HIGH);

    LL1 = map(LL1, 400, 1024, 0, 100);
    LL2 = map(LL2, 400, 1024, 0, 100);
    LL3 = map(LL3, 400, 1024, 0, 100);
    LL4 = map(LL4, 400, 1024, 0, 100);

    LL1 = constrain(LL1, 0, 100);
    LL2 = constrain(LL2, 0, 100);
    LL3 = constrain(LL3, 0, 100);
    LL4 = constrain(LL4, 0, 100);

    TestInput[0] = float(LL1) / 100;
    TestInput[1] = float(LL2) / 100;
    TestInput[2] = float(LL3) / 100;
    TestInput[3] = float(LL4) / 100;
#ifdef DEBUG
    SerialUSB.print("Input: ");
    SerialUSB.print(TestInput[3], 2);
    SerialUSB.print("\t");
    SerialUSB.print(TestInput[2], 2);
    SerialUSB.print("\t");
    SerialUSB.print(TestInput[1], 2);
    SerialUSB.print("\t");
    SerialUSB.println(TestInput[0], 2);
#endif

    InputToOutput(TestInput[0], TestInput[1], TestInput[2], TestInput[3]); //INPUT to ANN to obtain OUTPUT

    int speedA = Output[0] * 100;
    int speedB = Output[1] * 100;
    speedA = int(speedA);
    speedB = int(speedB);
#ifdef DEBUG
    SerialUSB.print("Speed: ");
    SerialUSB.print(speedA);
    SerialUSB.print("\t");
    SerialUSB.println(speedB);
#endif
    motorA(speedA);
    motorB(speedB);
    delay(50);
  }
}

void drawGraph(){
  for (int x = 2; x<64; x++){
  u8g2.drawLine(x-1, ErrorGraph[x-2], x-1, ErrorGraph[x-1]);
  } 
}


void toTerminal()
{

  for ( p = 0 ; p < PatternCount ; p++ ) {
    SerialUSB.println();
    SerialUSB.print ("  Training Pattern: ");
    SerialUSB.println (p);
    SerialUSB.print ("  Input ");
    for ( i = 0 ; i < InputNodes ; i++ ) {
      SerialUSB.print (Input[p][i], DEC);
      SerialUSB.print (" ");
    }
    SerialUSB.print ("  Target ");
    for ( i = 0 ; i < OutputNodes ; i++ ) {
      SerialUSB.print (Target[p][i], DEC);
      SerialUSB.print (" ");
    }
    /******************************************************************
      Compute hidden layer activations
    ******************************************************************/

    for ( i = 0 ; i < HiddenNodes ; i++ ) {
      Accum = HiddenWeights[InputNodes][i] ;
      for ( j = 0 ; j < InputNodes ; j++ ) {
        Accum += Input[p][j] * HiddenWeights[j][i] ;


      }
      Hidden[i] = 1.0 / (1.0 + exp(-Accum)) ;
    }

    /******************************************************************
      Compute output layer activations and calculate errors
    ******************************************************************/

    for ( i = 0 ; i < OutputNodes ; i++ ) {
      Accum = OutputWeights[HiddenNodes][i] ;
      for ( j = 0 ; j < HiddenNodes ; j++ ) {
        Accum += Hidden[j] * OutputWeights[j][i] ;
      }
      Output[i] = 1.0 / (1.0 + exp(-Accum)) ;
    }
    //SerialUSB.print ("  Output ");
    //for ( i = 0 ; i < OutputNodes ; i++ ) {
    //  SerialUSB.print (Output[i], 5);
    //  SerialUSB.print (" ");
    // }
  }
}

void InputToOutput(float In1, float In2, float In3, float In4)
{
  float TestInput[] = {0, 0, 0, 0};
  TestInput[0] = In1;
  TestInput[1] = In2;
  TestInput[2] = In3;
  TestInput[3] = In4;

  /******************************************************************
    Compute hidden layer activations
  ******************************************************************/

  for ( i = 0 ; i < HiddenNodes ; i++ ) {
    Accum = HiddenWeights[InputNodes][i] ;
    for ( j = 0 ; j < InputNodes ; j++ ) {
      Accum += TestInput[j] * HiddenWeights[j][i] ;
    }
    Hidden[i] = 1.0 / (1.0 + exp(-Accum)) ;
  }

  /******************************************************************
    Compute output layer activations and calculate errors
  ******************************************************************/

  for ( i = 0 ; i < OutputNodes ; i++ ) {
    Accum = OutputWeights[HiddenNodes][i] ;
    for ( j = 0 ; j < HiddenNodes ; j++ ) {
      Accum += Hidden[j] * OutputWeights[j][i] ;
    }
    Output[i] = 1.0 / (1.0 + exp(-Accum)) ;
  }
#ifdef DEBUG
  SerialUSB.print ("  Output ");
  for ( i = 0 ; i < OutputNodes ; i++ ) {
    SerialUSB.print (Output[i], 5);
    SerialUSB.print (" ");
  }
#endif
}


