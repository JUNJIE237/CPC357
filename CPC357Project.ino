#include <Wire.h>
#include "DHT.h"
#include "MAX30105.h"
#include "heartRate.h"
#include <SPI.h>
#include <ADXL362.h>
#include <WebServer.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <PubSubClient.h>
#define DHTTYPE DHT11 // DHT11
MAX30105 particleSensor; // Define  Object for Heartbeat Sensor 
ADXL362 adxl; // Create an ADXL362 object
/////////Define for accelerometer///////////
#define CS_PIN 5 // Chip Select pin for ADXL362
// Thresholds for fall detection (adjust based on testing)
#define IMPACT_THRESHOLD 2500 // Value in milli-g
#define INACTIVITY_THRESHOLD 1300 // Value in milli-g
#define INACTIVITY_TIME 2000 // Time in milliseconds
/////////////////////////////////////////////////

//////////////////////DHT11 and Light sensor Variable ////////////////////////
const int dht11Pin = 15; // Digital pin connected to the DHT11 sensor
const int relayPin = 32; // Relay pin (active high)
const int lightSensorPin = 34; // Analog pin connected to the light sensor
const int ledPinY = 0; // Yellow LED

DHT dht(dht11Pin, DHTTYPE);

unsigned long lastMsgTime = 0;
///////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////Heartbeat Sensor Variable///////////////////////
const byte RATE_SIZE = 4; // Increase this for more averaging. 4 is good.
byte rates[RATE_SIZE]; // Array of heart rates
byte rateSpot = 0;
long lastBeat = 0; // Time at which the last beat occurred
int count_sense = 0;

float beatsPerMinute;
int beatAvg;

long lastOutputTime = 0; // Time of the last output
const long OUTPUT_INTERVAL = 3000; // 3 seconds interval in milliseconds

long lastWarningTime = 0; // Time of the last warning
const long WARNING_INTERVAL = 5000; // 5 seconds interval in milliseconds

const int ledPinR = 2;
///////////////////////////////////////////////
////////////////////////////Accelerometer Variable//////////////////////////
// Variables to track fall detection
volatile bool isFalling = false;
unsigned long inactivityStart = 0;


//////////////////////////////////////////////////////////////////////////
////////////////////////////////////WIFI THINGS//////////////////////////////////////////////
// Wi-Fi credentials
const char* ssid = "CSLOUNGE666";
const char* password = "ONLY4.0CANPASS";
// WebSocket server
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

// Function to send status update to clients
void notifyClients() {
    String status="fall detected";
    String message = "{\"status\": \"" + status + "\"}";
    ws.textAll(message); // Broadcast to all connected clients
}

// Handle WebSocket events
void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
               void *arg, uint8_t *data, size_t len) {
  if (type == WS_EVT_CONNECT) {
    Serial.println("WebSocket client connected");
  } else if (type == WS_EVT_DISCONNECT) {
    Serial.println("WebSocket client disconnected");
  }
}
//////////////////////////////////////////////////////////////////////////////////////////////
////////MQTT Server in GCP///////


const char* MQTT_SERVER = "34.60.18.131"; // Your VM instance public IP address
const char* MQTT_TOPIC_temp = "temperature"; // MQTT topic for subscription
const char* MQTT_TOPIC_heartbeat = "heartbeat"; // MQTT topic for subscription
const int MQTT_PORT = 1883; // Non-TLS communication port

char buffer[128] = ""; // Text buffer
char buffer2[128] = ""; // Text buffer

WiFiClient espClient;
PubSubClient client(espClient);

///////////////////////////////
/////////Accelerometer Function/////////////
void sendAlert() {
  // Example alert mechanism (e.g., print message, send via Wi-Fi/Bluetooth)
  Serial.println("ALERT: Fall detected! Notify caregiver.");
  notifyClients();
}

void resetDetection() {
  isFalling = false;
  inactivityStart = 0;
}
/////////////////////////////////////////////////////////////////
///////////// Reconnect MQTT//////////
void reconnect() {
  while (!client.connected())
  {
    Serial.println("Attempting MQTT connection...");

    if(client.connect("ESP32Client")) {
      Serial.println("Connected to MQTT server");
    }
    else {
      Serial.print("Failed, rc=");
      Serial.print(client.state());
      Serial.println(" Retrying in 5 seconds...");
      delay(5000);
      }
  }
}
/////////////////////////////////////////////
// void setup_wifi() {

//   delay(10);
//   // We start by connecting to a WiFi network
//   Serial.println();
//   Serial.print("Connecting to ");
//   Serial.println(WIFI_SSID);

//   WiFi.mode(WIFI_STA);
//   WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

//   while (WiFi.status() != WL_CONNECTED) {
//     delay(500);
//     Serial.print(".");
//   }
  
//   Serial.println("");
//   Serial.println("WiFi connected");
//   Serial.println("IP address: ");
//   Serial.println(WiFi.localIP());
// }

void setup() {
    Serial.begin(115200); // Start serial communication
    while (!Serial) delay(10);
    //Set up wifi connection on ESP32
    //setup_wifi();   
  
    dht.begin(); // Initialize the DHT sensor

    // Set the  pin mode
    pinMode(relayPin, OUTPUT); // Set relay pin as output
    pinMode(ledPinY, OUTPUT);

    // Set the default behaviour for the components
    digitalWrite(relayPin, LOW); // Ensure relay is off at start
    digitalWrite(ledPinY, LOW);  // Ensure the Led Light is off initially

    ////////////////Configure for heart beat sensor //////////
    Wire.begin(21, 22);
    pinMode(ledPinR, OUTPUT);
    digitalWrite(ledPinR, LOW);

    // Initialize sensor
    if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) // Use default I2C port, 400kHz speed
    {
      Serial.println("MAX30105 was not found. Please check wiring/power.");
      while (1);
    }
    Serial.println("Place your index finger on the sensor with steady pressure.");

    particleSensor.setup(); // Configure sensor with default settings
    particleSensor.setPulseAmplitudeRed(0x0A); // Turn Red LED to low to indicate sensor is running
    particleSensor.setPulseAmplitudeGreen(0); // Turn off Green LED
    ////////////////////////////////////////////////////////////////////////
    //////////////////////////Accelerometer/////////// 
    SPI.begin(); // Initialize SPI
    adxl.begin(CS_PIN); // Initialize ADXL362 with CS pin
    adxl.beginMeasure(); // Start measurement mode
    Serial.println("ADXL362 Initialized");
    ////////////////////////////////////////////////
    // WIFI THINGS///////////////////////////////// 
    // Connect to Wi-Fi
    WiFi.begin(ssid, password);
    Serial.print("Connecting to Wi-Fi");
    while (WiFi.status() != WL_CONNECTED) {
      delay(1000);
      Serial.print(".");
    }
    Serial.println("\nConnected to Wi-Fi!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());

    // WebSocket setup
    ws.onEvent(onWsEvent);
    server.addHandler(&ws);
    // Start server
    server.begin();
    Serial.println("WebSocket server started");
    ///////////////////////////////////////////////
    client.setServer(MQTT_SERVER, MQTT_PORT); // Set up the MQTT client
}


void loop() {
  
  if(!client.connected()) {
    reconnect();
  }
  client.loop();

  float h = dht.readHumidity(); // Read humidity
  float t = dht.readTemperature(); // Read temperature

  /////////////////////////// DHT Sensor //////////////////////////////

  if (isnan(h) || isnan(t)) {
      Serial.println("Failed to read from the DHT sensor");
      return;
  }
  // relay control based on temperature
  if (t > 30) {
      digitalWrite(relayPin, HIGH); // Turn relay on
  } else {
      digitalWrite(relayPin, LOW); // Turn relay off
  }

  ////////////////////////////////////////////////////////////////////////////////
  //For light Intensity
  int lightIntensity = analogRead(lightSensorPin); // Read the raw analog value
  float voltage = (lightIntensity / 4095.0) * 3.3; // Convert to voltage (3.3V reference for ESP32 ADC)
  // Print the results

  
  // Example relay control based on temperature
  if ( lightIntensity < 1000) {
      digitalWrite(ledPinY, HIGH); // Turn Led On
  } else {
      digitalWrite(ledPinY, LOW); // Turn Led Off
  }

  //////////////////////Below is used for MAXX3010 ///////////////////////////////////////
  long irValue = particleSensor.getIR();

  if (irValue > 50000) {
    if (checkForBeat(irValue) == true) {
      Serial.print("We sensed a beat!");
      count_sense++;
      long delta = millis() - lastBeat;
      lastBeat = millis();

      beatsPerMinute = 60 / (delta / 1000.0);

      if (beatsPerMinute < 255 && beatsPerMinute > 20) {
        rates[rateSpot++] = (byte)beatsPerMinute; // Store this reading in the array
        rateSpot %= RATE_SIZE; // Wrap variable
        if (beatsPerMinute < 100 && beatsPerMinute > 70){
          digitalWrite(ledPinR, LOW); // Turn relay on
          //sprintf(buffer, "%.2f", beatsPerMinute);
          client.publish(MQTT_TOPIC_heartbeat, String(beatsPerMinute).c_str());
        }else{
          digitalWrite(ledPinR, HIGH);
          //sprintf(buffer, "Hearbeat: %.2f", beatsPerMinute);
          //client.publish(MQTT_TOPIC_heartbeat, buffer);
          }
        

        // Take average of readings
        beatAvg = 0;
        for (byte x = 0; x < RATE_SIZE; x++)
          beatAvg += rates[x];
        beatAvg /= RATE_SIZE;
      }
    }

  } else {
    // Print warning every 5 seconds if no finger is detected
    if (millis() - lastWarningTime >= WARNING_INTERVAL) {
      lastWarningTime = millis(); // Update the last warning time
      Serial.println("Warning: No finger detected. Please place your finger on the sensor.");
    }
  }

  ////////////////////////////////////////////////////////////////////////////////
  //Below is used for Printing Logging
        // Output values every 3 seconds
    if (millis() - lastOutputTime >= OUTPUT_INTERVAL) {
      lastOutputTime = millis(); // Update the last output time
      count_sense = 0;

      Serial.print("Temperature: ");
      Serial.print(t);
      Serial.print("°C, Humidity: ");
      Serial.print(h);
      Serial.println("%");

      //sprintf(buffer, "Temperature: %.2f degree Celsius", t);
      client.publish(MQTT_TOPIC_temp, String(t).c_str());

      if (t > 30) {
          Serial.println("Relay ON: High temperature detected.");
      } else {
          Serial.println("Relay OFF: Temperature within range.");
      }

      Serial.print("Light Intensity : ");
      Serial.print(lightIntensity);
      Serial.print(" | Voltage: ");
      Serial.print(voltage);
      Serial.println(" V");

      if ( lightIntensity < 1000) {
        Serial.println("Light ON: Low Light Intensity Detected.");
      } else {
        Serial.println("Light OFF: High Light Intensity Detected");
        }

      Serial.print("IR=");
      Serial.print(irValue);
      Serial.print(", BPM=");
      Serial.print(beatsPerMinute);
      Serial.print(", Avg BPM=");
      Serial.print(beatAvg);
      Serial.println();
    }
  //////////////////////////Accelerometer///////////////////////////
  int16_t x, y, z,temp; // Variables to store acceleration data
  adxl.readXYZTData(x, y, z,temp); // Read X, Y, Z data

  // Compute the Signal Vector Magnitude (SVM)
  int16_t svm = sqrt(x * x + y * y + z * z);
    // Check for impact
  if (svm > IMPACT_THRESHOLD) {
    isFalling = true;
    Serial.println("Impact detected! \n");
    Serial.println("Falling SVM is \n");
    Serial.println(svm);
    Serial.println(" \n");
  }
    if (isFalling && svm < INACTIVITY_THRESHOLD) {
    Serial.println("In inactivity \n");
    Serial.println("SVM is \n");
    Serial.println(svm);
    Serial.println(" \n");
    if (inactivityStart == 0) {
      inactivityStart = millis(); // Start inactivity timer
    }else {
     // Reset inactivity timer
     Serial.print("inactivityStart: \n");
     Serial.print(inactivityStart);
     Serial.print("\n isFalling: \n");
     Serial.print(isFalling);
    }
    if (millis() - inactivityStart >= INACTIVITY_TIME) {
      Serial.println("Fall detected! Sending alert...");
      sendAlert(); // Custom function to send an alert
      delay(5000);
      resetDetection();
    } else
    {
      Serial.println("Waiting 2 seconds");
    }
  } 
//////////////////////////////////////////////////////////////////////

}