#include <LiquidCrystal.h>

const int ledPin = 13;
const int dirPin = 2;
const int stepPin = 3;

// State keeps track of what the machine is doing
// 0 Startup
// 1 Reset motors until both the stops are hit
// 2 Wait for keypress
// 3 Sample insert
// 4 Carriage descent
// 5 Timing
// 6 Carriage ascent
// 7 Sample return

byte state = 0;
int timer = 0;
int testTime = 1000;
int sampleMoveSpeed = 200;
int carriageMoveSpeed = 200;

int dir = false;
int dirCount = 0;
int dirLimit = 1000;
bool touching = false;
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

// ADC readings expected for the 5 buttons on the ADC input
#define RIGHT_10BIT_ADC           0  // right
#define UP_10BIT_ADC            145  // up
#define DOWN_10BIT_ADC          329  // down
#define LEFT_10BIT_ADC          505  // left
#define SELECT_10BIT_ADC        741  // right
#define BUTTONHYSTERESIS         145  // hysteresis for valid button sensing window

//return values for ReadButtons()
#define BUTTON_ADC_PIN           A0  // A0 is the button ADC input
#define BUTTON_NONE               0  // 
#define BUTTON_RIGHT              1  // 
#define BUTTON_UP                 2  // 
#define BUTTON_DOWN               3  // 
#define BUTTON_LEFT               4  // 
#define BUTTON_SELECT             5  // 

byte buttonJustPressed  = false;         //this will be true after a ReadButtons() call if triggered
byte buttonJustReleased = false;         //this will be true after a ReadButtons() call if triggered
byte buttonWas          = BUTTON_NONE;   //used by ReadButtons() for detection of button events

void setup() 
{
  pinMode(dirPin, OUTPUT);
  pinMode(stepPin, OUTPUT);
  digitalWrite(dirPin, LOW);
  digitalWrite(stepPin, LOW);
  lcd.begin(16, 2);
  PrintState();
  lcd.setCursor(0, 1);
  lcd.print("TestTime: 1000ms");

  pinMode( BUTTON_ADC_PIN, INPUT );         //ensure A0 is an input
  digitalWrite( BUTTON_ADC_PIN, LOW );      //ensure pullup is off on A0
}

void loop() 
{
  ButtonLoop();
  SampleLoop();
  CarriageLoop();
}

void ButtonLoop()
{
  byte button;
  byte timestamp;
   
  //get the latest button pressed, also the buttonJustPressed, buttonJustReleased flags
  button = ReadButtons();

  if( buttonJustPressed || buttonJustReleased )
  {
    lcd.setCursor(0, 1);
    lcd.print( "TestTime: " );
    
    // Print a leading space for justification
    if (testTime < 1000)
    {
      lcd.setCursor(10, 1);
      lcd.print(" ");
      lcd.setCursor(11, 1);
    }
    else
    {
      lcd.setCursor(10, 1);
    }
    lcd.print(testTime, DEC); 
    lcd.setCursor(14, 1);
    lcd.print("ms");
  }
   
  //show text label for the button pressed
  switch( button )
  {
    case BUTTON_NONE:
    {
       break;
    }
    case BUTTON_RIGHT:
    {
      if (buttonJustPressed)
      {
        testTime = testTime + 100;
        if (testTime < 100)
        {
          testTime = 100;
        }
        if (testTime > 9000)
        {
          testTime = 9000;
        }
      }
      break;
    }
    case BUTTON_UP:
    {
      if (buttonJustPressed)
      {
        testTime = testTime - 100;
        if (testTime < 100)
        {
          testTime = 100;
        }
        if (testTime > 9000)
        {
          testTime = 9000;
        }
      }
      break;
    }
    case BUTTON_DOWN:
    {
      break;
    }
    case BUTTON_LEFT:
    {
      // If we are waiting for keypress then move to testing
      if (buttonJustPressed)
      {
        if (state == 2)
        {
          state = 3;
          PrintState();
        }
      }
      break;
    }
    case BUTTON_SELECT:
    {
      break;
    }
    default:
    {
      break;
    }
  }
   
  //clear the buttonJustPressed or buttonJustReleased flags, they've already done their job now.
  if (buttonJustPressed)
  {
    buttonJustPressed = false;
  }
  if (buttonJustReleased)
  {
    buttonJustReleased = false;
  }
}

void SampleLoop()
{

}

void CarriageLoop()
{ 
  int inSpeed = 500;
  
  if (dirCount >= dirLimit)
  {
    dirCount = 0;
    dir = !dir;
  }
  
  if (dir)
  { 
    digitalWrite(dirPin, HIGH); 
  }
  else
  {
    digitalWrite(dirPin, LOW);
  }

  digitalWrite(stepPin, HIGH);
  delayMicroseconds(inSpeed);

  digitalWrite(stepPin, LOW);
  delayMicroseconds(inSpeed);
    
  dirCount++;
}

void PrintState()
{
  lcd.setCursor(0, 0);
  switch (state)
  {
    case 0: lcd.print("Please wait... "); break;
    case 1: lcd.print("Please wait... "); break;
    case 2: lcd.print("Ready          "); break;
    case 3: lcd.print("Sample IN      "); break;
    case 4: lcd.print("Carriage DOWN  "); break;
    case 5: lcd.print("Test           "); break;
    case 6: lcd.print("Carriage UP    "); break;
    case 7: lcd.print("Sample OUT      "); break;
    default: break;
  }
}

byte ReadButtons()
{
   unsigned int buttonVoltage;
   byte button = BUTTON_NONE;   // return no button pressed if the below checks don't write to btn
   
   //read the button ADC pin voltage
   buttonVoltage = analogRead( BUTTON_ADC_PIN );
   
   //sense if the voltage falls within valid voltage windows
   if( buttonVoltage < ( RIGHT_10BIT_ADC + BUTTONHYSTERESIS ) )
   {
      button = BUTTON_RIGHT;
   }
   else if(   buttonVoltage >= ( UP_10BIT_ADC - BUTTONHYSTERESIS )
           && buttonVoltage <= ( UP_10BIT_ADC + BUTTONHYSTERESIS ) )
   {
      button = BUTTON_UP;
   }
   else if(   buttonVoltage >= ( DOWN_10BIT_ADC - BUTTONHYSTERESIS )
           && buttonVoltage <= ( DOWN_10BIT_ADC + BUTTONHYSTERESIS ) )
   {
      button = BUTTON_DOWN;
   }
   else if(   buttonVoltage >= ( LEFT_10BIT_ADC - BUTTONHYSTERESIS )
           && buttonVoltage <= ( LEFT_10BIT_ADC + BUTTONHYSTERESIS ) )
   {
      button = BUTTON_LEFT;
   }
   else if(   buttonVoltage >= ( SELECT_10BIT_ADC - BUTTONHYSTERESIS )
           && buttonVoltage <= ( SELECT_10BIT_ADC + BUTTONHYSTERESIS ) )
   {
      button = BUTTON_SELECT;
   }
   //handle button flags for just pressed and just released events
   if( ( buttonWas == BUTTON_NONE ) && ( button != BUTTON_NONE ) )
   {
      //the button was just pressed, set buttonJustPressed, this can optionally be used to trigger a once-off action for a button press event
      //it's the duty of the receiver to clear these flags if it wants to detect a new button change event
      buttonJustPressed  = true;
      buttonJustReleased = false;
   }
   if( ( buttonWas != BUTTON_NONE ) && ( button == BUTTON_NONE ) )
   {
      buttonJustPressed  = false;
      buttonJustReleased = true;
   }
   
   //save the latest button value, for change event detection next time round
   buttonWas = button;
   
   return( button );
}
