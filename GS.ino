#include <WiFi.h>
#include <PubSubClient.h>
#include "DHT.h"
#include "HX711.h"

const char *SSID = "Wokwi-GUEST";
const char *PASSWORD = "";
const char *BROKER_MQTT = "46.17.108.113";
int BROKER_PORT = 1883;
const char *ID_MQTT = "fiware_240";

// HX711 circuit wiring
const int LOADCELL_DOUT_PIN = 12;
const int LOADCELL_SCK_PIN = 14;

HX711 scale;

// DHT 11 (Sensor de umidade e temperatura)
#define DHTTYPE DHT22
#define DHTPIN 4
DHT dht(DHTPIN, DHTTYPE);

// Saídas Leds
int D4 = 2; // Led builtin on ESP32 (onboard LED)

WiFiClient espClient;
PubSubClient MQTT(espClient);
char EstadoSaida = '0';

void initSerial();
void initWiFi();
void initMQTT();
void reconectWiFi();
void mqtt_callback(char *topic, byte *payload, unsigned int length);
void VerificaConexoesWiFIEMQTT(void);
void InitOutput(void);
void reconnectMQTT();
void EnviaEstadoOutputMQTT(void);

void setup()
{
  initSerial();
  initWiFi();
  initMQTT();
  dht.begin();
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);

  // Initializing the output in a low logical level
  InitOutput();
  MQTT.publish("/TEF/hosp240/attrs", "s|off");
}

void loop()
{
  // HX711 sensor reading
  if (scale.is_ready())
  {
    double reading = scale.read();
    double peso = reading * 0.002380952;
    Serial.print("- Leitura de HX711: ");
    Serial.println(peso);

    // Publish the exact weight to the broker
    char weightString[4];
    sprintf(weightString, "%f", peso);
    MQTT.publish("/TEF/hosp240/attrs/p", weightString);
    Serial.println("- Peso enviado para o broker como " + String(peso,2) );

    // Check if peso is greater than or equal to 2.50 and turn on the LED
    if (peso >= 2.50)
    {
      digitalWrite(D4, HIGH);
      EstadoSaida = '1';
      Serial.println("- Led Ligado (Peso >= 2.50)");
    }
    else
    {
      digitalWrite(D4, LOW);
      EstadoSaida = '0';
      Serial.println("- Led Desligado (Peso < 2.50)");
    }
  }
  else
  {
    Serial.println("HX711 not found.");
  }

  // MQTT and WiFi connection check and handling
  VerificaConexoesWiFIEMQTT();

  // Sending the status of all outputs to the MQTT Broker
  EnviaEstadoOutputMQTT();

  // Luminosity reading
  // ...

  // Humidity and Temperature reading
  // ...

  // keep-alive for communication with MQTT broker
  MQTT.loop();

  delay(1000);
}

void initSerial()
{
  Serial.begin(115200);
}

void initWiFi()
{
  delay(10);
  Serial.println("------Conexao WI-FI------");
  Serial.print("Conectando-se na rede: ");
  Serial.println(SSID);
  Serial.println("Aguarde");

  reconectWiFi();
}

void initMQTT()
{
  MQTT.setServer(BROKER_MQTT, BROKER_PORT);
  MQTT.setCallback(mqtt_callback);
}

void reconectWiFi()
{
  if (WiFi.status() == WL_CONNECTED)
    return;

  WiFi.begin(SSID, PASSWORD);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(100);
    Serial.print(".");
  }

  Serial.println();
  Serial.print("Conectado com sucesso na rede ");
  Serial.print(SSID);
  Serial.println("IP obtido: ");
  Serial.println(WiFi.localIP());
}

void mqtt_callback(char *topic, byte *payload, unsigned int length)
{
  // MQTT callback implementation
  // ...
}

void VerificaConexoesWiFIEMQTT(void)
{
  if (!MQTT.connected())
    reconnectMQTT();

  reconectWiFi();
}

void reconnectMQTT()
{
  while (!MQTT.connected())
  {
    Serial.print("* Tentando se conectar ao Broker MQTT: ");
    Serial.println(BROKER_MQTT);
    if (MQTT.connect(ID_MQTT))
    {
      Serial.println("Conectado com sucesso ao broker MQTT!");
      MQTT.subscribe("/TEF/hosp240/cmd");
    }
    else
    {
      Serial.println("Falha ao reconectar no broker.");
      Serial.println("Havera nova tentativa de conexao em 2s");
      delay(2000);
    }
  }
}

void EnviaEstadoOutputMQTT(void)
{
  if (EstadoSaida == '1')
  {
    MQTT.publish("/TEF/hosp240/attrs", "s|on");
    Serial.println("- Led Ligado");
  }
  if (EstadoSaida == '0')
  {
    MQTT.publish("/TEF/hosp240/attrs", "s|off");
    Serial.println("- Led Desligado");
  }
  Serial.println("- Estado do LED onboard enviado ao broker!");
  Serial.println("=========================================================================");
  delay(1000);
}

void InitOutput(void)
{
  pinMode(D4, OUTPUT);
  digitalWrite(D4, HIGH);

  boolean toggle = false;

  for (int i = 0; i <= 10; i++)
  {
    toggle = !toggle;
    digitalWrite(D4, toggle);
    delay(200);
  }

  digitalWrite(D4, LOW);
}
