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
const uint8_t RELAYPIN = 9;           // Pompa
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

// Parametri per diagnostica e correzione sonar
const int NUM_MEASUREMENTS = 10;
const float SOUND_SPEED = 0.0343; // cm/microsecondo
const float MAX_TANK_DEPTH = 12.0; // profondità massima del serbatoio in cm

void checkPumpTimer() {
  if (pumpOn && (millis() - pumpTimer >= 1000)) {
    Serial.println("Spegnimento pompa");
    digitalWrite(RELAYPIN, LOW);
    pumpOn = false;
  }
}

void checkMoisture() {
  int moistureValue = analogRead(MOISTURESENSORPIN);
  
  unsigned long currentTime = millis();
  
  if (moistureValue > SOGLIA_UM && !pumpOn && (currentTime - lastActivationTime >= 30000)) {
    Serial.println("Attivazione pompa");
    digitalWrite(RELAYPIN, HIGH);
    pumpOn = true;
    pumpTimer = currentTime;
    lastActivationTime = currentTime;
  }
}

float measureDistance() {
  // Funzione diagnostica per misurare la distanza
  long totalDuration = 0;
  int validMeasurements = 0;
  
  for (int i = 0; i < NUM_MEASUREMENTS; i++) {
    // Genera impulso trigger
    digitalWrite(TRIGPIN, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIGPIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIGPIN, LOW);
    
    // Misura durata dell'eco
    long duration = pulseIn(ECHOPIN, HIGH, 30000); // Timeout 30ms
    
    // Filtra misurazioni errate
    if (duration > 0 && duration < 30000) {
      totalDuration += duration;
      validMeasurements++;
    }
    
    delay(10);
  }
  
  // Calcola distanza media
  if (validMeasurements > 0) {
    float avgDuration = totalDuration / validMeasurements;
    float distance = (avgDuration * SOUND_SPEED) / 2;
    
    // Debug diagnostico
    Serial.print("Misurazioni valide: ");
    Serial.print(validMeasurements);
    Serial.print("Distanza misurata: ");
    Serial.print(distance);
    Serial.println(" cm");
    
    // Inverti la distanza se necessario
    return MAX_TANK_DEPTH - distance;
  }
  
  return -1; // Errore
}

void checkWaterLevel() {
  float waterLevel = measureDistance();
  
  if (waterLevel >= 0 && waterLevel <= MAX_TANK_DEPTH) {
    Serial.print("Livello acqua: ");
    Serial.print(waterLevel);
    Serial.println(" cm");
    
    // Invia a Blynk
    Blynk.virtualWrite(V1, waterLevel);
    
    // Logica LED e controllo livello
    if (waterLevel > 4) {
      Blynk.virtualWrite(V3, 255);   // LED Rosso
      Blynk.virtualWrite(V4, 0);     // LED Verde spento
      
      digitalWrite(LEDRGB, HIGH);    // LED RGB Rosso
      digitalWrite(LEDRGB2, LOW);    // LED RGB Verde spento
      
      // Invia valore analogico allo slave
      int analogValue = map(constrain(waterLevel * 10, 10, 200), 10, 200, 0, 255);
      analogWrite(ANALOG_OUT_PIN, analogValue);
      Serial.print("Valore analogico: ");
      Serial.println(analogValue);
    } 
    else if (waterLevel > 1 && waterLevel < 4) {
      Blynk.virtualWrite(V3, 0);     // LED Rosso spento
      Blynk.virtualWrite(V4, 255);   // LED Verde acceso
      digitalWrite(LEDRGB, LOW);     // LED RGB Rosso spento
      digitalWrite(LEDRGB2, HIGH);   // LED RGB Verde acceso
    }
  }
}

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
  pinMode(ANALOG_OUT_PIN, OUTPUT);

  // Inizializzazione stato pin
  digitalWrite(RELAYPIN, LOW);
  digitalWrite(LEDRGB, LOW);
  digitalWrite(LEDRGB2, LOW);
  analogWrite(ANALOG_OUT_PIN, 0);

  Serial.begin(115200);
  
  Serial.println("Avvio del sistema di irrigazione - MASTER");

  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

  // Configurazione timer
  timer.setInterval(100L, checkPumpTimer);
  timer.setInterval(10000L, checkWaterLevel);
  timer.setInterval(10000L, sendMoistureToBlynk);
}

void loop() {
  Blynk.run();
  timer.run();
  
  checkMoisture();

  delay(10);
}