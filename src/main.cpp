/*
  RadioLib CC1101 Receive with Interrupts Example

  This example listens for FSK transmissions and tries to
  receive them. Once a packet is received, an interrupt is
  triggered.

  To successfully receive data, the following settings have to be the same
  on both transmitter and receiver:
  - carrier frequency
  - bit rate
  - frequency deviation
  - sync word

  For default module settings, see the wiki page
  https://github.com/jgromes/RadioLib/wiki/Default-configuration#cc1101

  For full API reference, see the GitHub Pages
  https://jgromes.github.io/RadioLib/
*/

#include <RadioLib.h>
#include <SPI.h>
#include "RotatoryEncoder.hpp"

#define SCK_PIN 47
#define MISO_PIN 45
#define MOSI_PIN 20

#define CS_PIN 10
#define GDO0_PIN 2
#define GDO2_PIN 3

// Rotary encoder pins
#define SWITCH_PIN 4

SPIClass spi(HSPI);
SPISettings spiSettings(2000000, MSBFIRST, SPI_MODE0);

// CC1101 has the following connections:
// CS pin:    CS_PIN
// GDO0 pin:  GDO0_PIN
// RST pin:   unused
// GDO2 pin:  GDO2_PIN (optional)
CC1101 radio = new Module(CS_PIN, GDO0_PIN, RADIOLIB_NC, GDO2_PIN, spi, spiSettings);

RotatoryEncoder rotatoryEncoder(SWITCH_PIN);

// or detect the pinout automatically using RadioBoards
// https://github.com/radiolib-org/RadioBoards
/*
#define RADIO_BOARD_AUTO
#include <RadioBoards.h>
Radio radio = new RadioModule();
*/

enum Mode {
  RECEIVE,
  TRANSMIT
};

int transmissionState = RADIOLIB_ERR_NONE;

int countReceivedPackets = 0;

Mode currentMode = RECEIVE;
Mode previousMode = RECEIVE;
bool modeChanged = false;

// Flag to indicate that a packet was received and sent
volatile bool receivedFlag = false;
volatile bool transmittedFlag = false;

// This function is called when a complete packet is received by the module
// IMPORTANT: this function MUST be 'void' type and MUST NOT have any arguments!
#if defined(ESP8266) || defined(ESP32)
  ICACHE_RAM_ATTR
#endif
void setReceiveFlag(void) {
  receivedFlag = true; // We got a packet, set the flag
}

void setSentFlag(void) {
  transmittedFlag = true; // We sent a packet, set the flag
}

void setup() {
  Serial.begin(115200);
  rotatoryEncoder.begin();
  
  spi.begin(SCK_PIN, MISO_PIN, MOSI_PIN, -1);

  // Initialize CC1101 with default settings
  Serial.print(F("[CC1101] Initializing ... "));
  int state = radio.begin();
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while (true) { delay(10); }
  }

  // Set callback for packet reception and transmission
  radio.setPacketReceivedAction(setReceiveFlag);
  radio.setPacketSentAction(setSentFlag);

  // Start listening for packets
  Serial.print(F("[CC1101] Starting to listen ... "));
  state = radio.startReceive();
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while (true) { delay(10); }
  }
}

void handleReceivedPacket() {
  if (modeChanged) {
    // Start listening for packets
    Serial.print(F("[CC1101] Starting to listen ... "));
    int state = radio.startReceive();
    if (state == RADIOLIB_ERR_NONE) {
      Serial.println(F("Success!"));
    } else {
      Serial.print(F("Failed, code "));
      Serial.println(state);
      while (true) { delay(10); }
    }
  }

  if(receivedFlag) {
    receivedFlag = false;

    // You can read received data as an Arduino String
    String str;
    int state = radio.readData(str);

    // You can also read received data as byte array
    /*
      byte byteArr[8];
      int numBytes = radio.getPacketLength();
      int state = radio.readData(byteArr, numBytes);
    */

    if (state == RADIOLIB_ERR_NONE) {
      // Packet was successfully received
      Serial.println(F("[CC1101] Received packet!"));

      // Print data of the packet
      Serial.print(F("[CC1101] Data:\t\t"));
      Serial.println(str);

      // Print RSSI (Received Signal Strength Indicator) of the last received packet
      Serial.print(F("[CC1101] RSSI:\t\t"));
      Serial.print(radio.getRSSI());
      Serial.println(F(" dBm"));

      // Print LQI (Link Quality Indicator) of the last received packet, lower is better
      Serial.print(F("[CC1101] LQI:\t\t"));
      Serial.println(radio.getLQI());

    } else if (state == RADIOLIB_ERR_CRC_MISMATCH) {
      // Packet was received, but is malformed
      Serial.println(F("CRC error!"));

    } else {
      // Some other error occurred
      Serial.print(F("Failed, code "));
      Serial.println(state);

    }

    // Put module back to listen mode
    radio.startReceive();
  }
}

void handleSentPacket() {
  transmissionState = RADIOLIB_ERR_NONE;

  if (modeChanged) {
    // Start transmitting the first packet
    Serial.print(F("[CC1101] Sending first packet ... "));

    // You can transmit C-string or Arduino string up to 255 characters long
    transmissionState = radio.startTransmit("Hello World!");

    // You can also transmit byte array up to 255 bytes long
    // When transmitting more than 64 bytes startTransmit blocks to refill the FIFO.
    // Blocking ceases once the last bytes have been placed in the FIFO
    /*
      byte byteArr[] = {0x01, 0x23, 0x45, 0x56,
                        0x78, 0xAB, 0xCD, 0xEF};
      state = radio.startTransmit(byteArr, 8);
    */
  }

  // Check if the previous transmission finished
  if(transmittedFlag) {
    transmittedFlag = false;

    if (transmissionState == RADIOLIB_ERR_NONE) {
      // Packet was successfully sent
      Serial.println(F("Transmission finished!"));

      // NOTE: When using interrupt-driven transmit method,
      //       it is not possible to automatically measure
      //       transmission data rate using getDataRate()

    } else {
      Serial.print(F("Failed, code "));
      Serial.println(transmissionState);
    }

    // Clean up after transmission is finished
    // This will ensure transmitter is disabled,
    // RF switch is powered down etc.
    radio.finishTransmit();

    delay(1000);

    Serial.print(F("[CC1101] Sending another packet ... "));

    // You can transmit C-string or Arduino string up to 255 characters long
    String str = "Hello World! #" + String(countReceivedPackets++);
    transmissionState = radio.startTransmit(str);

    // You can also transmit byte array up to 255 bytes long with limitations https://github.com/jgromes/RadioLib/discussions/1138
    /*
      byte byteArr[] = {0x01, 0x23, 0x45, 0x67,
                        0x89, 0xAB, 0xCD, 0xEF};
      int state = radio.startTransmit(byteArr, 8);
    */
  }
}

void handleRotatoryEncoder() {
  rotatoryEncoder.update();

  // Check for button press to toggle mode
  if (rotatoryEncoder.wasPressed()) {
    currentMode = currentMode == Mode::RECEIVE ? Mode::TRANSMIT : Mode::RECEIVE;
    modeChanged = true;
    Serial.print(F("Switched to "));
    Serial.println(currentMode == Mode::RECEIVE ? F("RECEIVE mode") : F("TRANSMIT mode"));
  } else {
    modeChanged = false;
  }

  auto fn = currentMode == Mode::RECEIVE ? handleReceivedPacket : handleSentPacket;
  fn();
}

void loop() {
  handleRotatoryEncoder();
}