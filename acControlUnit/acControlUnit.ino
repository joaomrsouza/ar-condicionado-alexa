#include "secrets.h"

#include <WiFiClient.h>
#include <ESP8266WiFi.h>
#include <Arduino_JSON.h>

#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>

#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <ir_Samsung.h>

/* Set these to your desired credentials. */
const char *ssid = SECRET_SSID;     // Enter your WIFI ssid
const char *password = SECRET_PASS; // Enter your WIFI password
const uint16_t irLedPin = 4;        // ESP8266 GPIO pin to use. Recommended: 4 (D2).

IRSamsungAc ac(irLedPin); // Set the GPIO used for sending messages.

void setup()
{
  Serial.begin(115200);

  // Connectando no Wifi
  Serial.println();
  Serial.print("Configuring access point...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  // Inicializando lib de controle
  ac.begin();

  ac.off();
  ac.setFan(kSamsungAcFanAuto);
  ac.setMode(kSamsungAcCool);
  ac.setEcono(true);
  ac.setTemp(26);
  ac.setSwing(true);
}

JSONVar getWebState()
{
  Serial.println("Getting web state...");

  std::unique_ptr<BearSSL::WiFiClientSecure> client(new BearSSL::WiFiClientSecure);

  client->setInsecure();

  HTTPClient https;
  JSONVar result;

  bool conection = https.begin(*client, (String)SECRET_API_ENDPOINT + "/esp");

  if (!conection)
  {
    Serial.println("Conection Error while getting web state.");
    result["success"] = false;
    return result;
  }

  https.addHeader("authorization", SECRET_AUTH_TOKEN);

  int httpCode = https.GET();

  Serial.print("HTTP Response code: ");
  Serial.println(httpCode);

  if (httpCode <= 0 || httpCode != HTTP_CODE_OK)
  {
    Serial.println("Error while getting web state.");
    result["success"] = false;
    return result;
  }

  String payload = https.getString();
  Serial.println(payload);

  https.end();

  Serial.println("Web state get successfully.");

  result["success"] = true;
  result["dataStr"] = payload;

  return result;
}

JSONVar getState()
{
  Serial.println("Getting device state...");
  JSONVar state;
  state["powerState"] = (bool)ac.getPower();
  return state;
}

bool stateChanged(JSONVar oldState, JSONVar newState)
{
  Serial.println("Checking for state changes...");
  newState["_id"] = undefined;
  newState["__v"] = undefined;
  newState["key"] = undefined;

  Serial.print(JSON.stringify(oldState));
  Serial.print(" != ");
  Serial.println(JSON.stringify(newState));

  if (JSON.stringify(oldState) != JSON.stringify(newState))
  {
    Serial.println("State changed!");
    return true;
  }

  Serial.println("State didn't change. No need for processing...");
  return false;
}

void processState(JSONVar newState)
{
  Serial.println("Processing new state:");

  Serial.println("Power State");
  if ((bool)newState["powerState"] == true)
  {
    ac.on();
    Serial.println("- Turn On");
  }
  else if ((bool)newState["powerState"] == false)
  {
    ac.off();
    Serial.println("- Turn Off");
  }

  // Enviar state pro ar-condicionado
  Serial.println("Sending state to device...");
  ac.send();
}

void loop()
{
  Serial.println("========================================");
  JSONVar webState = getWebState();

  if ((bool)webState["success"] == false)
  {
    Serial.println("Error while getting web state. Trying again in 5 seconds...");
    delay(5000);
    return;
  }

  JSONVar newState = JSON.parse((String)webState["dataStr"]);
  JSONVar oldState = getState();
  if (stateChanged(oldState, newState))
  {
    processState(newState);
  }
  Serial.println("========================================");
}

// server.on("/LED_BUILTIN_on", []() {
//   digitalWrite(LED_BUILTIN, 1);
//   Serial.println("on");
//   handleRoot();
// });
// server.on("/LED_BUILTIN_off", []() {
//   digitalWrite(LED_BUILTIN, 0);
//   Serial.println("off");
//   handleRoot();
// });