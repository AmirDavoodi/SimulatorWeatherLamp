#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <Adafruit_NeoPixel.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <WorldClockClient.h>
//#include <Adafruit_SSD1306.h>
#include "SSD1306Wire.h"
#include "OLEDDisplayUi.h"
#include "icons.h"

// All our states in our lamp
#define S_TURNOFF 1
#define S_INITIALIZATION 2
#define S_WORKING 3
#define S_CITY_SELECTION 4

// All different colors of our LEDs
#define c_c_white 1
#define c_c_blue 2
#define c_c_purple 3
#define c_c_yellow 4 

// demo time limit 
#define demo_time_limit 5000
#define city_selection_timer_limit 5000

#define Led_pin            D1
#define NUMPIXELS      60
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, Led_pin, NEO_RGB + NEO_KHZ800);

// Initialize the OLED display using brzo_i2c
// D3 -> SDA
// D5 -> SCL

// Initialize the OLED display using Wire library
SSD1306Wire  display(0x3c, D3, D5);
OLEDDisplayUi ui     ( &display );




int led_light_up_number = NUMPIXELS; //- 20;

int TouchSensor1 = D2; //connected to Digital pin D2
int TouchSensor2 = D6; //connected to Digital pin D6
float brightness = 255; // Initialize Brightness
float fadeAmount = 25.5;
boolean booting_up = true;

// WiFi credentials
const char* ssid = "UC_PCOMP";
const char* password =  "pr3pAr3d";

// api credentials
const String api_url = "http://api.openweathermap.org/data/2.5/weather?&units=metric";
const String api_token = "&appid=134f461a6f03f55040a08b8935cd2f85";

String cities_name [] = {"Lugano, Switzerland", "Beijing, China", "Moscow, Russia", "Quito, Ecuador", "Tehran, Iran"};
String cities_weather_api [] = {"&q=Lugano,CH", "&q=Beijing,CN", "&q=Moscow,RU", "&q=Quito,EC", "&q=Tehran,IR"};

// time library configuration
String cities_time [] = {"Europe/Zurich", "Asia/Shanghai", "Europe/Moscow", "America/Guayaquil", "Asia/Tehran"};
WorldClockClient worldClockClient("en", "CH", "E, dd. MMMMM yyyy", 4, cities_time);
int current_location_index = 0;


// weather informations
String current_city_name = "";
String current_time = "";
String current_temperature = "";
String current_humidity = "";
String current_weather_desc = "";
String current_net_stat = "";
String current_api_stat = "";
String current_weather_id = "";
int current_led_color = 0;

static int state = S_TURNOFF; // initial state is (S_TURNOFF = 1) which means the "off" state.
static unsigned long start_timer;  // To store the "current" time for delays.
static unsigned long thunderstorm_timer; // timer for switching color of the LED for the thunderstorm
static unsigned long demo_timer;
static unsigned long city_selection_timer;

int delayval = 20;// Delay for a period of time (in milliseconds).

void off_all_leds(){
  for ( int led_num = 0; led_num < led_light_up_number; led_num++ ) {
    pixels.setPixelColor(led_num, pixels.Color(0, 0, 0));
  }
  pixels.show(); // This sends the updated pixel color to the hardware.
  delay(delayval); // Delay for a period of time (in milliseconds).
}

void bootup_effect(){
  int color_mode = 1;
  for ( int led_num = 0; led_num < led_light_up_number; led_num++ ) {
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
      pixels.setPixelColor(led_num, pixels.Color((int)brightness, 0, 0));
    } else if (color_mode == 2) {
      pixels.setPixelColor(led_num, pixels.Color(0, (int)brightness , 0));
    } else if (color_mode == 3) {
      pixels.setPixelColor(led_num, pixels.Color(0, 0 , (int)brightness));
    }

    pixels.show(); // This sends the updated pixel color to the hardware.
    delay(delayval); // Delay for a period of time (in milliseconds).
  }
  Serial.println("starting color motion");
}

void increase_light_intensity(int sensor1) {
  //INCREASES LED intensity
  if (sensor1 == HIGH) {
    delay(800);
    while (digitalRead(TouchSensor1) == HIGH && digitalRead(TouchSensor2) != HIGH) {
      if (brightness <= 255 - fadeAmount) {
        
        // LED affect
        brightness = fadeAmount + brightness;
        pixels.setBrightness((int)brightness);
        pixels.show(); // This sends the updated pixel color to the hardware.
        delay(delayval);

        Serial.println("brightness");
        Serial.println((int)brightness);
      }
      display.clear();
      // LCD intensity display
      // draw the progress bar
      display.drawProgressBar(0, 32, 120, 10, brightness/2.55);
    
      // draw the percentage as String
      display.setTextAlignment(TEXT_ALIGN_CENTER);
      display.drawString(64, 15, String(lround(brightness/2.55)) + "%");
      display.display();
      delay(800);
      display.clear();

      start_timer = millis()+59000;
    }
  }
}

void decrease_light_intensity(boolean sensor2) {
  //DECREASES LED intensity
  if (sensor2 == HIGH) {
    Serial.println("decreasing light");
    delay(800);
    while (digitalRead(TouchSensor2) == HIGH && digitalRead(TouchSensor1) != HIGH) {
      if (brightness > fadeAmount) {
        brightness = brightness - fadeAmount;
        
        // LED affect
        pixels.setBrightness((int)brightness);
        pixels.show(); // This sends the updated pixel color to the hardware.
        delay(delayval);
        
        Serial.println("brightness");
        Serial.println((int)brightness);
      }
      display.clear();
      // LCD intensity display
      // draw the progress bar
      display.drawProgressBar(0, 42, 120, 10, brightness/2.55);
    
      // draw the percentage as String
      display.setTextAlignment(TEXT_ALIGN_CENTER);
      display.drawString(64, 15, "Light Intensity:");
      display.drawString(64, 25, String(lround(brightness/2.55)) + "%");
      display.display();
      delay(800);
      display.clear();

      start_timer = millis()+59000;
    }
  }
}

// LCD welcome Screen
void greeting_sc(){
  display.clear();
  // draw circle for showing for greeting on the screen
  drawCircle();
  delay(2000);
  display.clear();
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.setFont(ArialMT_Plain_10);
  display.drawString(64, 32, "WELCOME!");
  display.display();
  delay(2000);
}

// LCD goodbye Screen
void goodbye_sc(){
  display.clear();
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.setFont(ArialMT_Plain_10);
  display.drawString(64, 32, "Goodbye For Now!");
  display.display();
  delay(2000);
  display.clear();
}

// LCD system is off
void system_off_sc(){
  display.clear();
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.setFont(ArialMT_Plain_10);
  display.drawString(64, 6, "SYSTEM OFF");
  display.drawStringMaxWidth(64, 32, 128,"Press both Buttons to Initiate");
  display.drawString(64, 40, "-\n- -\n-  -\n-   -\n-    -");
  display.display();
}


void drawCircle(void) {
  for (int16_t i=0; i<display.getHeight(); i+=2) {
    display.drawCircle(display.getWidth()/2, display.getHeight()/2, i);
    display.display();
    delay(10);
  }
  delay(1000);
  display.clear();
}



bool check_api_stat() {
  HTTPClient http;
  http.begin(api_url+api_token+cities_weather_api[current_location_index]); //Specify the URL
  int httpCode = http.GET();  //Make the request
  if (httpCode > 0) { //Check for the returning code
    http.end(); //Free the resources
    return true;
  } else {
    http.end(); //Free the resources
    return false;
  }

  
}

void get_weather_info() {
  
  HTTPClient http;
  http.begin(api_url+api_token+cities_weather_api[current_location_index]); //Specify the URL
  int httpCode = http.GET();  //Make the request

  if (httpCode > 0) { //Check for the returning code
      String json = http.getString();
      
      Serial.println(json);
      DynamicJsonBuffer jsonBuffer;
      JsonObject& root = jsonBuffer.parseObject(json);
      
      if (!root.success()){
        display.clear();
        display.setTextAlignment(TEXT_ALIGN_CENTER);
        display.setFont(ArialMT_Plain_10);
        display.drawStringMaxWidth(64, 32, 128, "API response ParseObject() failed!!!");
        display.display();
        delay(2000);
        Serial.print("API response ParseObject() failed!!!");
        return;
      }
      JsonObject& weather = root["weather"][0];
      current_weather_id = weather["id"].as<String>();
//      current_weather_id = "200";
      current_city_name = root["name"].as<String>(); 
      double temperature = root["main"]["temp"];
      current_temperature = String(lround(temperature));
      current_humidity = root["main"]["humidity"].as<String>(); 
      current_weather_desc = weather["description"].as<String>();

      // show weather on LCD
      show_weather_on_LCD();

      // show weather on LED
      show_weather_on_LED();
      
  } else {
    display.clear();
    display.setTextAlignment(TEXT_ALIGN_CENTER);
    display.setFont(ArialMT_Plain_10);
    display.drawStringMaxWidth(64, 32, 128, "Error on HTTP request");
    display.display();
    delay(2000);
    Serial.println("Error on HTTP request");
  }

  http.end(); //Free the resources
}

void show_weather_on_LCD(){
  String line1 = "CITY: "+ current_city_name;
  worldClockClient.updateTime();
  String line2 = worldClockClient.getHours(current_location_index) + ":" + worldClockClient.getMinutes(current_location_index);
  String line3 = current_temperature+"  C";
  String line4 = "HUMIDITY: "+ current_humidity +"%";
  String line5 = "DESCRIPTION: "+current_weather_desc;
  String line6 = "--------------------";
  String line7 = "Network status: Connected";
  String line8 = "API status: Connected";
  display.clear();
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(ArialMT_Plain_10);
  display.drawString(0, 0, line1+"\n"+line2+"\n"+line3+"\n"+line4+"\n"+line5+"\n"+line6+"\n"+line7+"\n"+line8+"\n");
  display.display();
  // pring celcius character
  display.drawCircle(15, 30, 1);
  display.display();
  delay(2000); 
}


void show_weather_on_LED()
{
  /*
   * Color mapping (API weather code ---> LED color)
   * 
   * thunder= 2XX ---> blue<->white  
   * Rain/drizzle = 5XX,3XX ---> Blue
   * Snow = 6XX ---> White
   * Clear = 800 --> Yellow
   * clouds/atmosphere = 7, 80X ---> purple  
  */
  char code = current_weather_id.charAt(0);
  switch(code){
    case '2':
      Serial.println("LED color is: blue<->white blinking");
      current_led_color = c_c_blue;
      thunderstorm_timer = millis();  // Remember the current time
      leds_color_change(0, 0, 255);
      break;
    case '3':
    case '5':
      Serial.println("LED color is: Blue");
      current_led_color = c_c_blue;
      leds_color_change(0, 0, 255);
      break;
    case '6':
      Serial.println("LED color is: White");
      current_led_color = c_c_white;
      leds_color_change(100,140,140);
      break;
    case '7':
    case '8':
      if(current_weather_id.equals("800")){
        Serial.println("LED color is: Yellow");
        current_led_color = c_c_yellow;
        leds_color_change(140, 140, 0);
      } else{
        Serial.println("LED color is: purple");
        current_led_color = c_c_purple;
        leds_color_change(30,60,150);
      }
      break;
  }
}


//change the color of all leds
void leds_color_change(int r, int g, int b){
  for ( int led_num = 0; led_num < led_light_up_number; led_num++ ) {
    pixels.setPixelColor(led_num, pixels.Color(r, g, b));
  }
  pixels.show(); // This sends the updated pixel color to the hardware.
  delay(delayval); // Delay for a period of time (in milliseconds).
}

//Select color to display, or demo of all settings (TBD)
void LCD_text_display(String text){
    display.clear();
    display.setTextAlignment(TEXT_ALIGN_CENTER);
    display.setFont(ArialMT_Plain_10);
    display.drawString(64, 32, text);
    display.display();
    display.clear();
}

// check demo intruption
bool check_demo_intruption(){
  if (digitalRead(TouchSensor1) == HIGH || digitalRead(TouchSensor2) == HIGH) {
    return true;    
  }else {
    return false;  
  }
}


// demo main function
void Demo_mode(){
  //LCD_text_display(text, delay)
  LCD_text_display("Initializing Lamp Demo");
  delay(2000);

  //Type of Weather displays 
  LCD_text_display("Weather Conditions Demo");
  delay(2000);

  //Clear
  LCD_text_display("CLEAR SKY");
  leds_color_change(140, 140, 0);
  delay(2000);
  
  //Clouds
  LCD_text_display("CLOUDY");
  leds_color_change(47,79,79);
  delay(2000);

  //ThurderStorm
  LCD_text_display("THUNDERSTORM");
  leds_color_change(0, 0, 255);
  for( int i = 0; i < 4; i++ ) {
    // blinking effect for thunderstorm    
    delay(2000);
    // change the color to white
    leds_color_change(100,140,140);
    delay(100);
    leds_color_change(0, 0, 255);
  }

  //Rain/drizzle
  LCD_text_display("RAIN/DRIZZLE");
  leds_color_change(0, 0, 255); 
  delay(2000);

  //Snow
  LCD_text_display("SNOWY");
  leds_color_change(100,140,140); 
  delay(2000);

  //End of Weather Demo
  LCD_text_display("End of Weather Demo");
  leds_color_change(0,0,0); //turns LEDs off
  delay(2000);

  //LED Intensity decrease/increase
  LCD_text_display("LED Intensity Demo");
  
  // change the color to white
  leds_color_change(100,140,140);
  LCD_text_display("LED Intensity Decrease");
  delay(2000);
  
  float fade=25.5;
  float how_bright=255;
  while (how_bright>fade  ) {
    // LED affect
    how_bright =  how_bright - fade;
    pixels.setBrightness((int)how_bright);
    pixels.show(); // This sends the updated pixel color to the hardware.
    delay(delayval);
    
    // LCD affect
    display.clear();
    
    // LCD intensity display
    // draw the progress bar
    display.drawProgressBar(0, 32, 120, 10, how_bright/2.55);
    
    // draw the percentage as String
    display.setTextAlignment(TEXT_ALIGN_CENTER);
    display.drawString(64, 15, String(lround(how_bright/2.55)) + "%");
    display.display();
    delay(800);
    display.clear();
  }
  
  LCD_text_display("Min intensity reached!");
  delay(2000);

  while (how_bright<255  ) {
    // LED affect
    how_bright =  how_bright + fade;
    pixels.setBrightness((int)how_bright);
    pixels.show(); // This sends the updated pixel color to the hardware.
    delay(delayval);
    
    // LCD affect
    display.clear();
    
    // LCD intensity display
    // draw the progress bar
    display.drawProgressBar(0, 32, 120, 10, how_bright/2.55);
    
    // draw the percentage as String
    display.setTextAlignment(TEXT_ALIGN_CENTER);
    display.drawString(64, 15, String(lround(how_bright/2.55)) + "%");
    display.display();
    delay(800);
    display.clear();
  }
  
  LCD_text_display("Max intensity reached!");
  delay(2000);

  //End of LED intensity Demo
  LCD_text_display("End of LED intensity Demo");
  delay(2000);
  LCD_text_display("End of Lamp Demo");
  delay(2000);

  //TURNING SYSTEM OFF
  off_all_leds(); // Turn off all LEDs
  goodbye_sc();    // LCD turn off
  system_off_sc();  // display system is off on LCD
  state = S_TURNOFF;  //change state to OFF_STATE
}

// show all available cities on the LCD
void show_all_available_cities(){
  display.clear();
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(ArialMT_Plain_10);
  for( int i_cities = 0; i_cities < 5; i_cities++ ) {
     display.drawString(0, 0 + (i_cities*12), cities_name[i_cities] + "     " + ((i_cities == current_location_index) ? "<<<" : "") );
  }
  display.display();
}

// select next city from the list of all available city for taking time and weather data from Internet (API)
void select_next_city(int sensor1){
  if (sensor1 == HIGH) {
    delay(300);
    while (digitalRead(TouchSensor1) == HIGH && digitalRead(TouchSensor2) != HIGH) {
      if (current_location_index == 4) {
        current_location_index = 0;
      } else{
        current_location_index++;
      }
      show_all_available_cities();
      delay(800);
    }
  }
}

// select previous city from the list of all available city for taking time and weather data from Internet (API)
void select_previous_city(int sensor2){
  if (sensor2 == HIGH) {
    delay(300);
    while (digitalRead(TouchSensor2) == HIGH && digitalRead(TouchSensor1) != HIGH) {
      if (current_location_index == 0) {
        current_location_index = 4;
      } else{
        current_location_index--;
      }
      show_all_available_cities();
      delay(800);
    }
  }
}


void setup() {
  Serial.begin(9600);
  pinMode(TouchSensor1, INPUT);
  pinMode(TouchSensor2, INPUT);
  pinMode(Led_pin, OUTPUT);
  //  initialize LED
  pixels.begin(); // This initializes the NeoPixel library.
  
  // Turn off all LEDs
  off_all_leds();

  
  //   initialize LCD
  // Initialising the UI will init the display too.
  display.init();

  display.flipScreenVertically();
  display.setFont(ArialMT_Plain_10);
  
   // display system is off on LCD
  system_off_sc();
  Serial.println("end of setup part");

}

void loop() {
  switch(state)
  {
    case S_TURNOFF:
      {
        if (digitalRead(TouchSensor1) == HIGH && digitalRead(TouchSensor2) == HIGH) {
          //Change of state
          demo_timer=millis();
          state = S_INITIALIZATION;
          while (digitalRead(TouchSensor1) == HIGH || digitalRead(TouchSensor2) == HIGH) {
            delay(10);
            if (millis()-demo_timer >= demo_time_limit){
              Demo_mode();
              state = S_TURNOFF;
            }
          }
        }
        break;
      }
    case S_INITIALIZATION:
      {
        start_timer = millis();  // Remember the current time
         
        // LED effect
        brightness = 255;
        bootup_effect();
        delay(delayval); // Delay for a period of time (in milliseconds).
        
        // LCD welcome message
        greeting_sc();
        display.clear();
        
        delay(delayval); // Delay for a period of time (in milliseconds).
  
        // connect through WiFi
        WiFi.begin(ssid, password);
        int counter = 0;
        while (WiFi.status() != WL_CONNECTED) {
          delay(500);
          Serial.print(".");
          display.clear();
          display.drawString(64, 10, "Connecting to WiFi");
          display.drawXbm(46, 30, 8, 8, counter % 3 == 0 ? activeSymbol : inactiveSymbol);
          display.drawXbm(60, 30, 8, 8, counter % 3 == 1 ? activeSymbol : inactiveSymbol);
          display.drawXbm(74, 30, 8, 8, counter % 3 == 2 ? activeSymbol : inactiveSymbol);
          display.display();
          counter++;
        }
        
        // show success message on screen
        display.clear();
        display.setTextAlignment(TEXT_ALIGN_CENTER);
        display.setFont(ArialMT_Plain_10);
        display.drawStringMaxWidth(64, 23, 128, "WiFi Connected Successfully!");
        display.display();
        Serial.println("WiFi Connected Successfully!");
        delay(2000);

        if(check_api_stat()){
          // show success message on screen
          display.clear();
          display.setTextAlignment(TEXT_ALIGN_CENTER);
          display.setFont(ArialMT_Plain_10);
          display.drawStringMaxWidth(64, 23, 128, "API Connected Successfully!");
          display.display();
          Serial.println("API Connected Successfully!");
          delay(2000);
        }else{
          // show success message on screen
          display.clear();
          display.setTextAlignment(TEXT_ALIGN_CENTER);
          display.setFont(ArialMT_Plain_10);
          display.drawString(64, 32, "<<<API Connection Error>>>");
          display.display();
          Serial.println("<<<API Connection Error>>>");
          delay(2000);
        }

        // get current time from wifi
        worldClockClient.updateTime();
        Serial.println("Current time is: " + worldClockClient.getHours(current_location_index) + ":" + worldClockClient.getMinutes(current_location_index));
        
        get_weather_info();
        state = S_WORKING;
        break;
      }
    case S_WORKING:
      {
        // blinking effect for thunderstorm        
        if( current_weather_id.charAt(0) == '2'){
          if( (millis() - thunderstorm_timer) > 2000){
            thunderstorm_timer = millis();
            if(current_led_color == c_c_blue){
              // change the color to white
              leds_color_change(100,140,140);
              current_led_color = c_c_white;
              thunderstorm_timer = millis()+ 1000;
            } else{
              // change the color to blue
              leds_color_change(0, 0, 255);
              current_led_color = c_c_blue;
            }
          }
        }
        
        if( (millis() - start_timer) > 60000){
          start_timer = millis();
          get_weather_info();
        }  
      
        //Detects double button pressed for TURNING OFF the lamp or Switch to City Selection Mode 
        if (digitalRead(TouchSensor1) == HIGH && digitalRead(TouchSensor2) == HIGH) {
          
          //Change the state to whether to TURN_OFF state or CITY_SELECTION_STATE
          city_selection_timer=millis();
          
          while (digitalRead(TouchSensor1) == HIGH || digitalRead(TouchSensor2) == HIGH) {
            delay(10);
          }
          if (millis()-city_selection_timer >= city_selection_timer_limit){
            // Turn off all LEDs
            off_all_leds();

            // LCD turn off
            goodbye_sc();
          
            // display system is off on LCD
            system_off_sc();
            
            state = S_TURNOFF;
            break;
          } else{
            state = S_CITY_SELECTION;
            
            // show all cities on LCD
            show_all_available_cities();
            break;
          }
        }
        increase_light_intensity(digitalRead(TouchSensor1));
        decrease_light_intensity(digitalRead(TouchSensor2));
        break;
      }
    case S_CITY_SELECTION:
      {
        // listen to touchsensor1 for switching between cities
        select_next_city(digitalRead(TouchSensor1));
        
        // listen to touchsensor2 for switching between cities
        select_previous_city(digitalRead(TouchSensor2));

        // listen to both touchsensors for swtiching back to working state
        if (digitalRead(TouchSensor1) == HIGH && digitalRead(TouchSensor2) == HIGH) {
          while (digitalRead(TouchSensor1) == HIGH || digitalRead(TouchSensor2) == HIGH) {
            delay(10);
          }
          get_weather_info();
          state = S_WORKING;
        }
        break;
      }
    default:
      {
        state = S_TURNOFF;
        break;
      }
  }
}

