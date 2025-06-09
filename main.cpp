#include <Arduino.h>
#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include <addons/TokenHelper.h>
#include <addons/RTDBHelper.h>

#define WIFI_SSID "wahren"
#define WIFI_PASSWORD "123456789"
#define API_Key "AIzaSyCQ46adOkMKdS4BiawGur52j99-GueX_L4"

#define url "https://espman-5b234-default-rtdb.europe-west1.firebasedatabase.app/"

#define SENSOR_A 18  
#define SENSOR_B 19 


//initialize the count
int people_count = 0;

//initialize the logic to detect entry
bool sensorA_prev = 0;
bool sensorB_prev = 0;


unsigned long last_trigger_time = 0;
unsigned long debounce_delay = 100;

//firebase objects
FirebaseData fbdo; //data object
FirebaseAuth auth; //authentication object
FirebaseConfig config; //configuration object
FirebaseJson entry_log; //json object

void setup() {

  Serial.begin(115200);

  pinMode(SENSOR_A, INPUT);
  pinMode(SENSOR_B, INPUT);

  // connect to the wifi

  pinMode(BUILTIN_LED, OUTPUT);





  digitalWrite(BUILTIN_LED, HIGH);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }

  Serial.println("\nConnected!");

  digitalWrite(BUILTIN_LED, LOW);

  // configure the database information

  config.api_key = API_Key;
  config.database_url = url;
  auth.user.email = "";
  auth.user.password = "";

  //sign up
  Serial.print("Signing up...");
  if(Firebase.signUp(&config, &auth, "", "")){
    Serial.println("ok");
  }else{
    Serial.printf("%s\n", config.signer.signupError.message.c_str());
  }
 

  
  // register the token status callback function
  config.token_status_callback = tokenStatusCallback;

  // Connect to Firebase
  
  
  Firebase.begin(&config, &auth);
  // Enable auto reconnect
  Firebase.reconnectWiFi(true);

  // Send data once connected
  if (Firebase.ready()) {
    Serial.println("Firebase ready");

    // Send a string to path: /test/message
    if (Firebase.RTDB.setString(&fbdo, "/test/message", "Hello from ESP32")) {
      Serial.println("String sent!");
    } else {
      Serial.println(fbdo.errorReason());
    }

    // Send a number to path: /test/value
    if (Firebase.RTDB.setInt(&fbdo, "/test/value", 42)) {
      Serial.println("Number sent!");
    } else {
      Serial.println(fbdo.errorReason());
    }
  }
}

void loop() {
  bool sensorA_state = digitalRead(SENSOR_A);
  bool sensorB_state = digitalRead(SENSOR_B);

  // Print live sensor status
  Serial.print("Sensor A: ");
  Serial.print(sensorA_state == 1 ? "DETECTED " : "CLEAR    ");
  Serial.print(" | Sensor B: ");
  Serial.print(sensorB_state == 1 ? "DETECTED " : "CLEAR    ");
  Serial.print(" | People Count: ");
  Serial.println(people_count);

  // Detect entry (A then B)
  if (sensorA_state == 1 && sensorA_prev == 0) {
    last_trigger_time = millis();
    while (millis() - last_trigger_time < 2000) {
      sensorB_state = digitalRead(SENSOR_B);
      if (sensorB_state == 1) {
        people_count++;
        Serial.println("🚶 Person ENTERED");
        Firebase.RTDB.setInt(&fbdo, "/room/people_count", people_count);


        entry_log.set("time", millis());
        entry_log.set("action", "entered");
        entry_log.set("count", people_count);
        Firebase.RTDB.pushJSON(&fbdo, "/room/logs", &entry_log);
        delay(500);  // Debounce
        break;
      }
    }
  }

  // Detect exit (B then A)
  if (sensorB_state == 1 && sensorB_prev == 0) {
    last_trigger_time = millis();
    while (millis() - last_trigger_time < 2000) {
      sensorA_state = digitalRead(SENSOR_A);
      if (sensorA_state == 1) {
        if (people_count > 0) people_count--;
        Serial.println("🏃 Person EXITED");
        Firebase.RTDB.setInt(&fbdo, "/room/people_count", people_count);

      
        entry_log.set("time", millis());
        entry_log.set("action", "exited");
        entry_log.set("count", people_count);
        Firebase.RTDB.pushJSON(&fbdo, "/room/logs", &entry_log);
        delay(500);  // Debounce
        break;
      }
    }
  }

  // Update previous sensor states
  sensorA_prev = sensorA_state;
  sensorB_prev = sensorB_state;

  delay(200);  // Reduce spam in Serial Monitor
}
