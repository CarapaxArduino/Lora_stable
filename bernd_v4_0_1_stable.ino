#include "LoRaWan_APP.h"
#include "HT_SSD1306Wire.h"
#include "Arduino.h"

#define RF_FREQUENCY        433775000
#define TX_OUTPUT_POWER     15
#define LORA_BANDWIDTH      0
#define LORA_SPREADING_FACTOR 9
#define LORA_CODINGRATE     1
#define LORA_PREAMBLE_LENGTH 8
#define LORA_SYMBOL_TIMEOUT 0
#define LORA_FIX_LENGTH_PAYLOAD_ON false
#define LORA_IQ_INVERSION_ON false

const int prgButtonPin = 0;

#define OLED_ADDR 0x3c
SSD1306Wire myDisplay(OLED_ADDR, 400000, SDA_OLED, SCL_OLED, GEOMETRY_128_64, RST_OLED);

#define BUFFER_SIZE 64
char txpacket[BUFFER_SIZE];
double txNumber = 0;
bool lora_idle = true;

static RadioEvents_t RadioEvents;

const char* channelNames[] = {"Front", "Heck", "Innen", "Pumpe"};
int channelStatus[] = {1, 0, 1, 0};

bool longPressHandled = false;
unsigned long buttonPressStart = 0;
unsigned long lastDisplayUpdate = 0;
const unsigned long displayInterval = 2000;

const char* version = "Bernd_v4_0_1_stable";

void OnTxDone() {
  Serial.println("TX abgeschlossen.");
  lora_idle = true;
}

void OnTxTimeout() {
  Serial.println("TX Timeout!");
  lora_idle = true;
}

void VextON() {
  pinMode(Vext, OUTPUT);
  digitalWrite(Vext, LOW);
}

void updateDisplayStatus() {
  myDisplay.clear();
  myDisplay.drawString(0, 0, "Status aller Kanaele:");
  for (int i = 0; i < 4; i++) {
    myDisplay.drawString(0, 12 + i * 10, String(channelNames[i]) + ": " + (channelStatus[i] ? "an" : "aus"));
  }
  myDisplay.display();
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("Starte Bernd...");

  pinMode(prgButtonPin, INPUT_PULLUP);
  VextON();
  delay(100);

  myDisplay.init();
  myDisplay.flipScreenVertically();
  myDisplay.setFont(ArialMT_Plain_10);
  myDisplay.setTextAlignment(TEXT_ALIGN_LEFT);

  myDisplay.clear();
  myDisplay.drawString(0, 0, version);
  myDisplay.drawString(0, 12, "Kurzer Tastendruck:");
  myDisplay.drawString(0, 24, "-> Status senden");
  myDisplay.drawString(0, 36, "Langer Druck:");
  myDisplay.drawString(0, 48, "-> Zufallstatus erzeugen");
  myDisplay.display();
  delay(5000);

  updateDisplayStatus();

  Mcu.begin(HELTEC_BOARD, SLOW_CLK_TPYE);
  RadioEvents.TxDone = OnTxDone;
  RadioEvents.TxTimeout = OnTxTimeout;
  Radio.Init(&RadioEvents);
  Radio.SetChannel(RF_FREQUENCY);
  Radio.SetTxConfig(MODEM_LORA, TX_OUTPUT_POWER, 0, LORA_BANDWIDTH,
                    LORA_SPREADING_FACTOR, LORA_CODINGRATE,
                    LORA_PREAMBLE_LENGTH, LORA_FIX_LENGTH_PAYLOAD_ON,
                    true, 0, 0, LORA_IQ_INVERSION_ON, 3000);
}

void loop() {
  Radio.IrqProcess();

  bool buttonPressed = digitalRead(prgButtonPin) == LOW;
  unsigned long now = millis();

  if (buttonPressed && buttonPressStart == 0) {
    buttonPressStart = now;
    longPressHandled = false;
  }

  if (!buttonPressed && buttonPressStart > 0) {
    if (!longPressHandled) {
      snprintf(txpacket, sizeof(txpacket), "STATUS: Front=%s Heck=%s Innen=%s Pumpe=%s",
        channelStatus[0] ? "an" : "aus",
        channelStatus[1] ? "an" : "aus",
        channelStatus[2] ? "an" : "aus",
        channelStatus[3] ? "an" : "aus");

      Serial.printf("Sende Paket: %s\n", txpacket);
      if (lora_idle) {
        Radio.Send((uint8_t*)txpacket, strlen(txpacket));
        lora_idle = false;
      }
    }
    buttonPressStart = 0;
  }

  if (buttonPressed && !longPressHandled && now - buttonPressStart >= 2000) {
    int ch = random(0, 4);
    channelStatus[ch] = !channelStatus[ch];
    Serial.printf("Zufallsstatus: %s auf %s\n", channelNames[ch], channelStatus[ch] ? "an" : "aus");
    myDisplay.clear();
    myDisplay.drawString(0, 0, "Status geaendert:");
    myDisplay.drawString(0, 12, String(channelNames[ch]) + ": " + (channelStatus[ch] ? "an" : "aus"));
    myDisplay.display();
    delay(1000);
    updateDisplayStatus();
    longPressHandled = true;
  }

  if (millis() - lastDisplayUpdate > displayInterval) {
    lastDisplayUpdate = millis();
    updateDisplayStatus();
  }
}
