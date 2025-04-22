// Bibliotheken für LoRaWAN und das OLED-Display einbinden
#include "LoRaWan_APP.h"
// Bibliotheken für LoRaWAN und das OLED-Display einbinden
#include "HT_SSD1306Wire.h"
// Bibliotheken für LoRaWAN und das OLED-Display einbinden
#include "Arduino.h"

// LoRa-Parameter
#define RF_FREQUENCY        433775000
#define TX_OUTPUT_POWER     15
#define LORA_BANDWIDTH      0
#define LORA_SPREADING_FACTOR 9
#define LORA_CODINGRATE     1
#define LORA_PREAMBLE_LENGTH 8
#define LORA_SYMBOL_TIMEOUT 0
#define LORA_FIX_LENGTH_PAYLOAD_ON false
#define LORA_IQ_INVERSION_ON false

// PRG-Taste
const int prgButtonPin = 0;

// Display
#define OLED_ADDR 0x3c
SSD1306Wire myDisplay(OLED_ADDR, 400000, SDA_OLED, SCL_OLED, GEOMETRY_128_64, RST_OLED);

// Lora TX
#define BUFFER_SIZE 64
char txpacket[BUFFER_SIZE];
double txNumber = 0;
bool lora_idle = true;

static RadioEvents_t RadioEvents;

// Anzeige-Logik
const char* channelNames[] = {"Front", "Heck", "Innen", "Pumpe"};
int channelStatus[] = {1, 0, 1, 0};
int currentChannel = 0;
unsigned long lastDisplayUpdate = 0;
const unsigned long displayInterval = 2000;

// Callback für TX abgeschlossen
void OnTxDone() {
// Rückmeldung nach erfolgreicher Sendung
  Serial.println("TX abgeschlossen.");
  lora_idle = true;
}

// Callback für TX Timeout
void OnTxTimeout() {
// Rückmeldung bei Sendefehler (Timeout)
  Serial.println("TX Timeout!");
  lora_idle = true;
}

void VextON() {
// Aktiviere die externe Spannungsversorgung für das Display und das LoRa-Modul
  pinMode(Vext, OUTPUT);
  digitalWrite(Vext, LOW);
}

void setup() {
  Serial.begin(115200);
// Kurze Pause, um ungewolltes mehrfaches Auslösen durch Taster-Prellen zu vermeiden
  delay(1000);
  Serial.println("Starte Bernd...");

  pinMode(prgButtonPin, INPUT_PULLUP);

  VextON();
  delay(100);

  // Display initialisieren
  myDisplay.init();
  myDisplay.flipScreenVertically();
  myDisplay.setFont(ArialMT_Plain_10);
  myDisplay.setTextAlignment(TEXT_ALIGN_LEFT);
  myDisplay.clear();
  myDisplay.drawString(0, 0, "Display bereit");
  myDisplay.display();

  // LoRa initialisieren
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
  if (lora_idle && digitalRead(prgButtonPin) == LOW) {
    delay(500); // Entprellung
    txNumber += 0.01;
    snprintf(txpacket, sizeof(txpacket), "Statuspaket Nr. %0.2f", txNumber);
    Serial.printf("Sende Paket: %s\n", txpacket);
// Sende das vorbereitete Paket über das LoRa-Radio
    Radio.Send((uint8_t*)txpacket, strlen(txpacket));
    lora_idle = false;
  }

// Verarbeite LoRa-Ereignisse, wie Sende- oder Empfangsabschluss
  Radio.IrqProcess();

  if (millis() - lastDisplayUpdate > displayInterval) {
    lastDisplayUpdate = millis();
    myDisplay.clear();
    myDisplay.drawString(0, 0, "Aktueller Kanal:");
    myDisplay.drawString(0, 12, channelNames[currentChannel]);
    myDisplay.drawString(0, 24, String("Status: ") + (channelStatus[currentChannel] ? "an" : "aus"));
    myDisplay.display();
    currentChannel = (currentChannel + 1) % 4;
  }
}
