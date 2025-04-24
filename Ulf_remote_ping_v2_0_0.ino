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
String lastReceivedMessage = "";

const int prgButtonPin = 0;
bool sendPingRequested = false;
bool lora_idle = true;
unsigned long lastSendTime = 0;
unsigned long pingDisplayTimeout = 0;

static RadioEvents_t RadioEvents;

void VextON() {
  pinMode(Vext, OUTPUT);
  digitalWrite(Vext, LOW);
}

void displayLastMessage() {
  myDisplay.clear();
  myDisplay.drawString(0, 0, "Letztes Paket:");
  myDisplay.drawString(0, 12, lastReceivedMessage);
  myDisplay.display();
}

void displayPingSent() {
  myDisplay.clear();
  myDisplay.drawString(0, 0, "PING gesendet...");
  myDisplay.display();
}

void OnTxDone() {
  Serial.println("TX abgeschlossen. Zurück in RX...");
  Radio.Rx(0);  // Direkt wieder in den RX-Modus
  sendPingRequested = false;  // Senden abgeschlossen
  pingDisplayTimeout = millis() + 1000; // 1 Sekunde "Ping gesendet" anzeigen
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("Ulf Iteration 13 startet...");
  
  pinMode(prgButtonPin, INPUT_PULLUP);
  VextON();
  delay(100);

  myDisplay.init();
  myDisplay.setFont(ArialMT_Plain_10);
  myDisplay.setTextAlignment(TEXT_ALIGN_LEFT);
  
  Mcu.begin(HELTEC_BOARD, SLOW_CLK_TPYE);

  RadioEvents.TxDone = OnTxDone;
  RadioEvents.RxDone = OnRxDone;
  Radio.Init(&RadioEvents);
  Radio.SetChannel(RF_FREQUENCY);
  Radio.SetRxConfig(MODEM_LORA, LORA_BANDWIDTH, LORA_SPREADING_FACTOR,
                    LORA_CODINGRATE, 0, LORA_PREAMBLE_LENGTH,
                    LORA_SYMBOL_TIMEOUT, LORA_FIX_LENGTH_PAYLOAD_ON,
                    0, true, 0, 0, LORA_IQ_INVERSION_ON, true);

  myDisplay.drawString(0, 0, "Ulf_remote_ping_v2_0_0");
  myDisplay.drawString(0, 12, "is starting!");
  myDisplay.drawString(0, 36, "Press PRG for PING!");
  myDisplay.display();
  delay(1000);
  Radio.Rx(0);
}

void loop() {
  Radio.IrqProcess();

  if (digitalRead(prgButtonPin) == LOW && lora_idle && !sendPingRequested) {
    Serial.println("PRG gedrückt – Beende RX und sende PING...");
    Radio.Sleep();  // RX beenden
    Radio.Send((uint8_t *)"PING", 4);
    displayPingSent();
    lora_idle = false;
    sendPingRequested = true;
    lastSendTime = millis();
  }

  if (sendPingRequested && millis() > pingDisplayTimeout) {
    displayLastMessage();
  }
}

void OnRxDone(uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr) {
  memcpy(rxBuffer, payload, size);
  rxBuffer[size] = '\0';
  lastReceivedMessage = String(rxBuffer);
  Serial.printf("Empfangen: %s\n", rxBuffer);
  Serial.printf("RSSI: %d dBm, SNR: %d dB\n", rssi, snr);

  displayLastMessage();
  lora_idle = true;
}
