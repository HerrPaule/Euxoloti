/*  Arduino - Axoloti communcation and I/O expanding for EUXOLOTI 1.2
 *  created by Paul 2016 - www.irieelectronics.de
 *  licence: BSD
 *
 *  SV: 1.4.1
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
 *
 *  changelog:
 *  - added Debug options
 *  - send CC by midiReset message
 */


#include <MIDI.h>

// Debug option
#define serialDebug Serial // select Serialport for debugging 
#define debug 0
#define adcDebug 0

struct euxoSettings : public midi::DefaultSettings
{
  static const bool UseRunningStatus = false; // Messes with my old equipment!
  static const bool HandleNullVelocityNoteOnAsNoteOff = true;
  static const bool Use1ByteParsing = true;
  static const long BaudRate = 115200; // should be equal to axo settings
  static const unsigned SysExMaxSize = 128; // not implemented in axo object
};

MIDI_CREATE_CUSTOM_INSTANCE(HardwareSerial, Serial3, euxo, euxoSettings);

#define midiCh_In 1     // MIDI IN channel
#define midiCh_Out 1    // MIDI OUT channel

// maple mini pin outs
const uint8_t button[] = {22, 21, 20, 19};
const uint8_t led[] = {25, 16, 26, 11};
const uint8_t pot[] = {10, 9, 8, 7, 4, 3, 5, 6};

// button state section
uint8_t buttonState[4];
uint8_t lastButtonState[4] = {HIGH, HIGH, HIGH, HIGH};  // default state

// for debouncing
long buttonLastDebounceTime[] = {0, 0, 0, 0}; // the last time the output pin was toggled
long buttonDebounceDelay = 50; // the debounce time; increase if the output flickers
uint8_t buttonReading[4];

// adc section
uint32_t adcValue[] = {0, 0, 0, 0, 0, 0, 0, 0};
long adcLastValue[] = {0, 0, 0, 0, 0, 0, 0, 0};
bool adcCheck = false;  // for callback routine. not used at the moment.
unsigned long oldMillis[] =  {0, 0, 0, 0, 0, 0, 0, 0};
long adcLastDebounceTime[] = {0, 0, 0, 0, 0, 0, 0, 0}; // the last time the output pin has been changed
long adcDebounceDelay = 100; // the debounce time; increase if the output flickers

// average section
uint8_t adcTreshhold = 5;
uint8_t maxCount = 5;       // max. depends on adcbuf[]
uint8_t adcBuf[10];         // adc read buffer
uint32_t adcTempValue;  // temporary storage
bool ioStatus = false;
bool midiRequest = 0;

void midiReset () {
  for (uint8_t b = 0; b < 4; b++) {
    analogWrite(led[b], 65535);   // scale midi max 7bit to 16bit pwm
    //delay(50);
  }
  for (uint8_t pR = 0; pR < 8; pR++) {
    digitalWrite(PB1, LOW);
    delay(10);


    adcLastValue[pR] = analogRead(pot[pR]);
    euxo.sendControlChange(pR + 1, (adcLastValue[pR] >> 5), midiCh_Out);
    digitalWrite(PB1, HIGH);
  }

  for (uint8_t c = 0; c < 4; c++) {
    analogWrite(led[c], 0);   // scale midi max 7bit to 16bit pwm
  }
  if (debug != 0) {
    serialDebug.println("midi reset");
  }
}

void ccIn (byte channel, byte number, byte value) {
  if (number < 4) {                         // ignore any Note which is higher then 4 (0-3 correspond to LED pins)
    analogWrite(led[number], value << 9);   // scale midi max 7bit to 16bit pwm
  }                                         // and use value to control each LED by PWM modulation.
  else if (number == 5 && channel == 1) {
    adcTreshhold = value;
  }

  if (debug != 0 && adcDebug != 0) {
    serialDebug.println("CC received");
  }
}

void handleNoteOn (byte channel, byte note, byte value) {   //not used yet
  if (debug != 0) {
    serialDebug.println("note in received");
  }
}

void setup() {
  pinMode(PB1, OUTPUT);
  //digitalWrite(PB1, HIGH);
  if (debug != 0) {
    serialDebug.begin(115200);
    delay(10);
    serialDebug.println("debug enabled");
  }

  // pinMode(PB1, OUTPUT);                // maple mini onboard LED

  for (uint8_t i = 0; i < 4; i++) {           // setup pins for LED, BUTTON and GATE inputs.
    pinMode(button[i], INPUT);
    pinMode(led[i], PWM);
  }
  for (uint8_t a = 0; a < 4; a++) {
    analogWrite(led[a], 0);   // scale midi max 7bit to 16bit pwm

  }

  euxo.setHandleControlChange(ccIn);      // midi handle for Control Change messages = leds
  euxo.setHandleNoteOn(handleNoteOn);     // midi handle for Note On. Not used but usefull for callback tasks.
  euxo.setHandleSystemReset(midiReset);   // not used at the moment

  euxo.begin(midiCh_In);                  // start MIDI
  euxo.turnThruOff();                     // turn MIDI through off
  digitalWrite(PB1, HIGH);

  for (uint8_t b = 0; b < 4; b++) {
    analogWrite(led[b], 65535);   // scale midi max 7bit to 16bit pwm
    delay(50);
  }
  for (uint8_t c = 0; c < 4; c++) {
    analogWrite(led[c], 0);   // scale midi max 7bit to 16bit pwm
    delay(50);
  }
  delay(10);
}


void loop() {
  euxo.read();                                        // check if there is any incoming midi message
  // BUTTON SECTION

  for (uint8_t x = 0; x < 4; x++) {                       // go through every pin
    buttonReading[x] = digitalRead(button[x]);        // read current pin state

    if (buttonReading[x] != lastButtonState[x]) {     // if the button state has changed
      buttonLastDebounceTime[x] = millis();           // take a timestamp
    }
    if ((millis() - buttonLastDebounceTime[x]) > buttonDebounceDelay) {   // check if the state has changed within a pre selected time
      if (buttonReading[x] != buttonState[x]) {
        buttonState[x] = buttonReading[x];
        if (buttonState[x] == HIGH) {
          euxo.sendNoteOff(x, 0, midiCh_Out);        // send Note On message
          //analogWrite(led[x], 0);   // scale midi max 7bit to 16bit pwm
        }                                             // x=Note Number
        else if (buttonState[x] == LOW) {
          euxo.sendNoteOn(x, 127, midiCh_Out);         // send Note Off message
          //analogWrite(led[x], 127 << 9);   // scale midi max 7bit to 16bit pwm
        }
      }
    }
    lastButtonState[x] = buttonReading[x];            // store button State for next turn
  }

  for (uint8_t p = 0; p < 8; p++) {

    if ((abs(millis() - adcLastDebounceTime[p]) > adcDebounceDelay)) {


      uint16_t potState = analogRead(pot[p]);

      adcLastDebounceTime[p] = millis();

      if (abs(potState - adcLastValue[p]) >= adcTreshhold) {
        adcLastValue[p] = potState;
        euxo.sendControlChange(p + 1, (potState >> 5), midiCh_Out);
      }
    }
  }
}
