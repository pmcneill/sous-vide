#include <OneWire.h>
#include <DallasTemperature.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266mDNS.h>
#include <WiFiServer.h>
#include <WiFiUdp.h>
#include <WiFiClient.h>

#define RELAY_PIN 12
#define LED_PIN 13
#define TEMP_PIN 14

#define RELAY_OFF 0
#define RELAY_ON 1

WiFiServer ws_server(80);
int goal_temp = 135;
int temp = 136;

OneWire oneWire(TEMP_PIN);
DallasTemperature sensors(&oneWire);

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(TEMP_PIN, INPUT_PULLUP);

  WiFi.mode(WIFI_AP);
  WiFi.softAP("Mmmmm Tasty");

  ws_server.begin();
  
  MDNS.begin("tasty");
  MDNS.addService("http", "tcp", 80);

  sensors.begin();
}

int readTemp() {
  sensors.requestTemperatures();
  return floor(sensors.getTempFByIndex(0));
}

int wsHandleRequest() {
  WiFiClient c = ws_server.available();

  if ( ! c ) return 0;

  while ( c.connected() && ! c.available() ) {
    yield();
  }

  String req = c.readStringUntil('\r');
  
  int addr_start = req.indexOf(' ');
  int addr_end = req.indexOf(' ', addr_start + 1);
  if (addr_start == -1 || addr_end == -1) {
    Serial.print("Invalid request: ");
    Serial.println(req);
    return 0;
  }

  Serial.print("Web request received: ");
  Serial.println(req);

  req = req.substring(addr_start + 1);
  Serial.print("Request: ");
  Serial.println(req);
  c.flush();

  String s;
  if ( (addr_start = req.indexOf('=')) >= 0 ) {
    s = "HTTP/1.1 302 Found\r\nLocation: /\r\n\r\n";
    goal_temp = req.substring(addr_start + 1, req.indexOf(' ', addr_start + 2)).toInt();
    Serial.print("Goal temp updated to ");
    Serial.println(goal_temp);
  } else {
    s = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE HTML>\r\n<html><body>";
    
    s += "<div>Current temp: ";
    s += temp;
    s += "</div><div>Goal temp: <form method=\"get\"><input name=\"t\" size=\"4\" value=\"";
    s += goal_temp;
    s += "\"> <input type=\"submit\" value=\"Change\"></form></div></body></html>\r\n\r\n";
  }

  c.print(s);

  return 0;
}

int i = 0;

void loop() {
  wsHandleRequest();

  if ( i % 20 == 0 ) {
    temp = readTemp();
    if ( temp < 0 ) return;
    
    Serial.println(temp);

    // Ignore temp == goal_temp to avoid switching too rapidly
    if ( temp > goal_temp ) {
      digitalWrite(RELAY_PIN, RELAY_OFF);
      digitalWrite(LED_PIN, 1);
    } else if ( temp < goal_temp ) {
      digitalWrite(RELAY_PIN, RELAY_ON);
      digitalWrite(LED_PIN, 0);
    }
  }

  i++;
  delay(50);
}
