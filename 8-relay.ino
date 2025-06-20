#include <ESP8266WiFi.h>
#include <PubSubClient.h>

// === Wi-Fi ===
const char* ssid = "********";
const char* password = "********";

// === MQTT ===
const char* mqtt_server = "test.mosquitto.org";
const int mqtt_port = 1883;
const char* token = "abc123xyz789";

// === Topics ===
String topic_controls[8];
String topic_status[8];
String topic_hello = String(token) + "/status/hello";

WiFiClient espClient;
PubSubClient client(espClient);

// === Relay pins (NodeMCU: D1,D2,D5,D6,D7,D8,D0,D4) ===
const int relayPins[8] = {5, 4, 14, 12, 13, 15, 16, 2};

void setup_wifi() {
  Serial.println();
  Serial.printf("Connecting to %s\n", ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n✅ WiFi connected");
}

void publishRelayStatus(int i) {
  String msg = (digitalRead(relayPins[i]) == LOW) ? "ON" : "OFF";
  client.publish(topic_status[i].c_str(), msg.c_str(), true);
  Serial.printf("Published Relay %d status: %s\n", i+1, msg.c_str());
}

void callback(char* topic, byte* payload, unsigned int length) {
  String msg;
  for (unsigned int i = 0; i < length; i++) {
    msg += (char)payload[i];
  }
  Serial.printf("Message [%s]: %s\n", topic, msg.c_str());

  for (int i = 0; i < 8; i++) {
    if (String(topic) == topic_controls[i]) {
      if (msg == "ON") {
        digitalWrite(relayPins[i], LOW);  // ACTIVE LOW = ON
      } else {
        digitalWrite(relayPins[i], HIGH); // OFF
      }
      publishRelayStatus(i);
    }
  }
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    String clientId = "ESP8266Client-" + String(random(0xffff), HEX);
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      for (int i = 0; i < 8; i++) {
        client.subscribe(topic_controls[i].c_str());
        Serial.printf("Subscribed to %s\n", topic_controls[i].c_str());
      }
      client.publish(topic_hello.c_str(), "HELLO", true);
      for (int i = 0; i < 8; i++) {
        publishRelayStatus(i);
      }
    } else {
      Serial.printf("failed, rc=%d, try again in 5 sec\n", client.state());
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  setup_wifi();
  randomSeed(micros());

  for (int i = 0; i < 8; i++) {
    topic_controls[i] = String(token) + "/control/" + String(i+1);
    topic_status[i] = String(token) + "/status/" + String(i+1);
    pinMode(relayPins[i], OUTPUT);
    digitalWrite(relayPins[i], HIGH); // ACTIVE LOW, so HIGH = OFF
  }

  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}

void loop() {
  if (!client.connected()) reconnect();
  client.loop();
}
