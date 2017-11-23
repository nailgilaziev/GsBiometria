#include "SoftwareSerial.h"
#include <Adafruit_Fingerprint.h>
#include <Bounce2.h>

Bounce debouncer = Bounce();

// pin #2 is RX IN from sensor (GREEN wire)
// pin #3 is TX OUT from arduino  (WHITE wire)
SoftwareSerial fingerSerial(D7, D8);
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&fingerSerial);

SoftwareSerial blu(D4, D2);

#define pTouch D6
#define pRed D1
#define pSwitch D5

byte intPin = 0;

//volatile bool touched = false;
bool touched = false;

// void touchedF(){
//   touched = digitalRead(pTouch) == LOW;
//   Serial.print(".");
// }

void setup() {


  pinMode(pTouch, INPUT);
  pinMode(pRed,OUTPUT);
  pinMode(pSwitch,OUTPUT);
  digitalWrite(pSwitch, HIGH);
  Serial.begin(9600);
  // while (!Serial) {
  //   ; // wait for serial port to connect. Needed for native USB port only
  // }
  Serial.println("started");
  // set the data rate for the sensor serial port
  finger.begin(57600);
  blu.begin(9600);

  if (finger.verifyPassword()) {
    Serial.println("Found fingerprint sensor!");
  } else {
    Serial.println("Did not find fingerprint sensor :(");
    while (1);
  }
  // attachInterrupt(intPin, touchedF, CHANGE);
  debouncer.attach(pTouch);
  debouncer.interval(2); // interval in ms
}

#define ENROLL 1
#define DELETE 2

void readInput(Stream *s){
  Serial.print("Input Cmd Event ");
  String line = s->readString();
  int cmd = (int)line[0] - '0';
  Serial.println(cmd);
  int id = line.substring(1).toInt();
  Serial.print("id = ");
  Serial.println(id);
  uint8_t res;
  if(cmd == ENROLL) res = getFingerprintEnroll(id);
  if(cmd == DELETE) res = deleteFingerprint(id);
  if (res!=FINGERPRINT_OK) {
    blu.write("fe\r\n");
  }
  Serial.print("Result ");
  Serial.println(res);
}

void loop() {
  if (blu.available()) {
    readInput(&blu);
    return;
  }
  if(Serial.available()){
    // detachInterrupt(intPin);
    readInput(&Serial);
    // attachInterrupt(intPin, touchedF, CHANGE);
    return;
  }
  debouncer.update();
  if (debouncer.fell()) {
    touched = true;
  }
  if (!touched) {
    return;
  }
  int tryCount = 0;
  Serial.println("New loop after touching");
  int scanResult = -1;
  while(scanResult == -1 && ++tryCount<4 && touched){
    long t = millis();
    scanResult = getFingerprintIDez();
    if(scanResult == -1){
      digitalWrite(pRed,HIGH);
      Serial.print("Attempt #");
      Serial.print(tryCount);
      Serial.println(" failed");
      while(millis()-t < 500 && touched);
      t=millis();
    }else{
      digitalWrite(pRed,LOW);
      digitalWrite(pSwitch, LOW);
      delay(1000);
      digitalWrite(pSwitch, HIGH);
    }
    debouncer.update();
    touched = debouncer.read() == LOW;
//    Serial.print("touched ");
//    Serial.println(touched);
  }
  digitalWrite(pRed,LOW);
  touched = false;
}


uint8_t deleteFingerprint(uint8_t id) {
  uint8_t p = -1;

  p = finger.deleteModel(id);

  if (p == FINGERPRINT_OK) {
    Serial.println("Deleted!");
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    return p;
  } else if (p == FINGERPRINT_BADLOCATION) {
    Serial.println("Could not delete in that location");
    return p;
  } else if (p == FINGERPRINT_FLASHERR) {
    Serial.println("Error writing to flash");
    return p;
  } else {
    Serial.print("Unknown error: 0x"); Serial.println(p, HEX);
    return p;
  }
}

uint8_t getFingerprintEnroll(uint8_t id) {
  uint8_t p = -1;
  Serial.println("Waiting for valid finger to enroll");
  blu.write("f1e\r\n");
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image taken");
      break;
    case FINGERPRINT_NOFINGER:
      Serial.println(".");
      break;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      break;
    case FINGERPRINT_IMAGEFAIL:
      Serial.println("Imaging error");
      break;
    default:
      Serial.println("Unknown error");
      break;
    }
  }

  // OK success!

  p = finger.image2Tz(1);
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image converted");
      break;
    case FINGERPRINT_IMAGEMESS:
      Serial.println("Image too messy");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return p;
    case FINGERPRINT_FEATUREFAIL:
      Serial.println("Could not find fingerprint features");
      return p;
    case FINGERPRINT_INVALIDIMAGE:
      Serial.println("Could not find fingerprint features");
      return p;
    default:
      Serial.println("Unknown error");
      return p;
  }

  Serial.println("Remove finger");
  blu.write("f1r\r\n");
  delay(2000);
  p = 0;
  while (p != FINGERPRINT_NOFINGER) {
    p = finger.getImage();
  }

  p = -1;
  Serial.println("Place same finger again");
  blu.write("f2e\r\n");
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image taken");
      break;
    case FINGERPRINT_NOFINGER:
      Serial.print(".");
      break;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      break;
    case FINGERPRINT_IMAGEFAIL:
      Serial.println("Imaging error");
      break;
    default:
      Serial.println("Unknown error");
      break;
    }
  }

  // OK success!

  p = finger.image2Tz(2);
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image converted");
      break;
    case FINGERPRINT_IMAGEMESS:
      Serial.println("Image too messy");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return p;
    case FINGERPRINT_FEATUREFAIL:
      Serial.println("Could not find fingerprint features");
      return p;
    case FINGERPRINT_INVALIDIMAGE:
      Serial.println("Could not find fingerprint features");
      return p;
    default:
      Serial.println("Unknown error");
      return p;
  }


  // OK converted!
  p = finger.createModel();
  if (p == FINGERPRINT_OK) {
    Serial.println("Prints matched!");
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    return p;
  } else if (p == FINGERPRINT_ENROLLMISMATCH) {
    Serial.println("Fingerprints did not match");
    return p;
  } else {
    Serial.print("Unknown error:");
    Serial.println(p,HEX);
    return p;
  }

  p = finger.storeModel(id);
  if (p == FINGERPRINT_OK) {
    Serial.println("Stored!");
    blu.write("f2r\r\n");
    return p;
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    return p;
  } else if (p == FINGERPRINT_BADLOCATION) {
    Serial.println("Could not store in that location");
    return p;
  } else if (p == FINGERPRINT_FLASHERR) {
    Serial.println("Error writing to flash");
    return p;
  } else {
    Serial.println("Unknown error");
    return p;
  }
}


uint8_t getFingerprintID() {
  uint8_t p = finger.getImage();
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image taken");
      break;
    case FINGERPRINT_NOFINGER:
      Serial.println("No finger detected");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return p;
    case FINGERPRINT_IMAGEFAIL:
      Serial.println("Imaging error");
      return p;
    default:
      Serial.println("Unknown error");
      return p;
  }

  // OK success!

  p = finger.image2Tz();
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image converted");
      break;
    case FINGERPRINT_IMAGEMESS:
      Serial.println("Image too messy");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return p;
    case FINGERPRINT_FEATUREFAIL:
      Serial.println("Could not find fingerprint features");
      return p;
    case FINGERPRINT_INVALIDIMAGE:
      Serial.println("Could not find fingerprint features");
      return p;
    default:
      Serial.println("Unknown error");
      return p;
  }

  // OK converted!
  p = finger.fingerFastSearch();
  if (p == FINGERPRINT_OK) {
    Serial.println("Found a print match!");
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    return p;
  } else if (p == FINGERPRINT_NOTFOUND) {
    Serial.println("Did not find a match");
    return p;
  } else {
    Serial.println("Unknown error");
    return p;
  }

  // found a match!
  Serial.print("Found ID #"); Serial.print(finger.fingerID);
  Serial.print(" with confidence of "); Serial.println(finger.confidence);
}

// returns -1 if failed, otherwise returns ID #
int getFingerprintIDez() {
  uint8_t p = finger.getImage();
  if (p != FINGERPRINT_OK)  return -1;

  p = finger.image2Tz();
  if (p != FINGERPRINT_OK)  return -1;

  p = finger.fingerFastSearch();
  if (p != FINGERPRINT_OK)  return -1;

  // found a match!
  Serial.print("Found ID #"); Serial.print(finger.fingerID);
  Serial.print(" with confidence of "); Serial.println(finger.confidence);
  return finger.fingerID;
}
