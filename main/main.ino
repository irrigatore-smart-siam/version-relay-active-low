#define BLYNK_TEMPLATE_ID "TMPL4ADZxyQKy"
#define BLYNK_TEMPLATE_NAME "SiamTemplate"
#define BLYNK_AUTH_TOKEN "ivLdN-NeQ0gOY_I3in3qTkis7cu0HhD9"
#define BLYNK_PRINT Serial

#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>

char ssid[] = "IoT - WIFI";
char pass[] = "03GDVBH223";

// Definizione pin per ESP32 Lolin S2 Mini Master
const uint8_t MOISTURESENSORPIN = 7;  // Sensore di umidità
const uint8_t RELAYPIN = 9;          // Pompa
const uint8_t TRIGPIN = 3;            // Trigger del sensore a ultrasuoni
const uint8_t ECHOPIN = 5;            // Echo del sensore a ultrasuoni
const uint8_t LEDRGB = 11;            // LED RGB per indicare se c'è acqua o meno nel serbatoio
const uint8_t LEDRGB2 = 12;           // LED RGB per indicare se c'è acqua o meno nel serbatoio

// Pin per comunicazione analogica con lo slave
const uint8_t ANALOG_OUT_PIN = 10;     // Pin DAC per inviare il livello dell'acqua allo slave

const uint16_t SOGLIA_UM = 7000;
unsigned long pumpTimer = 0;  // Timer per la pompa
bool pumpOn = false;          // Stato della pompa
unsigned long lastActivationTime = 0;  // Tempo dell'ultima attivazione
float lastWaterLevel = 0;      // Ultimo livello dell'acqua misurato

BlynkTimer timer;

void checkPumpTimer() {
  // Controlla se la pompa è attiva e se è passato 1 secondo
  if (pumpOn && (millis() - pumpTimer >= 1000)) {
    Serial.println("Spegnimento pompa");
    digitalWrite(RELAYPIN, LOW);   // Spegne la pompa
    pumpOn = false;                // Reimposta lo stato a OFF
  }
}

void checkMoisture() {
  int moistureValue = analogRead(MOISTURESENSORPIN);
  
  // Controllo se è passato abbastanza tempo dall'ultima attivazione (10 secondi)
  unsigned long currentTime = millis();
  
  // Se l'umidità è sotto la soglia, la pompa non è già attiva, 
  // e sono passati almeno 10 secondi dall'ultima attivazione
  if (moistureValue > SOGLIA_UM && !pumpOn && (currentTime - lastActivationTime >= 30000)) {
    Serial.println("Attivazione pompa");
    digitalWrite(RELAYPIN, HIGH);       // Accende la pompa
    pumpOn = true;                      // Imposta lo stato a ON
    pumpTimer = currentTime;            // Memorizza il tempo per lo spegnimento
    lastActivationTime = currentTime;    // Aggiorna il tempo dell'ultima attivazione
  }
}

void checkWaterLevel() {
  long duration;
  float distance;
    
  // Generazione dell'impulso ultrasonico
  digitalWrite(TRIGPIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIGPIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIGPIN, LOW);
    
  // Lettura della durata dell'eco
  duration = pulseIn(ECHOPIN, HIGH);

  // Calcolo della distanza
  distance = (duration * 0.0343) / 2;

  Serial.print("Distanza: ");
  Serial.print(distance);
  Serial.println(" cm");

  // Invia la distanza a Blynk
  Blynk.virtualWrite(V1, distance);

  // Controllo dei LED locali
  if (distance > 4) {
    Blynk.virtualWrite(V3, 255);   // LED display blynk Rosso ACCESO
    Blynk.virtualWrite(V4, 0);     // LED display blynk Verde SPENTO
    
    digitalWrite(LEDRGB, HIGH);  // LED RGB Rosso
    digitalWrite(LEDRGB2, LOW);  // LED RGB Verde spento

    // Memorizza l'ultimo livello dell'acqua
    lastWaterLevel = distance;
    
    // Invia il valore analogico allo slave
    // Mappatura del valore di distanza (tipicamente 1-20cm) a un valore DAC (0-255)
    int analogValue = map(constrain(distance * 10, 10, 200), 10, 200, 0, 255);
    analogWrite(ANALOG_OUT_PIN, analogValue);
    Serial.print("Valore analogico inviato allo slave: ");
    Serial.println(analogValue);
  } else if (distance > 1 && distance < 4) {
    Blynk.virtualWrite(V3, 0);     // LED display blynk Rosso SPENTO
    Blynk.virtualWrite(V4, 255);   // LED display blynk Verde ACCESO
    digitalWrite(LEDRGB, LOW);   // LED RGB Rosso spento
    digitalWrite(LEDRGB2, HIGH); // LED RGB Verde acceso
  }
}

// Funzione per inviare il valore dell'umidità a Blynk
void sendMoistureToBlynk() {
  int moistureValue = analogRead(MOISTURESENSORPIN);
  Serial.print("Umidità: ");
  Serial.println(moistureValue);
    
  Blynk.virtualWrite(V2, moistureValue); // invia il valore dell'umidità a Blynk su V2
}

void setup() {
  pinMode(RELAYPIN, OUTPUT);
  pinMode(TRIGPIN, OUTPUT);
  pinMode(ECHOPIN, INPUT);
  pinMode(LEDRGB, OUTPUT);
  pinMode(LEDRGB2, OUTPUT);
  pinMode(ANALOG_OUT_PIN, OUTPUT); // Configurazione del pin analogico di output

  // Inizializzazione stato pin per risparmiare energia
  digitalWrite(RELAYPIN, LOW);
  digitalWrite(LEDRGB, LOW);
  digitalWrite(LEDRGB2, LOW);
  analogWrite(ANALOG_OUT_PIN, 0);  // Inizializza il pin analogico a 0

  Serial.begin(115200);
  
  Serial.println("Avvio del sistema di irrigazione - MASTER");

  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

  // Configurazione timer per i controlli periodici
  timer.setInterval(100L, checkPumpTimer);       // controllo timer pompa ogni 100ms
  timer.setInterval(10000L, checkWaterLevel);     // controllo livello dell'acqua ogni 10 secondi
  timer.setInterval(10000L, sendMoistureToBlynk); // invia dati umidità a Blynk ogni 10 secondi
}

void loop() {
  Blynk.run();
  timer.run();
  
  checkMoisture();

  // Risparmio energetico durante l'inattività
  delay(10);
}