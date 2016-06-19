#include <Manchester.h>
#include <libHomeMesh.h>
#include <IRremote.h>

#define TX_PIN 2  //pin where your RF transmitter is connected
#define RX_PIN 0 //pin where your RF receiver is located
#define IR_TX_PIN 11 //pin where your IR LED is connected (via transistor)
#define IR_SIGNAL_TIMEOUT 5000 //maximum length, in microseconds, of a gap in an IR signal. Used to trigger IR sending
#define MAX_CODE_LENGTH 140
#define IR_FREQ 38 // IR Frequency in kilohertz
void setup() {
  man.setupReceive(RX_PIN, MAN_1200);
  man.beginReceive();
  Serial.begin(9600);
  pinMode(13, OUTPUT);
  pinMode(9,OUTPUT);
}

IRsend irsend;

uint8_t rfBuffer[MAX_CODE_LENGTH*2+2];
uint8_t recordingIndex = 0;
unsigned long lastEventTimestamp = 0;
uint8_t data;
uint8_t id;
unsigned int irBuffer[MAX_CODE_LENGTH];
void loop() {
  if(man.receiveComplete()) {
    rfBuffer[0] = 0xFF;
    //check if this is a command to begin
    //Serial.println("Received a message");
    uint16_t m = man.getMessage();
    if (man.decodeMessage(m, id, data)) {
      if(id == IR_REPEATER_DEVICE_ID && data == IR_REPEATER_BEGIN_TRANSMISSION) {
        //Serial.println("Transmission begins");
        //get the number of bytes in the stream
        man.beginReceive();
        //mark this time so we know if we timeout
        unsigned long rxStart;
        rxStart = millis();
        bool receiveFailed = false;
        while(!man.receiveComplete()) {
          if(millis() - rxStart > 1000) {
            receiveFailed = true;
            break;
          }
        }
        //check if the receive succeeded
        uint8_t payloadSize = 0;
        if(receiveFailed == false) {
          //get the number of bytes coming over the stream
          m = man.getMessage();
          if(man.decodeMessage(m, id, data) && id == IR_REPEATER_DEVICE_ID) {
            payloadSize = data;
            //Serial.print("Received this many bytes: ");
            //Serial.println(payloadSize,DEC);
          }
        }
        if(payloadSize > 0) {
          //start receiving array
          man.beginReceiveArray(payloadSize, rfBuffer);
          while(true) {
            if (man.receiveComplete()) {
              digitalWrite(13, HIGH);
              //print the data to serial for now
              for(uint8_t i=1;i<payloadSize;i+=2) {
                //reassemble the data into shorts
                irBuffer[i/2] = ((rfBuffer[i] & 0xFF) << 8) | (rfBuffer[i+1] & 0xFF);
                //Serial.println(irBuffer[i/2],DEC);
              }
              irsend.sendRaw(irBuffer, (payloadSize-2)/2,IR_FREQ);
              //Serial.println("Transmission ends");
              digitalWrite(13, LOW);
              break;
            } else if ((millis() - rxStart) > 3000) {
              Serial.println("Timeout");
              break;
            } else {
              //no timeout yet
              continue;
            }
          }
        }
      }
}
    man.beginReceive();
  }
}
