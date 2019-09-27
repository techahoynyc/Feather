#include <LoraStack.h>
#include <ParameterStore.h>
#include <RamStore.h>
#include <Timer.h>
#include <Arduino.h>
#include <CayenneLPP.h>

const int sensePin = A3;

// Lorawan Device ID, App ID, and App Key
const char *devEui = "00D0DA8B23A5D2A4";
const char *appEui = "70B3D57ED00227BC";
const char *appKey = "7A885E8DF14CAFCD997D9E1B98ABF85D";

#define MY_FREQUENCY_PLAN TTN_FP_US915 // One of TTN_FP_EU868, TTN_FP_US915, TTN_FP_AS920_923, TTN_FP_AS923_925, TTN_FP_KR920_923

// Using RamStore for quick start. In a real device you
// would have EEPROM or FRAM or some other non-volatile
// store supported by ParameterStore. Or serialize RamStore
// to a file on an SD card.
const int STORE_SIZE = 2000;
RamStore<STORE_SIZE> byteStore;
ParameterStore gParameters(byteStore);

// Create lower level hardware interface with access to parameters
const Arduino_LoRaWAN::lmic_pinmap define_lmic_pins = {
  // Feather LoRa wiring with IO1 wired to D6
  .nss = 8, // Chip select
  .rxtx = LMIC_UNUSED_PIN,
  .rst = 4, // Reset
  .dio = {3, 6, LMIC_UNUSED_PIN}, // SX1276 pins DIO0, DIO1, DIO2
};
LoraStack_LoRaWAN lorawan(define_lmic_pins, gParameters);
// Create high level interface to TTN
LoraStack ttn(lorawan, gParameters, MY_FREQUENCY_PLAN);

Timer gTimer;

void setup() {
  Serial.begin(9600);
  // Wait for 15 seconds. If no Serial by then, keep going. We are not connected.
  for (int timeout=0; timeout<15 && !Serial; ++timeout) {
    delay(1000);
  }

  #define LORA_CS 8
  // Log.Debug("Writing default value to NSS: %d", LORA_CS);
  digitalWrite(LORA_CS, HIGH); // Default, unselected
  pinMode(LORA_CS, OUTPUT);
  pinMode(sensePin, INPUT);

  bool status = byteStore.begin();
  if (!status) {
      Serial.println(F("Could not find a valid FRAM module, check wiring!" CR));
      while (1);
  }

  Serial.println(F("Initializing parameter store!" CR));
  status = gParameters.begin();
  if (!status) {
      Serial.println(F("Failed to initialize Parameter Store!" CR));
      while (1);
  }

  status = ttn.join(appEui, devEui, appKey);
  if (!status) {
      Serial.println(F("Failed to provision device!" CR));
      while (1);
  }

  lorawan.begin();

  // Setup timer to kick off send every 10 seconds
  gTimer.every(10 * 1000, [](){
    // put your main code here, to run repeatedly:
    int sensorInput = analogRead(sensePin);    //read the analog sensor and store it
    //double temp = sensorInput * 0.48828125;
    double temp = (double)sensorInput / 1024;       //find percentage of input reading
    temp = temp * 3.3;                 //multiply by 3.3V to get voltage
    temp = temp - 0.5;               //Subtract the offset
    temp = temp * 100;


    //instance of the payload class
    CayenneLPP payload (10);
    payload.reset();
    payload.addTemperature(0, temp);

    // Prepare payload of 1 byte to indicate LED status
    //byte payload[1];
    //payload[0] = (digitalRead(LED_BUILTIN) == HIGH) ? 1 : 0;
    //char* name = "Frank";
    //Serial.println(payload[0]);
    // Send it off
    //ttn.sendBytes(temp, sizeof(temp));

    ttn.sendBytes(payload.getBuffer(), payload.getSize());
    Serial.print("The temperature is: ");
    Serial.println(temp);
  });
}

void loop() {
  ttn.loop(); // Call often to do LoRaWAN work

  gTimer.update();

  delay(10); // Not too long!
}
