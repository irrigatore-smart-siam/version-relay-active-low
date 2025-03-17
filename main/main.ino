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
const uint8_t LEDRGB = 11;            // LED RGB per indicare se c'è acqua o meno nel serbatoio (in mancanza o in aggiunta ad alexa)
const uint8_t LEDRGB2 = 12;           // LED RGB per indicare se c'è acqua o meno nel serbatoio (in mancanza o in aggiunta ad alexa)
const uint8_t PIEZO = 1;              // Piezo elettrico sonoro che abbiamo usato a lezione per indicare se c'è acqua o meno nel serbatoio (in mancanza o in aggiunta ad alexa)

const uint16_t SOGLIA_UM = 7000;

BlynkTimer timer;

// Funzione di callback per spegnere l'elettrovalvola
void turnOffValve() {
  digitalWrite(REDLEDPIN, LOW);
}

void checkMoisture() {
  int moistureValue = analogRead(MOISTURESENSORPIN);
  Serial.print("Umidità: ");
  Serial.println(moistureValue);
    
  Blynk.virtualWrite(V2, moistureValue); // invia il valore dell'umidità a Blynk su V2

  if (moistureValue <= SOGLIA_UM) {
    digitalWrite(REDLEDPIN, HIGH); //"accensione" elettrovalvola per irrigazione
    // Utilizzo di un timer invece di delay per non bloccare l'esecuzione
    timer.setTimeout(1000L, turnOffValve);
  } else {
    digitalWrite(REDLEDPIN, LOW);
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

  // Controllo dei LED e del piezo
  if (distance > 4) {
      Blynk.virtualWrite(V3, 255);   // LED display blynk Rosso ACCESO
      Blynk.virtualWrite(V4, 0);     // LED display blynk Verde SPENTO
      Blynk.logEvent("alexa_alert", "Riempire contenitore di acqua"); //attiva Alexa con messaggio

      digitalWrite(LEDRGB, HIGH);  // LED RGB Rosso
      digitalWrite(LEDRGB2, LOW);  // LED RGB Verde spento
      digitalWrite(PIEZO, HIGH);   // piezo elettrico suona
  } else if (distance > 1 && distance < 4) {
      Blynk.virtualWrite(V3, 0);     // LED display blynk Rosso SPENTO
      Blynk.virtualWrite(V4, 255);   // LED display blynk Verde ACCESO

      digitalWrite(LEDRGB, LOW);   // LED RGB Rosso spento
      digitalWrite(LEDRGB2, HIGH); // LED RGB Verde acceso
      digitalWrite(PIEZO, LOW);    // piezo elettrico non suona
  }
}

void setup() {
  //Configurazione dei pin
  pinMode(REDLEDPIN, OUTPUT);
  pinMode(TRIGPIN, OUTPUT);
  pinMode(ECHOPIN, INPUT);
  pinMode(LEDRGB, OUTPUT);
  pinMode(LEDRGB2, OUTPUT);
  pinMode(PIEZO, OUTPUT);

  // Inizializzazione stato pin per risparmiare energia
  digitalWrite(REDLEDPIN, LOW);
  digitalWrite(LEDRGB, LOW);
  digitalWrite(LEDRGB2, LOW);
  digitalWrite(PIEZO, LOW);

  Serial.begin(9600);

  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

  // Ciclo do-while, prima parte = do
  timer.setInterval(0L, checkMoisture);    // primo controllo umidità terreno  alla partenza
  timer.setInterval(0L, checkWaterLevel);  // primo controllo livello dell'acqua alla partenza
}

void loop() {
  Blynk.run();
  timer.run();

  // Ciclo do-while, seconda parte = while | definitivo -> ogni 24 ore = 86400000L (?)
  timer.setInterval(10000L, checkMoisture);
  timer.setInterval(10000L, checkWaterLevel);

  // Risparmio energetico durante l'inattività, picccolo delay per evitare il consumo eccessivo della CPU
  delay(10);
 } 