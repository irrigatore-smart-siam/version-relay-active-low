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
#define MOISTURESENSORPIN 7    // Sensore di umidità
#define REDLEDPIN 9            // elettrovalvola
#define TRIGPIN 3              // Trigger del sensore ultrasuoni
#define ECHOPIN 5              // Echo del sensore ultrasuoni
#define LEDRGB = 11; //Led RGB per indicare se c'è acqua o meno nel serbatoio (in mancanza o in aggiunta ad alexa)
#define LEDRGB2 = 12; //Led RGB per indicare se c'è acqua o meno nel serbatoio (in mancanza o in aggiunta ad alexa)
#define PIEZO = 1; //piezoelettrico sonoro che abbiamo usato a lezione per indicare se c'è acqua o meno nel serbatoio (in mancanza o in aggiunta ad alexa)

#define SOGLIA_UM 7000 

BlynkTimer timer;

void checkMoisture() {
  int moistureValue = analogRead(moistureSensorPin);
  Serial.print("Umidità: ");
  Serial.println(moistureValue);
    
  Blynk.virtualWrite(V2, moistureValue); // Invia il valore dell'umidità a Blynk su V2

  /*

  Attiva il LED giallo solo se l'umidità è sotto la soglia (da tenere ?)

  if (moistureValue <= SOGLIA_UM) {
     Blynk.virtualWrite(V5, 255); // LED GIALLO (Alexa_Terreno) acceso
  } else {
    Blynk.virtualWrite(V5, 0);   // LED GIALLO spento
  }
    
  */
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

  // Controllo dei LED e del piezo
  if (distance > 4) {
      Blynk.virtualWrite(V3, 255);   // LED display blynk Rosso ACCESO
      Blynk.virtualWrite(V4, 0);     // LED display blynk Verde SPENTO
      Blynk.logEvent("alexa_alert", "Riempire contenitore di acqua"); // Attiva Alexa con messaggio

      digitalWrite(LEDRGB, HIGH);  // LED RGB Rosso
      digitalWrite(LEDRGB2, LOW);  // LED RGB Verde spento
      digitalWrite(PIEZO, HIGH);   // Piezoelettrico suona
  } else {
      Blynk.virtualWrite(V3, 0);     // LED display blynk Rosso SPENTO
      Blynk.virtualWrite(V4, 255);   // LED display blynk Verde ACCESO

      digitalWrite(LEDRGB, LOW);   // LED RGB Rosso spento
      digitalWrite(LEDRGB2, HIGH); // LED RGB Verde acceso
      digitalWrite(PIEZO, LOW);    // Piezoelettrico non suona
  }
}

void setup() {
  // Configurazione dei pin
  pinMode(REDLEDPIN, OUTPUT);
  pinMode(TRIGPIN, OUTPUT);
  pinMode(ECHOPIN, INPUT);
  pinMode(LEDRGB, OUTPUT);
  pinMode(LEDRGB2, OUTPUT);
  pinMode(PIEZO, OUTPUT);

  Serial.begin(9600);

  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

  timer.setInterval(5000L, checkMoisture);    // Controlla l'umidità dopo 5 secondi -> x "do-while finale"
  timer.setInterval(5000L, checkWaterLevel);  // Controlla il livello dell'acqua dopo 5 secondi -> x "do-while finale"
}

void loop() {
    Blynk.run();
    timer.run();
    
    //Irrigazione temporizzata, redLedPin = elettrovalvola -> versione finale ogni 24 ore (?)
    if (moistureValue > SOGLIA_UM) {
        digitalWrite(redLedPin, HIGH);
        delay(3000);
        digitalWrite(redLedPin, LOW);
        delay(10000); // Attende 10 secondi prima di ripetere il ciclo, ma noi metteremo 24 ore (ovvero 86.400.000)
    } else {
        digitalWrite(redLedPin, LOW);
        delay(10000); // Attende 10 secondi prima di ripetere il ciclo, ma noi metteremo 24 ore (ovvero 86.400.000)
    }
}
