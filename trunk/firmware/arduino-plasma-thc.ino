//we have to include the libary
#include <LedControl.h>
#include <EEPROM.h>

#define ANALOG_PIN 0
#define TORCH_OK_PIN 5
#define TORCH_UP_PIN 3
#define TORCH_DOWN_PIN 4
#define SET_OPERATION 0
#define SET_CUT_VOLT 1
#define SET_ACCUR_VOLT 2
#define MAX_CONFIG_NUM 20
#define UP_BUT

int ad_value = 0;
byte digits[3];
int actual_voltage;
byte cutting_voltage;
byte accuracy_voltage;
const int upButtonPin = 0;
const int downButtonPin = 1;
const int setButtonPin = 2;
int setButtonState = SET_OPERATION;
byte config_id;

// record definition for table
struct config{
   byte cutting_voltage;
   byte accuracy_voltage;
} config;

// create the LedControl object
LedControl lc=LedControl(8,10,9,1);

// fill EEPROM with initial values
void fill_eeprom() {
   byte i;
   config_id = 0;
   EEPROM.write(0, config_id);
   config.cutting_voltage = 100;
   config.accuracy_voltage = 5;
   
   for(i=0; i<=20; i++) {
      save_config(i, 12);
   }
}

// save config into EEPROM by address
void save_config(byte address, byte field) {
  byte new_address;
  switch(field) {
      case 1:
         new_address = address*2 + 1;
         EEPROM.write(new_address, config.cutting_voltage);
         break;
      case 2:
         new_address = address*2 + 2;
         EEPROM.write(new_address, config.accuracy_voltage);
         break;
      case 12:
         new_address = address*2 + 1;
         EEPROM.write(new_address, config.cutting_voltage);
         new_address = address*2 + 2;
         EEPROM.write(new_address, config.accuracy_voltage);
         break;
   }
}

// load configuration from EEPROM by address
void load_config(byte address) {
   byte cutting_voltage_address = address*2 + 1;
   byte accuracy_voltage_address = address*2 + 2;

   config.cutting_voltage = EEPROM.read(cutting_voltage_address);
   config.accuracy_voltage = EEPROM.read(accuracy_voltage_address);
}

// convert integer to 3 digit value
void toDigits(int num) {
   byte i=0;
   byte j;

   if(num > 999) return;

   while(num) {
      digits[i]=num%10;
      i++;
      num=num/10;
   }
   for(j=i;j<3;j++) digits[j]=0;
}

// print to LED display
void print(char disp, int num, char alpha) {
   // convert integer to 3 digit value
   toDigits(num);
   
   switch(disp) {
      case 1:
         // print digits to 1st display
         lc.setDigit(0, 0, digits[2], false);
         lc.setDigit(0, 1, digits[1], false);
         lc.setDigit(0, 2, digits[0], false);
         break;
      case 2:
         // print digits to 2nd display
         if(alpha == 0) {                                            // print 'F' or 'P' character to 1st digit if given
            lc.setDigit(0, 3, digits[2], false);
         }
         else {
            lc.setChar(0, 3, alpha, false);
         }
         lc.setDigit(0, 4, digits[1], false);

         if(config_id == 0 && (alpha == 'F' || alpha == 'P')) {             // print 'A' character to 3rd digit if given
            lc.setChar(0, 5, 'A', false);
         }else {
            lc.setDigit(0, 5, digits[0], false);
         }
         break;
      case 3:
         // print digits to 3rd display
         if(alpha == 0) {                                             // print 'A' character to 2nd digit if given
            lc.setDigit(0, 7, digits[0], false);
            lc.setDigit(0, 6, digits[1], false);
         }else if(alpha == '-') {
            lc.setChar(0, 7, alpha, false);
            lc.setChar(0, 6, alpha, false);
         }else {
            lc.setChar(0, 7, alpha, false);
            lc.setDigit(0, 6, digits[1], false);
         }
         break;
   }
}

void setup() {
   
   // wake up the MAX72XX from power-saving mode 
   lc.shutdown(0,false);
   // set the number of digits (or rows) to be displayed
   lc.setScanLimit(0, 7);
   // set a medium brightness for the Leds
   lc.setIntensity(0,15);
   // clear the display
   lc.clearDisplay(0);
   // initialize the pushbutton pin as an input
   pinMode(setButtonPin, INPUT);
   // initialize the torch_ok, torch_up and torch_down pins as output
   pinMode(TORCH_OK_PIN, OUTPUT);
   pinMode(TORCH_UP_PIN, OUTPUT);
   pinMode(TORCH_DOWN_PIN, OUTPUT);
   // fill EEPROM with initial configuration
//   fill_eeprom();
   // load last used configuration at startup
   config_id = EEPROM.read(0);
   load_config(config_id);
}


// loop forever
void loop() {

   // read the analog input pin
   ad_value = analogRead(ANALOG_PIN);
   // convert it to voltage value 0 - 255V
   actual_voltage = ad_value*0.248;
   
   // check if SET button is pressed 1st time -> set cutting voltage
   if(!digitalRead(setButtonPin) && setButtonState == SET_OPERATION) {
     // set new button state
     setButtonState = SET_CUT_VOLT;
     // wait for button release
     while(!digitalRead(setButtonPin)) {};
   // check if SET button is pressed 2nd time -> set accuracy
   }else if(!digitalRead(setButtonPin) && setButtonState == SET_CUT_VOLT) {
     // set new button state
     setButtonState = SET_ACCUR_VOLT;
     // wait for button release
     while(!digitalRead(setButtonPin)) {};
   // check if SET button is pressed 3rd time -> return to normal operation
   }else if(!digitalRead(setButtonPin) && setButtonState == SET_ACCUR_VOLT) {
     // set new button state
     setButtonState = SET_OPERATION;
     // wait for button release
     while(!digitalRead(setButtonPin)) {};
     // save configured voltages except config 'A'
     if(config_id != 0) {
        // save new cutting_voltage value for actual configuration id
        save_config(config_id, 1);
        // save new acuracy_voltage value for actual configuration id
        save_config(config_id, 2);
     }
   }

   // check SET button state 
   switch(setButtonState) {
     // cutting voltage setup
     case SET_CUT_VOLT:
       print(1, config.cutting_voltage, 0);
       print(2, config_id, 'F');
       print(3, 0, '-');
       // if UP button is pressed increment voltage value
       if(!digitalRead(upButtonPin) && config.cutting_voltage < 170) {
         config.cutting_voltage++;
       // wait for button release
       while(!digitalRead(upButtonPin)) {};
       }
       // if DOWN button is pressed decrement voltage value
       if(!digitalRead(downButtonPin) && config.cutting_voltage > 50) {
         config.cutting_voltage--;
       // wait for button release
       while(!digitalRead(downButtonPin)) {};
       }
       break;
     // accuracy voltage setup
     case SET_ACCUR_VOLT:
       print(1, config.accuracy_voltage, 0);
       print(2, config_id, 'P');
       print(3, 0, '-');
       // if UP button is pressed increment voltage value
       if(!digitalRead(upButtonPin) && config.accuracy_voltage < 10) {
         config.accuracy_voltage++;
       // wait for button release
       while(!digitalRead(upButtonPin)) {};
       }
       // if DOWN button is pressed decrement voltage value
       if(!digitalRead(downButtonPin) && config.accuracy_voltage > 0) {
         config.accuracy_voltage--;
       // wait for button release
       while(!digitalRead(downButtonPin)) {};
       }
       break;

     // in normal operation
     case SET_OPERATION:
       // print actual voltage on display 1
       print(1, actual_voltage, 0);
       // print cutting voltage on display 2
       print(2, config.cutting_voltage, 0);
       // print config id on display 3
       if(config_id == 0 ) {
          print(3, 0, 'A');
       }else {
          print(3, config_id, 0);
       }
       // if UP button is pressed increment config number
       if(!digitalRead(upButtonPin) && config_id < MAX_CONFIG_NUM) {
         // increment config id
         config_id++;
         // load configuration by config id
         load_config(config_id);
         // save loaded config id into EEPROM addres 0 
         EEPROM.write(0, config_id);
       // wait for button release
       while(!digitalRead(upButtonPin)) {};
       }
       // if DOWN button is pressed decrement config number
       if(!digitalRead(downButtonPin) && config_id > 0) {
         // decrement config id
         config_id--;
         // load configuration by config id
         load_config(config_id);
         // save loaded config id into EEPROM addres 0 
         EEPROM.write(0, config_id);
       // wait for button release
       while(!digitalRead(downButtonPin)) {};
       }
       break;
   }
}
