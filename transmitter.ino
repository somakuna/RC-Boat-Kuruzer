#include <Rotary.h>
#include <SPI.h>
#include <RF24.h>
#include <RF24_config.h>
#include <nRF24L01.h>
#include <printf.h>


#define MOTOR A1           // X os
#define RUDDER A0          // Y os
#define CONNECTION_LED 10  //prebirkavanje na analogni pin 3
#define R_ENCODER_S1 5
#define R_ENCODER_S2 6
#define R_ENCODER_SW 9
#define L_ENCODER_S1 2
#define L_ENCODER_S2 3
#define L_ENCODER_SW 4

RF24 radio(7, 8);  // CE, CSN

const byte address[6] = "00002";

// Max size of this struct is 32 bytes - NRF24L01 buffer limit
struct Data_Package {
  byte motorValue;
  byte rudderValue;
  byte rightCounter = 0;  // The variable to store value of the button
  byte leftCounter = 0;   // The variable to store value of the button
};

Data_Package data;  // Create a variable with the above structure
long ackData = 0;   // to hold the two values coming from the slave
// Change these pin numbers to the pins connected to your encoder.
//   Best Performance: both pins have interrupt capability
//   Good Performance: only the first pin has interrupt capability
//   Low Performance:  neither pin has interrupt capability

Rotary rotaryL = Rotary(L_ENCODER_S1, L_ENCODER_S2);
Rotary rotaryR = Rotary(R_ENCODER_S1, R_ENCODER_S2);
//   avoid using pins with LEDs attached

unsigned long currentMillis;
unsigned long prevMillis;
unsigned long lastRecvTime;
unsigned long txIntervalMillis = 20;

//left servo
bool leftREPushed = false;   // button push status    // actual signal pattern
bool rightREPushed = false;  // button push status

void setup() {
  Serial.begin(9600);
  radio.begin();
  //radio.setDataRate(RF24_250KBPS);
  radio.setPALevel(RF24_PA_HIGH);
  radio.enableAckPayload();
  //radio.enableDynamicPayloads();
  radio.setRetries(5, 5);
  radio.openWritingPipe(address);
  radio.stopListening();

  pinMode(MOTOR, INPUT);
  pinMode(RUDDER, INPUT);
  pinMode(L_ENCODER_S1, INPUT);
  pinMode(L_ENCODER_S2, INPUT);
  pinMode(L_ENCODER_SW, INPUT);
  pinMode(R_ENCODER_S1, INPUT);
  pinMode(R_ENCODER_S2, INPUT);
  pinMode(R_ENCODER_SW, INPUT);
  pinMode(CONNECTION_LED, OUTPUT);
  digitalWrite(CONNECTION_LED, LOW);
}
void loop() {

  currentMillis = millis();
  unsigned char leftRotaryMove = rotaryL.process();
  unsigned char rightRotaryMove = rotaryR.process();
  // left rotary encoder (ER)
  if (leftRotaryMove == DIR_CW) {
    if (data.leftCounter > 0) {
      data.leftCounter -= 10;
    } else {
      sendLEDWarning();
    }
  } else if (leftRotaryMove == DIR_CCW) {
    if (data.leftCounter < 180) {
      data.leftCounter += 10;
    } else {
      sendLEDWarning();
    }
  }
  //right rotary encoder (ER)
  if (rightRotaryMove == DIR_CCW) {
    if (data.rightCounter > 0) {
      data.rightCounter -= 10;
    } else {
      sendLEDWarning();
    }
  }
  if (rightRotaryMove == DIR_CW) {
    if (data.rightCounter < 180) {
      data.rightCounter += 10;
    } else {
      sendLEDWarning();
    }
  }
  bool leftREKey = !digitalRead(L_ENCODER_SW);
  bool rightREKey = !digitalRead(R_ENCODER_SW);
  if (leftREKey && leftREPushed == false) {
    leftREPushed = true;
    data.leftCounter = 0;
    sendLEDWarning();
  } else if (!leftREKey && leftREPushed == true) {
    leftREPushed = false;
  }
  if (rightREKey && rightREPushed == false) {
    rightREPushed = true;
    data.rightCounter = 0;
    sendLEDWarning();
  } else if (!rightREKey && rightREPushed == true) {
    rightREPushed = false;
  }
  data.motorValue = mapMotorValues(analogRead(MOTOR), 0, 512, 1023, false);
  data.rudderValue = mapRudderValues(analogRead(RUDDER), 0, 512, 1023, false);
  if (currentMillis - prevMillis >= txIntervalMillis) {
    // Serial.print("L: ");
    // Serial.print(data.leftCounter);
    // Serial.print(" D: ");
    // Serial.print(data.rightCounter);
    // Serial.print(" Motor: ");
    // Serial.print(data.motorValue);
    // Serial.print(" RUDDER: ");
    //Serial.println(data.rudderValue);
    send();
    prevMillis = millis();
  }
  if (prevMillis - lastRecvTime > 1000) {
    digitalWrite(CONNECTION_LED, HIGH);  // Signal lost
  }
}

void send() {
  //int con = constrain(analogRead(MOTOR), 512, 1023);
  //data.motorValue = map(analogRead(MOTOR), 0, 1023, 0, 255);
  bool rslt;
  rslt = radio.write(&data, sizeof(Data_Package));

  if (rslt || radio.isAckPayloadAvailable()) {
    radio.read(&ackData, sizeof(ackData));
    digitalWrite(CONNECTION_LED, LOW);
    lastRecvTime = millis();
  }
}

//Servo - rudder
int mapRudderValues(int val, int lower, int middle, int upper, bool reverse) {
  val = constrain(val, lower, upper);
  if (val < middle)
    val = map(val, lower, middle, 0, 90);  // 40, 90
  else
    val = map(val, middle, upper, 90, 180);  // 90, 140
  return (reverse ? 255 - val : val);
}

// Joystick - throttle
int mapMotorValues(int val, int lower, int middle, int upper, bool reverse) {
  val = constrain(val, lower, upper);
  if (val < middle)
    val = map(val, lower, middle, 0, 127);
  else
    val = map(val, middle, upper, 127, 255);
  return (reverse ? 255 - val : val);
}

void sendLEDWarning() {
  digitalWrite(CONNECTION_LED, HIGH);
  digitalWrite(CONNECTION_LED, LOW);
}