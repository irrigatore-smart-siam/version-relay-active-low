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
unsigned long pumpTimer = 0;
bool pumpOn = false;
unsigned long lastActivationTime = 0;
float lastWaterLevel = 0;

BlynkTimer timer;

// Parametri per diagnostica e correzione sonar
const float SOUND_SPEED = 0.0343; // cm/microsecondo
const float MAX_TANK_DEPTH = 10.0; // profondità massima del serbatoio in cm
float lastWaterPercentage = 0;

void checkMoisture() {
  int moistureValue = analogRead(MOISTURESENSORPIN);
  
  unsigned long currentTime = millis();
  
   //Attende 10 secondi prima di ripetere il ciclo, ma noi metteremo 24 ore (ovvero 86400000)
  if (moistureValue > SOGLIA_UM && !pumpOn && (currentTime - lastActivationTime >= 10000)) {
    Serial.println("->ATTIVAZIONE POMPA");
    digitalWrite(RELAYPIN, HIGH);
    pumpOn = true;
    pumpTimer = currentTime;
    lastActivationTime = currentTime;
  }
}

void checkPumpTimer() {
  if (pumpOn && (millis() - pumpTimer >= 1000)) {
    Serial.println("->SPEGNIMENTO POMPA");
    digitalWrite(RELAYPIN, LOW);
    pumpOn = false;
  }
}

float measureDistance() {
  digitalWrite(TRIGPIN, LOW);
  delayMicroseconds(2);
  
  // Genera impulso trigger
  digitalWrite(TRIGPIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIGPIN, LOW);
  
  // Misura durata dell'eco con timeout più lungo
  long duration = pulseIn(ECHOPIN, HIGH, 38000); // Timeout 38ms
  
  // Calcola distanza se duration è valido
  if (duration > 0) {
    float distance = duration * SOUND_SPEED / 2; // Conversione in cm

    return constrain(MAX_TANK_DEPTH - distance, 0, MAX_TANK_DEPTH);
  } else {
    Serial.println("ERRORE: Nessun impulso rilevato dal sensore ultrasuoni!");
    return -1; // Errore
  }
}

float getInverseWaterPercentage(float distance) {
  distance = constrain(distance, 0, MAX_TANK_DEPTH); // Limita tra 0 e 10 cm
  return 100.0 - ((distance / MAX_TANK_DEPTH) * 100.0);
}

void checkWaterLevel() {
  float distance = measureDistance();
  
    if (distance >= 0 && distance <= MAX_TANK_DEPTH) {
      float waterPercentage = getInverseWaterPercentage(distance);
      lastWaterPercentage = waterPercentage;
      
      Serial.print("Distanza: ");
      Serial.print(distance);
      Serial.print(" cm → Livello: ");
      Serial.print(waterPercentage);
      Serial.println("%");
      
      // Solo se il livello è superiore al 10% invia a Blynk
      if (waterPercentage > 10.0) {
        Blynk.virtualWrite(V1, waterPercentage); // Invia la percentuale a Blynk
        
        // Soglie in percentuale (50% = 5 cm, 0% = 10 cm)
        if (waterPercentage > 50) { // Serbatoio più che mezzo pieno
          Blynk.virtualWrite(V3, 0);    // Rosso spento
          Blynk.virtualWrite(V4, 255);  // Verde acceso
          digitalWrite(LEDRGB2, HIGH);  // LED verde ON
          digitalWrite(LEDRGB, LOW);    // LED rosso OFF
        } 
        else { // Serbatoio sotto il 50%
          Blynk.virtualWrite(V3, 255);  // Rosso acceso
          Blynk.virtualWrite(V4, 0);    // Verde spento
          digitalWrite(LEDRGB, HIGH);   // LED rosso ON
          digitalWrite(LEDRGB2, LOW);   // LED verde OFF
        }
      }
      else {
        Serial.println("Livello acqua inferiore al 10%, non inviato a Blynk");
        // Mantieni lo stato LED in modalità "basso livello" (sotto il 50%)
        Blynk.virtualWrite(V3, 255);  // Rosso acceso
        Blynk.virtualWrite(V4, 0);    // Verde spento
        digitalWrite(LEDRGB, HIGH);   // LED rosso ON
        digitalWrite(LEDRGB2, LOW);   // LED verde OFF
      }
  }
}

void sendMoistureToBlynk() {
  int moistureValue = analogRead(MOISTURESENSORPIN);
  Serial.print("Umidità: ");
  Serial.println(moistureValue);
    
  Blynk.virtualWrite(V2, moistureValue);
}

void setup() {
  // Configurazione dei pin
  pinMode(RELAYPIN, OUTPUT);
  pinMode(TRIGPIN, OUTPUT);
  pinMode(ECHOPIN, INPUT_PULLUP);
  pinMode(LEDRGB, OUTPUT);
  pinMode(LEDRGB2, OUTPUT);
  pinMode(ANALOG_OUT_PIN, OUTPUT);
  pinMode(MOISTURESENSORPIN, INPUT);

  // Inizializzazione stato pin
  digitalWrite(RELAYPIN, LOW);
  digitalWrite(LEDRGB, LOW);
  digitalWrite(LEDRGB2, LOW);
  analogWrite(ANALOG_OUT_PIN, 0);

  // Inizializza comunicazione seriale
  Serial.begin(115200);
  
  Serial.println("Avvio del sistema di irrigazione - MASTER");

  // Connessione Blynk
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

  // Controllo immediato dell'umidità
  int moistureValue = analogRead(MOISTURESENSORPIN);
  Serial.print("Umidità iniziale: ");
  Serial.println(moistureValue);

  if (moistureValue > SOGLIA_UM) {
    Serial.println("-> ATTIVAZIONE POMPA (Startup)");
    digitalWrite(RELAYPIN, HIGH);
    pumpOn = true;
    pumpTimer = millis();
    lastActivationTime = millis();
  }

  // Configurazione timer
  timer.setInterval(100L, checkPumpTimer);
  timer.setInterval(5000L, checkWaterLevel);
  timer.setInterval(10000L, sendMoistureToBlynk);
}

void loop() {
  Blynk.run();
  timer.run();
  
  checkMoisture();

  delay(10);
}