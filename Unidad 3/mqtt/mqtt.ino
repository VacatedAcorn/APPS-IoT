#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <WiFiClientSecure.h>
#include <DHT.h>

// WiFi
const char *ssid = "W_Aula_WB11";   // Enter your WiFi name
const char *password = "itcolima6"; // Enter WiFi password


// MQTT Broker
const char *mqttServer = "w3a4bbd9.ala.us-east-1.emqxsl.com";    // broker address
const int mqttPort = 8883;                                       // port of MQTT over TLS/SSL
const char *mqttUser = "server";                              // username for authentication
const char *mqttPassword = "password";                           // password for authentication
const char *mqttTopic = "monitores/web_bueno";                            // define topic

const char* ca_cert = \
"-----BEGIN CERTIFICATE-----\n" \
"MIIDrzCCApegAwIBAgIQCDvgVpBCRrGhdWrJWZHHSjANBgkqhkiG9w0BAQUFADBh\n" \
"MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3\n" \
"d3cuZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBD\n" \
"QTAeFw0wNjExMTAwMDAwMDBaFw0zMTExMTAwMDAwMDBaMGExCzAJBgNVBAYTAlVT\n" \
"MRUwEwYDVQQKEwxEaWdpQ2VydCBJbmMxGTAXBgNVBAsTEHd3dy5kaWdpY2VydC5j\n" \
"b20xIDAeBgNVBAMTF0RpZ2lDZXJ0IEdsb2JhbCBSb290IENBMIIBIjANBgkqhkiG\n" \
"9w0BAQEFAAOCAQ8AMIIBCgKCAQEA4jvhEXLeqKTTo1eqUKKPC3eQyaKl7hLOllsB\n" \
"CSDMAZOnTjC3U/dDxGkAV53ijSLdhwZAAIEJzs4bg7/fzTtxRuLWZscFs3YnFo97\n" \
"nh6Vfe63SKMI2tavegw5BmV/Sl0fvBf4q77uKNd0f3p4mVmFaG5cIzJLv07A6Fpt\n" \
"43C/dxC//AH2hdmoRBBYMql1GNXRor5H4idq9Joz+EkIYIvUX7Q6hL+hqkpMfT7P\n" \
"T19sdl6gSzeRntwi5m3OFBqOasv+zbMUZBfHWymeMr/y7vrTC0LUq7dBMtoM1O/4\n" \
"gdW7jVg/tRvoSSiicNoxBN33shbyTApOB6jtSj1etX+jkMOvJwIDAQABo2MwYTAO\n" \
"BgNVHQ8BAf8EBAMCAYYwDwYDVR0TAQH/BAUwAwEB/zAdBgNVHQ4EFgQUA95QNVbR\n" \
"TLtm8KPiGxvDl7I90VUwHwYDVR0jBBgwFoAUA95QNVbRTLtm8KPiGxvDl7I90VUw\n" \
"DQYJKoZIhvcNAQEFBQADggEBAMucN6pIExIK+t1EnE9SsPTfrgT1eXkIoyQY/Esr\n" \
"hMAtudXH/vTBH1jLuG2cenTnmCmrEbXjcKChzUyImZOMkXDiqw8cvpOp/2PV5Adg\n" \
"06O/nVsJ8dWO41P0jmP6P6fbtGbfYmbW0W5BjfIttep3Sp+dWOIrWcBAI+0tKIJF\n" \
"PnlUkiaY4IBIqDfv8NZ5YBberOgOzW6sRBc4L0na4UU+Krk2U886UAb3LujEV0ls\n" \
"YSEY1QSteDwsOoBrp+uvFRTp2InBuThs4pFsiv9kuXclVzDAGySj4dzp30d8tbQk\n" \
"CAUw7C29C79Fv1C5qfPrmAESrciIxpg0X40KPMbp1ZWVbd4=" \
"-----END CERTIFICATE-----\n";


WiFiClientSecure espClient;
PubSubClient client(espClient);

int valor = 0; // Este es el valor que incrementarás o decrementarás

// Pines de los botones
const int pinBotonIncrementar = 4;  
const int pinBotonDecrementar = 5;  
const int pinLED = 21;

// Configuración del sensor DHT11
#define DHTPIN 2  // Ejemplo, ajusta según tu configuración
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);

unsigned long tiempoAnterior = 0;
const long intervaloEnvio = 30000;

void setup() {
  Serial.begin(115200);

  // Inicializa los pines de los botones como entrada
  pinMode(pinBotonIncrementar, INPUT_PULLUP);
  pinMode(pinBotonDecrementar, INPUT_PULLUP);
  pinMode(pinLED, OUTPUT);
  dht.begin();

  // Conexión a la red WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Conectando a WiFi...");
  }
  Serial.println("Conectado a la red WiFi");

  // Configuración del cliente MQTT con TLS/SSL
  espClient.setCACert(ca_cert);
  client.setServer(mqttServer, mqttPort);
  client.setCallback(callback);

  while (!client.connected()) {
    String client_id = "esp32-client-";
    client_id += String(WiFi.macAddress());
    Serial.printf("El cliente %s se conecta al servidor MQTT\n", client_id.c_str());

    if (client.connect(client_id.c_str(), mqttUser, mqttPassword)) {
      Serial.println("Conexión exitosa al servidor MQTT");
    } else {
      Serial.print("Error de conexión, rc=");
      Serial.print(client.state());
      Serial.println(" Intentando de nuevo en 2 segundos");
      delay(2000);
    }
  }

  // Publicación y suscripción
  client.publish(mqttTopic, "¡Hola, soy ESP32 con TLS/SSL ^^!");
  client.subscribe(mqttTopic);
}

void loop() {
  // Lee el estado de los botones
  int estadoBotonIncrementar = digitalRead(pinBotonIncrementar);
  int estadoBotonDecrementar = digitalRead(pinBotonDecrementar);

  // Incrementa o decrementa el valor según el estado de los botones
  if (estadoBotonIncrementar == HIGH) {
    incrementarValor();
    delay(500);  // Evita múltiples incrementos rápidos con un pequeño retardo
  }

  if (estadoBotonDecrementar == HIGH) {
    decrementarValor();
    delay(500);  // Evita múltiples decrementos rápidos con un pequeño retardo
  }

  // Verifica el intervalo de envío para los datos del sensor DHT11
  unsigned long tiempoActual = millis();
  if (tiempoActual - tiempoAnterior >= intervaloEnvio) {
    // Lee datos del sensor DHT11
    float temperatura = dht.readTemperature();
    float humedad = dht.readHumidity();

    // Publica los datos en el tema MQTT
    enviarDatosDHT(temperatura, humedad);

    // Actualiza el tiempo anterior
    tiempoAnterior = tiempoActual;
  }

  // Continúa con el loop del cliente MQTT
  client.loop();
}

void callback(char* topic, byte* payload, unsigned int length) {
   // Este es el manejador de mensajes MQTT de entrada
  Serial.print("Mensaje recibido en el tema: ");
  Serial.println(topic);

  // Convierte el payload en una cadena
  String message;
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  Serial.print("Mensaje: ");
  Serial.println(message);

  // Procesa el mensaje JSON
  DynamicJsonDocument doc(1024);
  deserializeJson(doc, message);

  // Verifica si el mensaje contiene la propiedad "led"
  if (doc.containsKey("led")) {
    int estadoLED = doc["led"];
    if (estadoLED == 0) {
      digitalWrite(pinLED, LOW);  // Apagar LED
    } else if (estadoLED == 1) {
      digitalWrite(pinLED, HIGH);  // Encender LED
    }
  }
}

void enviarValor(int valor) {
  // Crea un objeto JSON con el formato requerido
  DynamicJsonDocument doc(1024);
  doc["from"] = "ESP32";
  doc["to"] = "broadcast";
  doc["action"] = "UPDATE_COUNTER";
  doc["value"] = valor;

  // Convierte el objeto JSON en una cadena
  String jsonString;
  serializeJson(doc, jsonString);

  // Imprime el mensaje JSON en la consola
  Serial.print("Enviando JSON: ");
  Serial.println(jsonString);

  // Publica el mensaje JSON en el tema MQTT
  client.publish("monitores/web_bueno", jsonString.c_str());
}

void enviarDatosDHT(float temperatura, float humedad) {
  // Crea un objeto JSON con los datos del sensor DHT11
  DynamicJsonDocument doc(1024);
  doc["from"] = "ESP32";
  doc["to"] = "server";
  doc["action"] = "SEND_DATA";
  JsonObject data = doc.createNestedObject("data");
  data["temperature"] = temperatura;
  data["humidity"] = humedad;

  // Convierte el objeto JSON en una cadena
  String jsonString;
  serializeJson(doc, jsonString);

  // Publica el mensaje JSON en el tema MQTT
  client.publish(mqttTopic, jsonString.c_str());
}

void incrementarValor() {
  valor++;
  enviarValor(valor);
}

void decrementarValor() {
  valor--;
  enviarValor(valor);
}