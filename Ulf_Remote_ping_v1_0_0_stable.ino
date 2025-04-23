#include "LoRaWan_APP.h"
#include "HT_SSD1306Wire.h"
#include "Arduino.h"

#define RF_FREQUENCY        433775000
#define LORA_BANDWIDTH      0
#define LORA_SPREADING_FACTOR 9
#define LORA_CODINGRATE     1
#define LORA_PREAMBLE_LENGTH 8
#define LORA_SYMBOL_TIMEOUT 0
#define LORA_FIX_LENGTH_PAYLOAD_ON false
#define LORA_IQ_INVERSION_ON false

#define OLED_ADDR 0x3c
SSD1306Wire myDisplay(OLED_ADDR, 400000, SDA_OLED, SCL_OLED, GEOMETRY_128_64, RST_OLED);

#define BUFFER_SIZE 64
char rxBuffer[BUFFER_SIZE];
const int prgButtonPin = 0;
bool sendPingRequested = false;
bool lora_idle = true;
unsigned long lastSendTime = 0;

static RadioEvents_t RadioEvents;

void VextON() {
  pinMode(Vext, OUTPUT);
  digitalWrite(Vext, LOW);
}

void displayListening() {
  myDisplay.clear();
  myDisplay.drawString(0, 0, "Ich lausche auf LoRa...");
  myDisplay.display();
}

void displayPingSent() {
  myDisplay.clear();
  myDisplay.drawString(0, 0, "Ping gesendet");
  myDisplay.display();
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("Ulf Iteration 6 startet...");

  pinMode(prgButtonPin, INPUT_PULLUP);
  VextON();
  delay(100);

  myDisplay.init();
  myDisplay.setFont(ArialMT_Plain_10);
  myDisplay.setTextAlignment(TEXT_ALIGN_LEFT);
  //myDisplay.flipScreenVertically();
  displayListening();

  Mcu.begin(HELTEC_BOARD, SLOW_CLK_TPYE);

  RadioEvents.RxDone = OnRxDone;
  Radio.Init(&RadioEvents);
  Radio.SetChannel(RF_FREQUENCY);
  Radio.SetRxConfig(MODEM_LORA, LORA_BANDWIDTH, LORA_SPREADING_FACTOR,
                    LORA_CODINGRATE, 0, LORA_PREAMBLE_LENGTH,
                    LORA_SYMBOL_TIMEOUT, LORA_FIX_LENGTH_PAYLOAD_ON,
                    0, true, 0, 0, LORA_IQ_INVERSION_ON, true);

  Radio.Rx(0);  // Start im RX-Modus
}

void loop() {
  // Tastendruck erkennen
  if (digitalRead(prgButtonPin) == LOW && !sendPingRequested) {
    sendPingRequested = true;
    lastSendTime = millis();
    Radio.Sleep();  // RX beenden
    Serial.println("PRG gedrückt – Sende Ping...");
    Radio.Send((uint8_t *)"PING", 4);
    displayPingSent();
  }

  // Nach dem Senden zurück in RX
  if (sendPingRequested && millis() - lastSendTime >= 800) {
    displayListening();
    Radio.Rx(0);  // RX-Modus wieder aktivieren
    sendPingRequested = false;
  }

  Radio.IrqProcess();
}

// Callback bei Empfang eines Pakets
void OnRxDone(uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr) {
  memcpy(rxBuffer, payload, size);
  rxBuffer[size] = '\0';

  Serial.printf("Empfangen: %s\n", rxBuffer);
  Serial.printf("RSSI: %d dBm, SNR: %d dB\n", rssi, snr);

  myDisplay.clear();
  myDisplay.drawString(0, 0, "Empfangen:");
  myDisplay.drawString(0, 12, String(rxBuffer));
  myDisplay.drawString(0, 54, String("RSSI: ") + String(rssi) + " dBm");
  myDisplay.display();
}
