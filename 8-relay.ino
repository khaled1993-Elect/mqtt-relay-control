#include <ESP8266WiFi.h>
#include <PubSubClient.h>

// === Wi-Fi Credentials ===
const char* ssid = "TP-LINK_2F649A_plus";
const char* password = "90491566";

// === MQTT Broker ===
const char* mqtt_server = "test.mosquitto.org";
const int mqtt_port = 1883;

// === Pairing Token ===
const char* token = "abc123xyz789";  // Must match HTML!

// === Topics ===
String topic_controls[8];
String topic_status = String(token) + "/status";

WiFiClient espClient;
PubSubClient client(espClient);

// === Relay Pins ===
// Adjust to your board: NodeMCU example
const int relayPins[8] = {5, 4, 14, 12, 13, 15, 16, 2}; 
// GPIO: D1, D2, D5, D6, D7, D8, D0, D4

void setup_wifi() {
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("✅ WiFi connected");
}

void callback(char* topic, byte* payload, unsigned int length) {
  String msg;
  for (unsigned int i = 0; i < length; i++) {
    msg += (char)payload[i];
  }

  Serial.print("Message arrived on [");
  Serial.print(topic);
  Serial.print("]: ");
  Serial.println(msg);

  for (int i = 0; i < 8; i++) {
    if (String(topic) == topic_controls[i]) {
      if (msg == "ON") {
        digitalWrite(relayPins[i], HIGH);
        Serial.printf("Relay %d ON\n", i+1);
      } else if (msg == "OFF") {
        digitalWrite(relayPins[i], LOW);
        Serial.printf("Relay %d OFF\n", i+1);
      }
    }
  }
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");

      for (int i = 0; i < 8; i++) {
        client.subscribe(topic_controls[i].c_str());
        Serial.print("Subscribed to: ");
        Serial.println(topic_controls[i]);
      }

      client.publish(topic_status.c_str(), "HELLO", true);
      Serial.print("Published retained HELLO on: ");
      Serial.println(topic_status);

    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 sec");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);   // ✅ Serial FIRST!
  setup_wifi();           // ✅ Then connect Wi-Fi
  randomSeed(micros());

  // Build topics
  for (int i = 0; i < 8; i++) {
    topic_controls[i] = String(token) + "/control/" + String(i+1);
  }

  // Configure relay pins
  for (int i = 0; i < 8; i++) {
    pinMode(relayPins[i], OUTPUT);
    digitalWrite(relayPins[i], LOW);
  }

  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
}
