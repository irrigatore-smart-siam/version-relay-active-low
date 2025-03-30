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
const float MAX_TANK_DEPTH = 10.0; // profondità massima del serbatoio in cm

void checkMoisture() {
  int moistureValue = analogRead(MOISTURESENSORPIN);
  
  unsigned long currentTime = millis();
  
  if (moistureValue > SOGLIA_UM && !pumpOn && (currentTime - lastActivationTime >= 30000)) { //Attende 30 secondi prima di ripetere il ciclo, ma noi metteremo 24 ore (ovvero 86.400.000)
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
  // Debug dettagliato per sensore a ultrasuoni
  Serial.println("Inizio misurazione distanza*");
  
  // Assicurati che il pin trigger sia basso
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
    float distance = duration * 0.034 / 2; // Conversione in cm
    
    //Serial.print("Durata impulso: ");
    //Serial.print(duration);
    Serial.print("Distanza misurata: ");
    Serial.print(distance);
    Serial.println(" cm");
    
    // Inverti la distanza se necessario
    return constrain(MAX_TANK_DEPTH - distance, 0, MAX_TANK_DEPTH);
  } else {
    Serial.println("ERRORE: Nessun impulso rilevato dal sensore ultrasuoni!");
    return -1; // Errore
  }
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
  // Configurazione dei pin
  pinMode(RELAYPIN, OUTPUT);
  pinMode(TRIGPIN, OUTPUT);
  pinMode(ECHOPIN, INPUT_PULLUP);  // Aggiunto INPUT_PULLUP per stabilità
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

  // Test iniziale sensore ultrasuoni
  Serial.println("Test iniziale sensore ultrasuoni:");
  measureDistance();
}

void loop() {
  Blynk.run();
  timer.run();
  
  checkMoisture();

  delay(10);
}