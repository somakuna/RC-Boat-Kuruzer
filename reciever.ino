#include <RF24.h>
#include <RF24_config.h>
#include <nRF24L01.h>
#include <printf.h>
#include <SPI.h>
#include <Servo.h>

#define RUDDER 5
#define MOTOR 3
#define LEFT_SERVO 2
#define RIGHT_SERVO 9

RF24 radio(7, 8);  // CE, CSN

const byte address[6] = "00002";
long ackData = 0;

Servo servoRudder;
Servo servoMotor;
Servo servoLeft;
Servo servoRight;

struct Data_Package {
  byte motorValue;
  byte rudderValue;
  byte rightCounter;  // The variable to store value of the button
  byte leftCounter;   // The variable to store value of the button
};
Data_Package data;

bool newData = false;
unsigned long previousMillis = 0;
unsigned long lastRecvTime = 0;
const long interval = 20;  // Interval in milliseconds

void setup() {
  //Serial.begin(9600);
  radio.begin();
  //radio.setDataRate(RF24_250KBPS);
  radio.setPALevel(RF24_PA_HIGH);
  radio.openReadingPipe(1, address);
  radio.enableAckPayload();
  radio.writeAckPayload(1, &ackData, sizeof(ackData));  // pre-load data
  radio.startListening();

  pinMode(MOTOR, OUTPUT);
  pinMode(RUDDER, OUTPUT);
  pinMode(LEFT_SERVO, OUTPUT);
  pinMode(RIGHT_SERVO, OUTPUT);

  servoRudder.attach(RUDDER);
  servoMotor.attach(MOTOR);
  servoLeft.attach(LEFT_SERVO);
  servoRight.attach(RIGHT_SERVO);

  servoMotor.write(1500);
  servoRudder.write(90);
  servoLeft.write(0);
  servoRight.write(0);
  resetData();
}
void loop() {
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    getData();
    //showData();
    // if ((currentMillis - lastRecvTime) >= 3000) {
    //   resetData();  // Signal lost.. Reset data | Sinyal kayıpsa data resetleniyor
    // }
    updateReplyData();
    //showData();
    //newData = true;
    servoMotor.write(map(data.motorValue, 0, 255, 1000, 2000));
    servoRudder.write(data.rudderValue);
    servoRight.write(data.rightCounter);
    servoLeft.write(data.leftCounter);
  }
}

void resetData() {
  data.motorValue = 127;  // Motor Stop | Motor Kapalı
  data.rudderValue = 90;  // Center | Merkez
  data.leftCounter = 0;   // Center | Merkez
  data.rightCounter = 0;  // Center | Merkez
}

void getData() {
  while (radio.available()) {
    radio.read(&data, sizeof(Data_Package));
    lastRecvTime = millis();  // receive the data | data alınıyor
  }
}

void updateReplyData() {
  ackData++;
  radio.writeAckPayload(1, &ackData, sizeof(ackData));  // load the payload for the next time
}

// void showData() {
//   Serial.print("x: ");
//   Serial.print(data.rudderValue);
//   Serial.print(" y: ");
//   Serial.print(data.motorValue);
//   Serial.print(" l: ");
//   Serial.print(data.leftCounter);
//   Serial.print(" r: ");
//   Serial.println(data.rightCounter);
// }
