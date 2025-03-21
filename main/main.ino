#define BLYNK_TEMPLATE_ID "TMPL4ADZxyQKy"
#define BLYNK_TEMPLATE_NAME "SiamTemplate"
#define BLYNK_AUTH_TOKEN "ivLdN-NeQ0gOY_I3in3qTkis7cu0HhD9"
#define BLYNK_PRINT Serial

#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>

char ssid[] = "IoT - WIFI";
char pass[] = "03GDVBH223";

// Definizione pin per ESP32 Lolin S2 Mini
const uint8_t MOISTURESENSORPIN = 7;  // Sensore di umidità
const uint8_t REDLEDPIN = 9;          // Elettrovalvola
const uint8_t TRIGPIN = 3;            // Trigger del sensore a ultrasuoni
const uint8_t ECHOPIN = 5;            // Echo del sensore a ultrasuoni
const uint8_t LEDRGB = 11;            // LED RGB per indicare se c'è acqua o meno nel serbatoio
const uint8_t LEDRGB2 = 12;           // LED RGB per indicare se c'è acqua o meno nel serbatoio

const uint16_t SOGLIA_UM = 7000;
unsigned long valveTimer = 0;  // Timer per l'elettrovalvola
bool valveOn = false;          // Stato dell'elettrovalvola

BlynkTimer timer;

// Funzione per controllare l'umidità
void checkMoisture() {
  int moistureValue = analogRead(MOISTURESENSORPIN);
  Serial.print("Umidità: ");
  Serial.println(moistureValue);
    
  Blynk.virtualWrite(V2, moistureValue); // invia il valore dell'umidità a Blynk su V2

  // Se l'umidità è sopra la soglia e l'elettrovalvola non è già attiva
  if (moistureValue > SOGLIA_UM && !valveOn) {
    Serial.println("Attivazione elettrovalvola");
    digitalWrite(REDLEDPIN, HIGH);  // Accende l'elettrovalvola
    valveOn = true;                 // Imposta lo stato a ON
    valveTimer = millis();          // Memorizza il tempo corrente
  }
}

// Funzione separata per controllare il timer dell'elettrovalvola
void checkValveTimer() {
  // Controlla se l'elettrovalvola è attiva e se è passato 1 secondo
  if (valveOn && (millis() - valveTimer >= 1000)) {
    Serial.println("Spegnimento elettrovalvola");
    digitalWrite(REDLEDPIN, LOW);   // Spegne l'elettrovalvola
    valveOn = false;                // Reimposta lo stato a OFF
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
  
  if (distance > 1) {
    Serial.print("Distanza: ");
    Serial.print(distance);
    Serial.println(" cm");
  }

  // Invia la distanza a Blynk
  Blynk.virtualWrite(V1, distance);

  // Controllo dei LED
  if (distance > 4) {
      Blynk.virtualWrite(V3, 255);   // LED display blynk Rosso ACCESO
      Blynk.virtualWrite(V4, 0);     // LED display blynk Verde SPENTO
      Blynk.logEvent("alexa_alert", "Riempire contenitore di acqua"); //attiva Alexa con messaggio

      digitalWrite(LEDRGB, HIGH);  // LED RGB Rosso
      digitalWrite(LEDRGB2, LOW);  // LED RGB Verde spento
  } else if (distance > 1 && distance < 4) {
      Blynk.virtualWrite(V3, 0);     // LED display blynk Rosso SPENTO
      Blynk.virtualWrite(V4, 255);   // LED display blynk Verde ACCESO

      digitalWrite(LEDRGB, LOW);   // LED RGB Rosso spento
      digitalWrite(LEDRGB2, HIGH); // LED RGB Verde acceso
  }
}

void setup() {
  //Configurazione dei pin
  pinMode(REDLEDPIN, OUTPUT);
  pinMode(TRIGPIN, OUTPUT);
  pinMode(ECHOPIN, INPUT);
  pinMode(LEDRGB, OUTPUT);
  pinMode(LEDRGB2, OUTPUT);

  // Inizializzazione stato pin per risparmiare energia
  digitalWrite(REDLEDPIN, LOW);
  digitalWrite(LEDRGB, LOW);
  digitalWrite(LEDRGB2, LOW);


  Serial.begin(115200);
  
  Serial.println("Avvio del sistema di irrigazione");

  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

  // Configurazione timer per i controlli periodici
  timer.setInterval(100L, checkValveTimer);    // controllo timer elettrovalvola ogni 100ms
  timer.setInterval(10000L, checkMoisture);    // controllo umidità terreno ogni 10 secondi
  timer.setInterval(10000L, checkWaterLevel);  // controllo livello dell'acqua ogni 10 secondi
}

void loop() {
  Blynk.run();
  timer.run();
  
  // Risparmio energetico durante l'inattività
  delay(10);
}