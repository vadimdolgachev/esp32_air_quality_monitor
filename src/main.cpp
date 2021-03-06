#include <map>

#include <Arduino.h>

#include <ArduinoJson.h>

#include <WiFi.h>
#include <ESPAsyncWebServer.h>

#include <DHTesp.h>

#include <SoftwareSerial.h>
#include <MHZ.h>

#include <MQUnifiedsensor.h>

#ifndef SSID_ENV_VAR
#define SSID_ENV_VAR ""
#endif

#ifndef PASSPHRASE_ENV_VAR
#define PASSPHRASE_ENV_VAR ""
#endif

constexpr int SENSORS_UPDATE_INTERVAL_MS = 30 * 1000;

constexpr int DHT_PIN = 14;
DHTesp dhtSensor;
TempAndHumidity tempAndHumidity = {0, 0};

constexpr int MHZ_PIN = 27;
MHZ mhz(MHZ_PIN, MHZ14A);
bool isCo2SensorReady = false;
bool isCo2SensorPreHeating = true;
int co2Ppm = 0;

constexpr int MQ7_PIN = 34;
MQUnifiedsensor mq7("ESP-32", 3.3, 12, MQ7_PIN, "MQ-7");
float coPpm = 0;

AsyncWebServer server(80);

void sensorsHandler(AsyncWebServerRequest *request);

// https://www.kane.co.uk/knowledge-centre/what-are-safe-levels-of-co-and-co2-in-rooms
const std::map<int, std::string> co2RoomLevels = {
    {400, "Normal background concentration in outdoor ambient air"},
    {1000, "Concentrations typical of occupied indoor spaces with good air exchange"},
    {2000, "Complaints of drowsiness and poor air"},
    {5000, "Headaches, sleepiness and stagnant, stale, stuffy air. Poor concentration, loss of attention, increased heart rate and slight nausea may also be present"},
    {40000, "Exposure may lead to serious oxygen deprivation resulting in permanent brain damage, coma, even death"}};
const std::map<int, std::string> coRoomLevels = {
    {9, "CO Max prolonged exposure (ASHRAE standard)"},
    {35, "CO Max exposure for 8 hour work day (OSHA)"},
    {800, "CO Death within 2 to 3 hours"},
    {12000, "CO Death within 1 to 3 minutes"}};

void setup()
{
  Serial.begin(115200);
  Serial.println("setup running");

  dhtSensor.setup(DHT_PIN, DHTesp::DHT11);

  pinMode(MHZ_PIN, INPUT);
  mhz.setDebug(true);

  mq7.setRegressionMethod(1);
  mq7.setA(99.042);
  mq7.setB(-1.518);
  mq7.init();

  float calcR0 = 0;
  const int calibrateCount = 10;
  for (int i = 1; i <= calibrateCount; i++)
  {
    mq7.update();
    calcR0 += mq7.calibrate(27.5);
  }
  mq7.setR0(calcR0 / calibrateCount);
  mq7.serialDebug(true);

  WiFi.mode(WIFI_STA);
  WiFi.begin(SSID_ENV_VAR, PASSPHRASE_ENV_VAR);
  Serial.printf("connection to %s\n", SSID_ENV_VAR);
  if (WiFi.waitForConnectResult() == WL_CONNECTED)
  {
    Serial.printf("connection successful. ip address=%s\n", WiFi.localIP().toString().c_str());
    server.on("/", HTTP_GET, sensorsHandler);
    server.onNotFound([](AsyncWebServerRequest *request)
                      { request->send(404, "text/plain", "Not found"); });
    server.begin();
  }
  else
  {
    Serial.printf("error connection to %s\n", SSID_ENV_VAR);
  }

  Serial.println("setup done");
}

void loop()
{
  tempAndHumidity = dhtSensor.getTempAndHumidity();

  isCo2SensorReady = mhz.isReady();
  isCo2SensorPreHeating = mhz.isPreHeating();
  co2Ppm = mhz.readCO2PWM();

  mq7.update();
  coPpm = mq7.readSensor();

  Serial.printf("temperature=%f,humidity=%f,co2Ppm=%d,isCo2SensorReady=%d,isCo2SensorPreHeating=%d,coPpm=%f\n",
                tempAndHumidity.temperature,
                tempAndHumidity.humidity,
                co2Ppm,
                isCo2SensorReady,
                isCo2SensorPreHeating,
                coPpm);

  delay(SENSORS_UPDATE_INTERVAL_MS);
}

void sensorsHandler(AsyncWebServerRequest *request)
{
  auto response = request->beginResponseStream("application/json");
  StaticJsonDocument<512> jsonDoc;
  auto root = jsonDoc.to<JsonObject>();
  root["temperature_c"] = tempAndHumidity.temperature;
  root["humidity_prc"] = tempAndHumidity.humidity;
  root["is_co2_sensor_ready"] = isCo2SensorReady;
  root["is_co2_sensor_pre_heating"] = isCo2SensorPreHeating;

  root["co2_ppm"] = co2Ppm;
  auto it = co2RoomLevels.lower_bound(co2Ppm);
  if (it != std::end(co2RoomLevels))
  {
    root["co2_meaning"] = it->second;
  }
  
  root["co_ppm"] = coPpm;
  it = coRoomLevels.lower_bound(std::round(coPpm));
  if (it != std::end(coRoomLevels))
  {
    root["co_meaning"] = it->second;
  }

  serializeJson(root, *response);
  request->send(response);
}
