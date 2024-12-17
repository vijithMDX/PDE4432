#include <WiFi.h>
#include <Update.h>

#include <HTTPClient.h>
#include <Arduino_JSON.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <TimeLib.h>
#include <ArduinoIoTCloud.h>
#include <Arduino_ConnectionHandler.h>

const char DEVICE_LOGIN_NAME[] = "ee36cf3a-c768-4bcd-9fd3-a171c8f35a19";
const char DEVICE_KEY[] = "#30i#?KMJ1i8XO6vNeWGrPh9w";
LiquidCrystal_I2C lcd(0x27, 16, 2);

const char* ssid = "Happiness connected";
const char* password = "vijith123";

// const char* ssid = "Viju 2.4G";
// const char* password = "Happiness";
String openWeatherMapApiKey = "298583cc84f68515e30c3cf37b7ef74b";
String city = "Dubai";
String countryCode = "AE";
String currentTime ;
String weatherNow ;
double tempCelsius;
float moisture;
int toggleRefillPumpState=0;

#define ReFillPump 4
#define Pump_pin 15
#define Water_level_high_touch T2 //pin 2
#define water_level_overflowLimit_touch T4 //pin 13
#define Water_level_low_touch T6 //pin 14
const int sensor_pin = 36;

unsigned long previousMillis = 0; // to store previous time
//unsigned long previousMillis_waterlevel = 0; // to store previous time
const long interval = 5000;

CloudTemperatureSensor moisture_IOT;
CloudSwitch refillPumpIOT;
WiFiConnectionHandler ArduinoIoTPreferredConnection(ssid, password);
// unsigned long previousMillis = 0;
// unsigned long lcdLastcheck=0;
// const long interval = 5000;  // 5 seconds


//threaded task 
void taskLCD(void *parameter);
void taskPumpControl(void *parameter);
void taskMainLoop(void *parameter);
void refillPumpChange();









void setup() {
  pinMode(Pump_pin, OUTPUT);
  pinMode(ReFillPump, OUTPUT);
  
  Serial.begin(115200);
  WiFi.begin(ssid, password);

  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(3, 0);
  lcd.print("Connecting");

  // Wait until WiFi is connected
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    lcd.print(".");
  }

  initProperties();
  ArduinoCloud.begin(ArduinoIoTPreferredConnection);
  setDebugMessageLevel(2);
  ArduinoCloud.printDebugInfo();

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Connected");
  lcd.setCursor(0, 1);
  lcd.print(WiFi.localIP());

xTaskCreate(
  taskLCD, //lcd displaying function 
  "LCD TASK", //task name
  8192, //stack size (in words)
  NULL, //parameter
  1, // task priority (lower the number lower the priority )
  NULL //task handle
);

xTaskCreate(
  taskPumpControl, //water monitoring and pump control task ,higher priority
  "Pump Control",
  8192,
  NULL,
  2, // higher the number higher the priority ,here pump control is having higher priority
  NULL

);

 xTaskCreate(
        taskMainLoop,   // Task function
        "Main Loop",    // Task name
        8192,           // Stack size
        NULL,           // Parameters
        2,              // Priority (can be adjusted)
        NULL            // Task handle
    );


}

// high priority task
void taskMainLoop(void *parameter) {
    for (;;) {
        ArduinoCloud.update();
         handleWeatherData();
         //readMoisture();
         refillPumpIOT=toggleRefillPumpState;

  
        vTaskDelay(100 / portTICK_PERIOD_MS);  // Avoid busy looping
    }
}

//task to display LCD , async operation.- Low priority
void taskLCD(void *parameter) {
    int displayState = 0; // State to alternate between displays

    while (true) {
        lcd.clear();
        switch (displayState) {
            case 0: // Display soil moisture
                lcd.setCursor(0, 0);
            lcd.print("Soil moisture");
            char moisture_lcd[16]; // Buffer to hold the formatted string
            sprintf(moisture_lcd, "Moisture %.2f%%", moisture);  // Display moisture value
            // Serial.print("Humidity in lcd:");
            // Serial.println(moisture_IOT);
            lcd.setCursor(0, 1);
            // Serial.print("moisture :");
            // Serial.println(moisture_lcd);
            lcd.print(moisture_lcd);
                break;
            case 1: // Display current time
                lcd.setCursor(0,0);
                lcd.print("TIME NOW");
                lcd.setCursor(0,1);
                lcd.print(currentTime);
                break;
            case 2: // Display weather info
                lcd.setCursor(0,0);
                lcd.print("Weather now:");
                lcd.setCursor(0,1);
                lcd.print(weatherNow); //  weather info
                break;

             case 3: // Display weather info
                lcd.setCursor(0,0);
                lcd.print("Temperature in Celcius");
                lcd.setCursor(0,1);
                lcd.print(tempCelsius); //  weather info
                break;   
        }

        displayState = (displayState + 1) % 4; // Cycle between states
        vTaskDelay(500 / portTICK_PERIOD_MS);  // Delay for 500ms
    }
}

//water pump control task -high priority 
void taskPumpControl(void *parameter) {
    while (true) {

      checkWaterLevel();
        readMoisture();

        

        vTaskDelay(500 / portTICK_PERIOD_MS); // Check every 500ms
    }
}


void loop() {
//void loop has no action in RTOS tasks




 }

void checkWaterLevel() {
  int water_level_low = touchRead(Water_level_low_touch);
  int water_level_high = touchRead(Water_level_high_touch);
  int water_level_overflowLimit = touchRead(water_level_overflowLimit_touch);
  unsigned long currentMillis = millis();
 
 // save current time as prev
  // Serial.print("Water level value:");
  // Serial.println(water_level_low);
  if (water_level_low > 22 && water_level_high > 22 && water_level_overflowLimit >25) 
  {
    Serial.println("Water level low");
    Serial.println(water_level_low);
    Serial.println("Water level low_high");
     Serial.println(water_level_high);
     Serial.println("Water level low_high_over");
      Serial.println(water_level_overflowLimit);
    previousMillis= currentMillis;
    toggleRefillPumpState=1;
    //refillPump=1;
    digitalWrite(ReFillPump, HIGH);
  }

  // int water_level_med = touchRead(Water_level_mid_touch);
  // if (water_level_med < 20) {
  //   Serial.println("Water level med");
  // }
 else if (water_level_high < 20 ) {
    Serial.println("Water level high");
    Serial.println(water_level_high);
        toggleRefillPumpState=0;

    digitalWrite(ReFillPump, LOW);
  }
  //int water_level_overflowLimit = touchRead(water_level_overflowLimit_touch);
  else if ( currentMillis - previousMillis>= interval || water_level_overflowLimit < 20) {
    Serial.println("Water level over flow");
    Serial.println(water_level_overflowLimit);
        toggleRefillPumpState=0;

    digitalWrite(ReFillPump, LOW);
    previousMillis=millis();
  }
}



//this handle pump on from Arduino cloud
void refillPumpChange() {

   int water_level_low = touchRead(Water_level_low_touch);
  int water_level_high = touchRead(Water_level_high_touch);
  int water_level_overflowLimit = touchRead(water_level_overflowLimit_touch);
  unsigned long currentMillis = millis();
  if (refillPumpIOT == 1)
  {
     Serial.println("Water level low");
    previousMillis = currentMillis;
    toggleRefillPumpState=1;
    //refillPump=1;
    digitalWrite(ReFillPump, HIGH);
  }
  else if(refillPumpIOT != 1 || currentMillis - previousMillis >= interval || water_level_high < 12 )
  {
     Serial.println("Water level high");
    Serial.println(water_level_high);
        toggleRefillPumpState=0;

    digitalWrite(ReFillPump, LOW);
    previousMillis=millis();
  }
}



//fetching weather data from openweathermap.com
void handleWeatherData() {
  if (WiFi.status() == WL_CONNECTED) {
    String serverPath = "http://api.openweathermap.org/data/2.5/weather?q=" + city + "," + countryCode + "&APPID=" + openWeatherMapApiKey;
    String jsonBuffer = httpGETRequest(serverPath.c_str());

    JSONVar myObject = JSON.parse(jsonBuffer);
    if (JSON.typeof(myObject) == "undefined") {
      Serial.println("Parsing input failed!");
      return;
    }

    double tempKelvin = (double)myObject["main"]["temp"];
    tempCelsius = tempKelvin - 273.15;
    weatherNow = (String)myObject["weather"][0]["description"];
    
    unsigned long currentUTC = (unsigned long)myObject["dt"];
    int zoneTime = (int)myObject["timezone"];
    currentTime = convertToReadableTime(currentUTC, zoneTime);

    // Serial.println("Weather: " + weatherNow);
    // Serial.println("Temperature (Celsius): " + String(tempCelsius));
    // Serial.println("Time: " + currentTime);
  } else {
    Serial.println("WiFi Disconnected");
  }
}

//reading moistrue from soil moisture sensor and turn on plant watering pump
void readMoisture() {
  int sensor_analog = analogRead(sensor_pin);
  moisture = (100 - ((sensor_analog / 4095.00) * 100));
  moisture_IOT = moisture;
  // Serial.print("moisture:");
  // Serial.println(moisture);
  // Serial.println(moisture_IOT);

  if (moisture < 18.00) {
    digitalWrite(Pump_pin, HIGH);
    Serial.println("Pump High");
    //digitalWrite(LED_BUILTIN, HIGH);
  } else if (weatherNow=="rain" || weatherNow=="thunderstorm"|| weatherNow=="shower rain"|| weatherNow=="snow"  &&  moisture >= 25.00 ) {
    digitalWrite(Pump_pin, LOW);
    Serial.println("Pump Low");
    //digitalWrite(LED_BUILTIN, LOW);
  }
  
  
  else if  (moisture > 35.00) {
    digitalWrite(Pump_pin, LOW);
    Serial.println("Pump Low");
    //digitalWrite(LED_BUILTIN, LOW);
  }
}

//checking network connection

String httpGETRequest(const char* serverName) {
  WiFiClient client;
  HTTPClient http;
  http.begin(client, serverName);
  int httpResponseCode = http.GET();
  String payload = "{}"; 

  if (httpResponseCode > 0) {
    // Serial.print("HTTP Response code: ");
    // Serial.println(httpResponseCode);
    payload = http.getString();
  } else {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }

  http.end();
  return payload;
}

//here it will convert json formated data from openweathermap api to string 
String convertToReadableTime(unsigned long utc, int timezoneOffset) {
  unsigned long localTime = utc + timezoneOffset;
  setTime(localTime);
  
  int now_hour = hour();
  int now_month=month();
  int now_minute = minute();
  int now_second = second();
  String am_pm = (now_hour >= 12) ? "PM" : "AM";
  int hour12 = now_hour % 12;
  if (hour12 == 0) hour12 = 12; // Adjust 0 to 12 for 12-hour format

  String month_text[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
  char timeBuffer[20];
  snprintf(timeBuffer, sizeof(timeBuffer), "%02d:%02d%s %02d-%02s", hour12, now_minute, am_pm.c_str(), day(), month_text[now_month - 1].c_str());
  
  return String(timeBuffer); // Return as a String
}

//ardunio cloud status update
void initProperties() {
  ArduinoCloud.setBoardId(DEVICE_LOGIN_NAME);
  ArduinoCloud.setSecretDeviceKey(DEVICE_KEY);
   ArduinoCloud.addProperty(refillPumpIOT, READWRITE, ON_CHANGE, refillPumpChange);
  ArduinoCloud.addProperty(moisture_IOT, READ, 8 * SECONDS, NULL); // Update humidity every 8 seconds
}
