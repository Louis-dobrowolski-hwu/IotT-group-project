#include <ESP8266WiFi.h>
#include <Adafruit_NeoPixel.h>
#include <DHT.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <espnow.h>
// --------------------------------------------------------------------------------------------
//        UPDATE CONFIGURATION TO MATCH YOUR ENVIRONMENT
// --------------------------------------------------------------------------------------------

// Watson IoT connection details
#define MQTT_HOST "broker.hivemq.com"
#define MQTT_PORT 1883
#define MQTT_DEVICEID "d:hwu:esp8266:mqttv312111"
#define MQTT_USER "" // no need for authentication, for now
#define MQTT_TOKEN "" // no need for authentication, for now
#define MQTT_TOPIC "mqttv312/evt/status/fmt/json/group_project"
//#define MQTT_TOPIC_DISPLAY "mqttv312/cmd/display/fmt/Intervalle"

// Add GPIO pins used to connect devices
#define BUT_PIN 5 // GPIO pin the data line of RGB button is connected to


volatile boolean haveReading = false;
int flag=0;
int flag2=0;
// Add WiFi connection information
char ssid[] = "iPhone de Mona";     //  your network SSID (name)
char pass[] = "123456789";  // your network password

int heartBeat;

// --------------------------------------------------------------------------------------------
//        SHOULD NOT NEED TO CHANGE ANYTHING BELOW THIS LINE
// --------------------------------------------------------------------------------------------
//ESP NOW 

typedef struct struct_message {
    int id;
    int b1;
    int b2;
    
} struct_message;

// Create a struct_message called myData
struct_message myData;

// Callback function that will be executed when data is received
void OnDataRecv(uint8_t * mac, uint8_t *incomingData, uint8_t len) {
  memcpy(&myData, incomingData, sizeof(myData));
  Serial.print("Bytes received: ");
  Serial.println(len);
  Serial.print("id: ");
  Serial.println(myData.id);
  Serial.println();
  Serial.print("b1: ");
  Serial.println(myData.b1);
  Serial.println();
  Serial.print("b2: ");
  Serial.println(myData.b2);
  Serial.println();
  if(myData.id==1){
    flag=1;
  }
  else if(myData.id==2){
    flag2=1;
  }
  haveReading = true;  

}

// MQTT objects
void callback(char* topic, byte* payload, unsigned int length);
WiFiClient wifiClient;
PubSubClient mqtt(MQTT_HOST, MQTT_PORT, callback, wifiClient);

// variables to hold data
StaticJsonDocument<100> jsonDoc;
JsonObject payload = jsonDoc.to<JsonObject>();
JsonObject status = payload.createNestedObject("d");
static char msg[50];

StaticJsonDocument<200> doc;


void callback(char* topic, byte* payload, unsigned int length) {
  // handle message arrived
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] : ");
  payload[length] = 0; // ensure valid content is zero terminated so can treat as c-string
  Serial.println((char *)payload);
  DeserializationError error = deserializeJson(doc, payload);

}

void setup() {
  Serial.begin(115200);
  
  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);

  // Init ESP-NOW
  if (esp_now_init() != 0) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  
  // Once ESPNow is successfully Init, we will register for recv CB to
  // get recv packer info
  esp_now_set_self_role(ESP_NOW_ROLE_SLAVE);
  esp_now_register_recv_cb(OnDataRecv);

  Serial.println("Setup done");
}

void loop() {
    if (millis()-heartBeat > 30000) {
    Serial.println("Waiting for ESP-NOW messages...");
    heartBeat = millis();
  }
  if (haveReading) {
    haveReading = false;
    wifiConnect();
    reconnectMQTT();
    sendToBroker();
    mqtt.disconnect();
    delay(200);
    ESP.restart(); // <----- Reboots to re-enable ESP-NOW
  }


  // Pause - but keep polling MQTT for incoming messages
}

void sendToBroker() {
// Send data to Watson IoT Platform
  if(flag==1){
    status["id"] = myData.id;
    status["button1"] = 1;
    status["button2"] = 0;
    flag=0;
  }
  else if(flag2==1){
    status["id"] = myData.id;
    status["button1"] = 0;
    status["button2"] = 1;
    flag2=0;
  }
  else if(flag==1 & flag2==1){
    status["id"] = 0;
    status["button1"] = 1;
    status["button2"] = 1;
    flag=0;
    flag2=0;
  }
  else{
    status["id"] = 0;
    status["button1"] = 0;
    status["button2"] = 0;
  }
  serializeJson(jsonDoc, msg, 50);
  Serial.println(msg);
  if (!mqtt.publish(MQTT_TOPIC, msg)) {
    Serial.println("MQTT Publish failed");
  }
}  

void wifiConnect() {
  WiFi.mode(WIFI_STA);
  Serial.println();
  Serial.print("Connecting to "); Serial.print(ssid);
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
     delay(250);
     Serial.print(".");
  }  
  Serial.print("\nWiFi connected, IP address: "); Serial.println(WiFi.localIP());
}

void reconnectMQTT() {
  if (mqtt.connect(MQTT_DEVICEID, MQTT_USER, MQTT_TOKEN)) {
    Serial.println("MQTT Connected");
    //mqtt.subscribe(MQTT_TOPIC_DISPLAY);

  } else {
    Serial.println("MQTT Failed to connect!");
    ESP.reset();
  }
  while (!mqtt.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (mqtt.connect(MQTT_DEVICEID, MQTT_USER, MQTT_TOKEN)) {
      Serial.println("MQTT Connected");
      //mqtt.subscribe(MQTT_TOPIC_DISPLAY);
      mqtt.loop();
    } else {
      Serial.println("MQTT Failed to connect!");
      delay(5000);
    }
  }
}

