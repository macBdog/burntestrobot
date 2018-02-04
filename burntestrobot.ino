#include <LiquidCrystal.h>

// Which pins are connected to the inputs. First four are digital outputs, the rest analog inputs
#define PIN_CARRIAGE_DIR    2
#define PIN_CARRIAGE_STEP   3
#define PIN_SAMPLE_MOTOR1   4
#define PIN_SAMPLE_MOTOR2   5
#define PIN_CARRIAGE_UP     A1
#define PIN_CARRIAGE_DOWN   A2
#define PIN_SAMPLE_OUT      A3
#define PIN_SAMPLE_IN       A4
#define PIN_HYSTERSIS       100

// State keeps track of what the machine is doing
#define STATE_STARTUP       0 // Fresh boot or reset
#define STATE_RESET         1 // Reset motors until both the stops are hit
#define STATE_WAIT          2 // Wait for keypress
#define STATE_SAMPLE_IN     3 // Sample insert
#define STATE_CARRIAGE_DOWN 4 // Carriage descent
#define STATE_TESTTIME      5 // Timing
#define STATE_CARRIAGE_UP   6 // Carriage ascent
#define STATE_SAMPLE_OUT    7 // Sample return

byte state = 0;
int timer = 0;
int testTime = 1000;

int sampleMoveSpeed = 200;
byte sampleDirIn = false;

int carriageMoveSpeed = 500;
byte carriageDirUp = false;

LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

// ADC readings expected for the 5 buttons on the ADC input
#define RIGHT_10BIT_ADC           0  // right
#define UP_10BIT_ADC            145  // up
#define DOWN_10BIT_ADC          329  // down
#define LEFT_10BIT_ADC          505  // left
#define SELECT_10BIT_ADC        741  // right
#define BUTTONHYSTERESIS         145  // hysteresis for valid button sensing window

#define BUTTON_ADC_PIN           A0  // A0 is the button ADC input
#define BUTTON_NONE               0  
#define BUTTON_TIME_INC           1  // Add 100ms onto the test time
#define BUTTON_TIME_DEC           2  // Remove 100ms from the test time
#define BUTTON_UNUSED2            3  // 
#define BUTTON_BEGIN              4  // Start the test where possible
#define BUTTON_UNUSED3            5  // 

byte buttonJustPressed  = false;         //this will be true after a ReadButtons() call if triggered
byte buttonJustReleased = false;         //this will be true after a ReadButtons() call if triggered
byte buttonWas          = BUTTON_NONE;   //used by ReadButtons() for detection of button events

void setup() 
{
  pinMode(PIN_CARRIAGE_DIR, OUTPUT);
  pinMode(PIN_CARRIAGE_STEP, OUTPUT);
  pinMode(PIN_SAMPLE_MOTOR1, OUTPUT);
  pinMode(PIN_SAMPLE_MOTOR2, OUTPUT);
  digitalWrite(PIN_CARRIAGE_DIR, LOW);
  digitalWrite(PIN_CARRIAGE_STEP, LOW);
  digitalWrite(PIN_SAMPLE_MOTOR1, LOW);
  digitalWrite(PIN_SAMPLE_MOTOR2, LOW);
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
  HandleState();
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
   
  switch (button)
  {
    case BUTTON_TIME_INC:
    {
      if (state <= STATE_WAIT)
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
      }
      break;
    }
    case BUTTON_TIME_DEC:
    {
      if (state <= STATE_WAIT)
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
      }
      break;
    }
    case BUTTON_BEGIN:
    {
      // If we are waiting for keypress then start testing
      if (buttonJustPressed)
      {
        if (state == STATE_WAIT)
        {
          timer = testTime;
          state = STATE_SAMPLE_IN;
          digitalWrite(PIN_SAMPLE_IN, HIGH);
          PrintState();
        }
      }
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

void SampleMove(boolean moveDirectionIn)
{
  if (moveDirectionIn)
  {
    digitalWrite(PIN_SAMPLE_MOTOR1, HIGH); 
    digitalWrite(PIN_SAMPLE_MOTOR2, LOW); 
  }
  else
  {
    digitalWrite(PIN_SAMPLE_MOTOR1, LOW); 
    digitalWrite(PIN_SAMPLE_MOTOR2, HIGH); 
  }
}

void CarriageMove(boolean moveDirectionDown)
{ 
  if (moveDirectionDown)
  { 
    digitalWrite(PIN_CARRIAGE_DIR, HIGH); 
  }
  else
  {
    digitalWrite(PIN_CARRIAGE_DIR, LOW);
  }

  digitalWrite(PIN_CARRIAGE_STEP, HIGH);
  delayMicroseconds(carriageMoveSpeed);

  digitalWrite(PIN_CARRIAGE_STEP, LOW);
  delayMicroseconds(carriageMoveSpeed);
}

void HandleState()
{
  switch (state)
  {
    case STATE_STARTUP:
    {
      // Turn off all motors
      timer = 0;
      digitalWrite(PIN_SAMPLE_MOTOR1, LOW); 
      digitalWrite(PIN_SAMPLE_MOTOR2, LOW); 
      digitalWrite(PIN_CARRIAGE_DIR, LOW);
      digitalWrite(PIN_CARRIAGE_STEP, LOW);

      state = STATE_RESET;
      PrintState();
      break;
    }
    case STATE_RESET:     
    {
      // Read end stops for both motors
      unsigned int sampleVoltage = analogRead(PIN_SAMPLE_OUT);
      unsigned int carriageVoltage = analogRead(PIN_CARRIAGE_UP);

      // Move sample out until stop hit
      if (sampleVoltage <= PIN_HYSTERSIS)
      {
        SampleMove(false);
      }

      // Move carriage up until stop hit
      if (carriageVoltage <= PIN_HYSTERSIS)
      {
        CarriageMove(false);
      }

      // Wait till both motors are safe before proceeding
      if (sampleVoltage > PIN_HYSTERSIS && carriageVoltage > PIN_HYSTERSIS)
      {
        digitalWrite(PIN_SAMPLE_MOTOR1, LOW); 
        digitalWrite(PIN_SAMPLE_MOTOR2, LOW); 

        state = STATE_WAIT;
        PrintState();
      }
      break;
    }
    case STATE_WAIT:
    {
      break;
    }
    case STATE_SAMPLE_IN:
    {
      // Move sample in, read end stop and proceed
      SampleMove(true);
      unsigned int sampleVoltage = analogRead(PIN_SAMPLE_IN);
      if (sampleVoltage > PIN_HYSTERSIS)
      {
        digitalWrite(PIN_SAMPLE_MOTOR1, LOW); 
        digitalWrite(PIN_SAMPLE_MOTOR2, LOW); 

        state = STATE_CARRIAGE_DOWN;
        PrintState();
        digitalWrite(PIN_SAMPLE_IN, LOW);
      }
      break;
    }
    case STATE_CARRIAGE_DOWN:
    {
      // Move carriage down, read end stop and proceed
      CarriageMove(true);
      unsigned int carriageVoltage = analogRead(PIN_CARRIAGE_DOWN);
      if (carriageVoltage > PIN_HYSTERSIS)
      {
        timer = testTime;
        state = STATE_TESTTIME;
        PrintState();
      }
      break;
    }
    case STATE_TESTTIME:
    {
      // Wait till test is complete
      if (timer > 0)
      {
        timer = timer - 10;
        return;
      }
      else
      {
        state = STATE_CARRIAGE_UP;
        PrintState();
        break;
      }
      break;
    } 
    case STATE_CARRIAGE_UP:
    {
      // Move carriage down, read end stop and proceed
      CarriageMove(false);
      unsigned int carriageVoltage = analogRead(PIN_CARRIAGE_UP);
      if (carriageVoltage > PIN_HYSTERSIS)
      {
        state = STATE_SAMPLE_OUT;
        PrintState();
      }
      break;
    }
    case STATE_SAMPLE_OUT:
    {
      // Move sample out, read end stop and proceed to reset
      SampleMove(false);
      unsigned int sampleVoltage = analogRead(PIN_SAMPLE_OUT);
      if (sampleVoltage > PIN_HYSTERSIS)
      {
        digitalWrite(PIN_SAMPLE_MOTOR1, LOW); 
        digitalWrite(PIN_SAMPLE_MOTOR2, LOW); 
        state = STATE_RESET;
        PrintState();
        break;
      }
    }
    default: break;
  }
}

void PrintState()
{
  lcd.setCursor(0, 0);
  switch (state)
  {
    case STATE_STARTUP      : lcd.print("Please wait...  "); break;
    case STATE_RESET        : lcd.print("Please wait...  "); break;
    case STATE_WAIT         : lcd.print("Ready           "); break;
    case STATE_SAMPLE_IN    : lcd.print("Sample IN       "); break;
    case STATE_CARRIAGE_DOWN: lcd.print("Carriage DOWN   "); break;
    case STATE_TESTTIME     : lcd.print("Testing         "); break;
    case STATE_CARRIAGE_UP  : lcd.print("Carriage UP     "); break;
    case STATE_SAMPLE_OUT   : lcd.print("Sample OUT      "); break;
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
      button = BUTTON_TIME_INC;
   }
   else if(   buttonVoltage >= ( UP_10BIT_ADC - BUTTONHYSTERESIS )
           && buttonVoltage <= ( UP_10BIT_ADC + BUTTONHYSTERESIS ) )
   {
      button = BUTTON_TIME_DEC;
   }
   else if(   buttonVoltage >= ( DOWN_10BIT_ADC - BUTTONHYSTERESIS )
           && buttonVoltage <= ( DOWN_10BIT_ADC + BUTTONHYSTERESIS ) )
   {
      button = BUTTON_UNUSED2;
   }
   else if(   buttonVoltage >= ( LEFT_10BIT_ADC - BUTTONHYSTERESIS )
           && buttonVoltage <= ( LEFT_10BIT_ADC + BUTTONHYSTERESIS ) )
   {
      button = BUTTON_BEGIN;
   }
   else if(   buttonVoltage >= ( SELECT_10BIT_ADC - BUTTONHYSTERESIS )
           && buttonVoltage <= ( SELECT_10BIT_ADC + BUTTONHYSTERESIS ) )
   {
      button = BUTTON_UNUSED3;
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
