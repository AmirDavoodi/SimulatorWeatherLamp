#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <Adafruit_NeoPixel.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>


#define Led_pin            D1
#define NUMPIXELS      60
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, Led_pin, NEO_GRB + NEO_KHZ800);


int TouchSensor1 = D5; //connected to Digital pin D5
int TouchSensor2 = D6; //connected to Digital pin D6
int on_off_status = -1; // -1 means it is OFF
float brightness = 255; // Initialize Brightness
float fadeAmount = 25.5;
boolean booting_up = true;

// WiFi credentials
const char* ssid = "PT";
const char* password =  "pang12345";

// api credentials
const String endpoint = "http://api.openweathermap.org/data/2.5/weather?&units=metric";
const String key = "&appid=134f461a6f03f55040a08b8935cd2f85";


// All our states in our lamp
#define S_TURNOFF 1
#define S_INITIALIZATION 2
#define S_WORKING 3


int delayval = 20;// Delay for a period of time (in milliseconds).

void off_all_leds(){
  for ( int led_num = 1; led_num <= 60; led_num++ ) {
    pixels.setPixelColor(led_num, pixels.Color(0, 0, 0));
  }
  pixels.show(); // This sends the updated pixel color to the hardware.
  delay(delayval); // Delay for a period of time (in milliseconds).
}

void bootup_effect(){
  int color_mode = 1;
  for ( int led_num = 1; led_num <= 60; led_num++ ) {
    if (led_num % 20 == 0) {
      if (color_mode == 1) {
        color_mode = 2;
      } else if (color_mode == 2) {
        color_mode = 3;
      } else if (color_mode == 3) {
        color_mode = 1;
      }
    }

    // set the color of leds
    if (color_mode == 1) {
      pixels.setPixelColor(led_num, pixels.Color(brightness, 0, 0));
    } else if (color_mode == 2) {
      pixels.setPixelColor(led_num, pixels.Color(0, brightness , 0));
    } else if (color_mode == 3) {
      pixels.setPixelColor(led_num, pixels.Color(0, 0 , brightness));
    }

    pixels.show(); // This sends the updated pixel color to the hardware.
    delay(delayval); // Delay for a period of time (in milliseconds).
  }
  Serial.println("starting color motion");
}

bool turn_off_on(int sensor1, int sensor2) {
  //Detects double button pressed
  if (sensor1 == HIGH && sensor2 == HIGH) {
    //Change of state
    on_off_status = on_off_status * (-1); // toggle the on_off_status
    while (digitalRead(TouchSensor1) == HIGH || digitalRead(TouchSensor2) == HIGH) {
      delay(10);
    }
    if (on_off_status == 1) { // lamp must switch to ON
      Serial.println("Turning ON");
      return true;
    } else if (on_off_status == -1) { // lamp must switch to OFF
      Serial.println("Turning OFF");
      return false;
    }
  }
}












//void get_weather_info(String city) {
//  HTTPClient http;
//  http.begin(endpoint+key+city); //Specify the URL
//  int httpCode = http.GET();  //Make the request
//
//  if (httpCode > 0) { //Check for the returning code
//      String json = http.getString();
//      Serial.println(json);
//      DynamicJsonBuffer jsonBuffer;
//      JsonObject& root = jsonBuffer.parseObject(json);
//      
//      if (!root.success()){
//        Serial.print("ParseObject() failed");
//        return;
//      }
//      JsonObject& weather = root["weather"][0];
//      String weatherId = weather["id"]; 
//      String cityName = root["name"]; 
//      double temperature = root["main"]["temp"]; 
//      String temp = String(lround(temperature));
//      String humidity = root["main"]["humidity"]; 
//      String weatherDesc = weather["description"];
//
//      String str_net_stat = "Not Connected";
//      display.setTextSize(1);
//      display.setTextColor(WHITE);
//      display.setCursor(0,0);
//      display.println("LOCATION:"+ loc);
//      display.println("TIME:"+timestr);
//      display.println("TEMPERATURE:"+temp);
//      display.println("HUMIDITY:"+hum);
//      display.println("DESCRIPTION:"+desc);
//      display.println("--------------------");
//      if(net_stat == true){
//        str_net_stat = "Connected";
//      }
//      display.println("Network status:"+net_stat); //connected or not connected
//      display.println("API status:"+api_stat); //connected or not connected
//      display.display();
//      delay(2000);
//      display.clearDisplay();
//      WiFi.status() != WL_CONNECTED
//
//
//
//
//      
//      Serial.println("Temperature is: "+temp);
//      Serial.println("City Name is: "+cityName);
//      Serial.println("Humidity is: "+humidity);
//      Serial.println("weather Description is: "+weatherDesc);
//      ShowWeatherOnLEDColor(weatherId);
//      
////      const char* situation = root["weather"][2]["description"];
////      //const char* situation = root["weather"][2];
////      Serial.println(situation);
//  } else {
//    Serial.println("Error on HTTP request");
//  }
//
//  http.end(); //Free the resources
//}


//void ShowWeatherOnLEDColor(String weatherId)
//{
//  char code = weatherId.charAt(0);
//  switch(code){
//    case '2':
//      Serial.println("LED color is: DarkBlue Blinking");
//      break;
//    case '3':
//      Serial.println("LED color is: VeryLightBlue");
//      break;
//    case '5':
//      Serial.println("LED color is: LightBlue");
//      break;
//    case '6':
//      Serial.println("LED color is: White");
//      break;
//    case '7':
//    case '8':
//      if(weatherId.equals("800"))
//        Serial.println("LED color is: Yellow");
//      else if(weatherId.equals("801"))
//        Serial.println("LED color is: lightgray");
//      else if(weatherId.equals("802"))
//        Serial.println("LED color is: silver");
//      else if(weatherId.equals("803"))
//        Serial.println("LED color is: grey");
//      else if(weatherId.equals("804"))
//        Serial.println("LED color is: dimgray");
//      break;
//  }
//}


void setup() {
  Serial.begin(9600);
  pinMode(TouchSensor1, INPUT);
  pinMode(TouchSensor2, INPUT);
  pinMode(Led_pin, OUTPUT);
  //  initialize LED
  pixels.begin(); // This initializes the NeoPixel library.
  // Turn off all LEDs
  off_all_leds();
  
//  // initialize LCD
//  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3D (for the 128x64)
//  // Clear Display.
//  display.clearDisplay();

}

void loop() { 
  
 
}
