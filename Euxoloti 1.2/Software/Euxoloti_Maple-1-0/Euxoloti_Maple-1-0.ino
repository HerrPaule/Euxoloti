/*  Arduino - Axoloti communcation and I/O expanding for EUXOLOTI 1.2
 *  created by Paul 2016 - www.irieelectronics.de
 *  licence: BSD
 *  
 *  Hardware Setup UART:
 *  Maple Mini (UART)
 *  RX3 = D0 (PB11)
 *  TX3 = D1 (PB10)
 *
 *  Axoloti (UART)
 *  RX = PA3
 *  TX = PA2
 *  
 *  TX and RX need 10K pullup resitor (3,3V) to stabilize serial port while axoloti resets or load new patches.
 */


#include <MIDI.h>
MIDI_CREATE_INSTANCE(HardwareSerial, Serial3, euxo);  // select Serial 3 for MIDI I/O

#define midiCh_In 1     // MIDI IN channel
#define midiCh_Out 2    // MIDI OUT channel

// maple mini pin outs
const int button[] = {22, 21, 20, 19};
const int led[] = {25, 16, 26, 11};
const int pot[] = {10, 9, 8, 7, 4, 3, 5, 6};

// button state section
int buttonState[4];
int lastButtonState[4] = {HIGH, HIGH, HIGH, HIGH};  // default state

// for debouncing
long buttonLastDebounceTime[] = {0, 0, 0, 0}; // the last time the output pin was toggled
long buttonDebounceDelay = 50; // the debounce time; increase if the output flickers
int buttonReading[4];

// adc section
uint32_t adcValue[] = {0, 0, 0, 0, 0, 0, 0, 0};
long adcLastValue[] = {0, 0, 0, 0, 0, 0, 0, 0};
bool adcCheck = false;  // for callback routine. not used at the moment.

// average section
int adcTreshhold = 2;
int maxCount = 5;       // max. depends on adcbuf[]
int adcBuf[10];         // adc read buffer
uint32_t adcTempValue;  // temporary storage
bool ioStatus = false;

void midiReset () {
  // not used at the moment
}

void ccIn (byte channel, byte number, byte value) {
  if (number < 4) {                         // ignore any Note which is bigger then 4 (0-3 correspond to LED pins)
    analogWrite(led[number], value << 9);   // scale midi max 7bit to 16bit pwm
  }                                         // and use value to control each LED by PWM modulation.
  else if (number == 5 && channel == 1) {
    adcTreshhold = value;
  }
}

void handleNoteOn (byte channel, byte note, byte value) {   //not used yet
  if (note == 0x7F && value == 0x7F) {
    for (int p = 0; p < 8; p++) {
      int potState = analogRead(pot[p]);
      if (abs(potState - adcLastValue[p]) >= adcTreshhold) {
        euxo.sendControlChange(p, (potState >> 5), midiCh_Out);
        adcLastValue[p] = potState;
      }
    }
    //delay(10);
  }
  else {
    //adcCheck = false;
  }
}

void setup() {
  //Serial.begin(115200);
  euxo.begin(midiCh_In);                  // start MIDI
  euxo.turnThruOff();                     // turn MIDI through off
  // pinMode(PB1, OUTPUT);                // maple mini onboard LED

  for (int i = 0; i < 4; i++) {           // setup pins for LED, BUTTON and GATE inputs.
    pinMode(button[i], INPUT);
    pinMode(led[i], OUTPUT);
  }

  //adcBuf[maxCount];
  for (int c = 0; c < maxCount; c++) {    // adc buffer action
    adcBuf[c] = 0;                        // reset adc buffer to 0
  }

  euxo.setHandleControlChange(ccIn);      // midi handle for Control Change messages = leds
  euxo.setHandleNoteOn(handleNoteOn);     // midi handle for Note On. Not used but usefull for callback tasks.
  euxo.setHandleSystemReset(midiReset);   // not used at the moment
}

void loop() {
  euxo.read();                                        // check if there is any incoming midi message
  // BUTTON SECTION
  for (int x = 0; x < 4; x++) {                       // go through every pin
    buttonReading[x] = digitalRead(button[x]);        // read current pin state

    if (buttonReading[x] != lastButtonState[x]) {     // if the button state has changed
      buttonLastDebounceTime[x] = millis();           // take a timestamp
    }
    if ((millis() - buttonLastDebounceTime[x]) > buttonDebounceDelay) {   // check if the state has changed within a pre selected time
      if (buttonReading[x] != buttonState[x]) {
        buttonState[x] = buttonReading[x];
        if (buttonState[x] == HIGH) {
          euxo.sendNoteOn(x, 0, midiCh_Out);        // send Note On message
        }                                             // x=Note Number
        else if (buttonState[x] == LOW) {
          euxo.sendNoteOff(x, 127, midiCh_Out);         // send Note Off message
        }
      }
    }
    lastButtonState[x] = buttonReading[x];            // store button State for next turn
  }
}
