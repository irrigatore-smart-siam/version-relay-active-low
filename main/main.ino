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
const uint8_t RED_PIN = 11;  // pin per il rosso
const uint8_t GREEN_PIN = 12;  // pin per il verde
const uint8_t BLUE_PIN = 16;  // Nuovo pin per il blu

// Pin per comunicazione analogica con lo slave
const uint8_t ANALOG_OUT_PIN = 10;     // Pin DAC per inviare il livello dell'acqua allo slave

const uint16_t SOGLIA_UM = 7000;
unsigned long pumpTimer = 0;
bool pumpOn = false;
unsigned long lastActivationTime = 0;
float lastWaterLevel = 0;

// Timer per il lampeggiamento del LED blu
unsigned long blueBlinkTimer = 0;
bool blueBlinkState = false;
bool isBlueBlinking = false;
unsigned long blueBlinkStartTime = 0;
bool needsLedUpdate = false;  // Flag per forzare l'aggiornamento dei LED

BlynkTimer timer;

// Parametri per diagnostica e correzione sonar
const float SOUND_SPEED = 0.0343; // cm/microsecondo
const float MAX_TANK_DEPTH = 10.0; // profondità massima del serbatoio in cm
float lastWaterPercentage = 0;

// Dichiarazione anticipata di funzioni
float measureDistance();
float getInverseWaterPercentage(float distance);
void updateLedStatus(float waterPercentage);

void startBlueBlinking() {
    isBlueBlinking = true;
    blueBlinkStartTime = millis();
    blueBlinkTimer = millis();
    blueBlinkState = true;
    
    // Spegni gli altri LED durante il lampeggiamento del blu
    digitalWrite(RED_PIN, LOW);
    digitalWrite(GREEN_PIN, LOW);
    digitalWrite(BLUE_PIN, HIGH);   // LED blu ON inizia acceso
}

void updateLedStatus(float waterPercentage) {
    // Funzione centralizzata per aggiornare lo stato dei LED
    if (isBlueBlinking) {
        // Durante il lampeggiamento blu, gli altri LED rimangono spenti
        return;
    }
    
    // Assicurati che il LED blu sia spento
    digitalWrite(BLUE_PIN, LOW);
    
    // Gestisci i LED rosso e verde in base al livello dell'acqua
    if (waterPercentage > 50) { // Serbatoio più che mezzo pieno
        digitalWrite(GREEN_PIN, HIGH);  // LED verde ON
        digitalWrite(RED_PIN, LOW);    // LED rosso OFF
        Blynk.virtualWrite(V3, 0);    // Rosso spento
        Blynk.virtualWrite(V4, 255);  // Verde acceso
    } else { // Serbatoio sotto il 50%
        digitalWrite(RED_PIN, HIGH);    // LED rosso ON
        digitalWrite(GREEN_PIN, LOW);  // LED verde OFF
        Blynk.virtualWrite(V3, 255);  // Rosso acceso
        Blynk.virtualWrite(V4, 0);    // Verde spento
    }
}

void updateBlueBlink() {
    if (!isBlueBlinking) return;
    
    // Verifica se il tempo di lampeggiamento di 5 secondi è terminato
    if (millis() - blueBlinkStartTime >= 5000) {
        isBlueBlinking = false;
        digitalWrite(BLUE_PIN, LOW); // Spegni il LED blu
        needsLedUpdate = true;  // Imposta il flag per aggiornare i LED al prossimo ciclo
        return;
    }
    
    // Inverti lo stato del LED blu ogni 250ms
    if (millis() - blueBlinkTimer >= 250) {
        blueBlinkTimer = millis();
        blueBlinkState = !blueBlinkState;
        if (blueBlinkState) {
            digitalWrite(BLUE_PIN, HIGH); // Blu acceso
        } else {
            digitalWrite(BLUE_PIN, LOW); // Blu spento
        }
    }
}

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
    
    // Attiva il lampeggiamento del LED blu per 5 secondi
    startBlueBlinking();
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
  // Se il blu sta lampeggiando, non aggiornare la percentuale dell'acqua
  // ma memorizza che serve un aggiornamento dei LED
  if (isBlueBlinking) {
    needsLedUpdate = true;
    return;
  }
  
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
      
      // Aggiorna lo stato dei LED usando la funzione centralizzata
      updateLedStatus(waterPercentage);
    }
    else {
      Serial.println("Livello acqua inferiore al 10%, non inviato a Blynk");
      // Aggiorna lo stato dei LED con valori bassi
      updateLedStatus(0); // Considera come acqua bassa
    }
  }
}

void checkIfLedUpdateNeeded() {
  // Nuova funzione per controllare se i LED necessitano di un aggiornamento
  if (needsLedUpdate && !isBlueBlinking) {
    needsLedUpdate = false;
    
    float distance = measureDistance();
    if (distance >= 0 && distance <= MAX_TANK_DEPTH) {
      float waterPercentage = getInverseWaterPercentage(distance);
      updateLedStatus(waterPercentage);
    } else {
      // In caso di errore del sensore, usa l'ultimo valore valido
      updateLedStatus(lastWaterPercentage);
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
  pinMode(RED_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(BLUE_PIN, OUTPUT);
  pinMode(ANALOG_OUT_PIN, OUTPUT);
  pinMode(MOISTURESENSORPIN, INPUT);

  // Inizializzazione stato pin
  digitalWrite(RELAYPIN, LOW);
  digitalWrite(RED_PIN, LOW);    // LED rosso OFF
  digitalWrite(GREEN_PIN, LOW);  // LED verde OFF
  digitalWrite(BLUE_PIN, LOW); // LED blu OFF
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
    
    // Attiva il lampeggiamento del LED blu all'avvio se necessario
    startBlueBlinking();
  }

  // Configurazione timer
  timer.setInterval(100L, checkPumpTimer);
  timer.setInterval(5000L, checkWaterLevel);
  timer.setInterval(10000L, sendMoistureToBlynk);
  timer.setInterval(50L, updateBlueBlink);    // Aggiorna il lampeggiamento del LED blu
  timer.setInterval(100L, checkIfLedUpdateNeeded); // Controlla se è necessario aggiornare i LED
}

void loop() {
  Blynk.run();
  timer.run();
  
  checkMoisture();

  delay(10);
}