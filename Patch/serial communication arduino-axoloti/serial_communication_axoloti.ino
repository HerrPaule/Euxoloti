/*  uart communication between arduino and axoloti core (v.1.0)
 *  by Paul 2016
 *  Serial Port connection:
 *  arduino RX -> axoloti core TX(PA2)
 *  arduino TX -> axoloti core RX(PA3)
 *  
 *  this example needs two serial ports at your arduino to work. 
 *  Please edit "Serial3." to "Serial." to run with only one serial port.
 */


uint8_t rxbuf[4];                 // for "l/" messages
String newTextmessage = "";       // string for incoming message
boolean messageComplete = false;  // state of completed string
bool message = false;             // state to dis-/enable string
int count = 0;                    // number of received strings.
int countMax = 20;                // max. number of counts

void setup() {
  Serial.begin(115200);           // initialize serial port
  Serial3.begin(115200);          // initialize 3.serial port. check your max. serial ports at your arduino. 
  newTextmessage.reserve(200);    // max. string size
  delay(5);
}

void loop() {
  
 if (Serial3.available()) {
    if (message == false) {                                         // check if no string is actually received.
      uint8_t messageDeclaration = Serial3.read();                  // read first byte of Serial3 buffer.
      uint8_t messageStart = Serial3.read();                        // read secound byte of Serial3 buffer.
      if (messageDeclaration == 'l' && messageStart == '/') {       // "l/" to receive four int to control e.g. pwm of leds
        ledMessage();                                            
      }
      else if (messageDeclaration == 'x' && messageStart == '/') {  // "x/" to flag an incoming string 
        message = true;
      }
    }
    else if(message == true) {            // string routine
      textMessage();                      // go to string analysing part
      if (messageComplete) {              // string is completed
        Serial.println(newTextmessage);   // send received string to Serial (arduino ide serial monitor)
        newTextmessage = "";              // reset string
        messageComplete = false;          // restart string receiving
        message = false;                  // disable string receiving
      }
    }
  }
}


void ledMessage() {             // after "l/", three values are send. whole message schould look like this: "l/value1 value2 value3 value4"
  rxbuf[0] = Serial3.read();    // 1.value (max.one byte 0-255)
  rxbuf[1] = Serial3.read();    // 2.value (max.one byte 0-255)
  rxbuf[2] = Serial3.read();    // 3.value (max.one byte 0-255)
  rxbuf[3] = Serial3.read();    // 4.value (max.one byte 0-255)
                                // print out the received values to Serial (arduino ide serial monitor)
  Serial.print(rxbuf[0]);
  Serial.print(" , ");
  Serial.print(rxbuf[1]);
  Serial.print(" , ");
  Serial.print(rxbuf[2]);
  Serial.print(" , ");
  Serial.println(rxbuf[3]);
}

void textMessage() {
  if (Serial3.available()>0) {
    char singleLetter = Serial3.read();     // receive characters by Serial3
    
    if (singleLetter == ';') {              // ";" at the end of a message flags end of message.
      if (count > countMax) {               // check count number
        count = 0;                          // reset count
        Serial3.write(count);             // send counts of string complete action to axoloti (example for callback)
      }
      count++;                              // count upwards same as  "count = count + 1;"
      messageComplete = true;               // stop character receiving loop
    }
    else {
      newTextmessage += singleLetter;       // add new character to already existing string elements
    }
  }
}

