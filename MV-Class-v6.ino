/**************************************************************
* Merrivale Primary School Weather Station Project
* STEM - Grade 4/5 - 2021 - Mr. White.
*
* To Do: Move the the lastest BMP package
* Get a sensor with hunidity
* Sort out the access point stuff
* 
* v4: Add BME280 I2C x76 / BMP280 I2C x77 / OLED x0x3C
* v5: Add BME280 I2C x76 / BME280 I2C x77 / OLED x0x3C
* v6: Add LED Start up test
*
*
**************************************************************/

/**** This section loads the various libraries that we need  *****/ 
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <SPI.h>
#include <ESP8266WiFi.h>

 String student = "Helena";     // Change the students name here
#define AccessPoint           // Comment this out to connect to WiFi
#ifdef AccessPoint
 const char* ssid = student.c_str();      // Enter wifi SSID here
 const char* pass = "";                   //Enter Password here
#else
/*Put your SSID & Password*/
 char ssid[] = "frodo";              // Enter wifi SSID here
 char pass[] = "pleasantr0ads";  //Enter Password here
#endif

//#define oneWirePresent

/**** Global Variables  **********/
const int oneWireBus = 2;  // DS18B20 sense pin 
const int ldrPin     = A0; // Light Dependent Resistor PinD
const int RedPin     = 14; // LED pins  ##Was 0
const int GreenPin   = 12;
const int BluePin    = 13; // ## Was 14

// Auxiliary variables to store the current output state of the RGB LED
String RedState   = "off";
String GreenState = "off";
String BlueState  = "off";
int IndicatorState = 0;

// Sensor Globals
float bme_humidity, bme_temperature, onewire_temp; 
int   bme_pressure, bme_altitude, light_sensor;
int   cycle = 0;

OneWire oneWire(oneWireBus); // Setup a oneWire instance to communicate with any OneWire devices
DallasTemperature dallasOneWire(&oneWire); // Pass our oneWire reference to Dallas Temperature sensor 

  #define SEALEVELPRESSURE_HPA (1013.25)
  Adafruit_BME280 bme; // I2C

// Define the screen size
#define SCREEN_WIDTH 128     // OLED display width, in pixels
#define SCREEN_HEIGHT 64     // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET     -1    // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

WiFiServer server(80);       //ESP8266WebServer server(80);  
String header;               // Variable to store the HTTP request

unsigned long currentTime = millis(); // Current time
unsigned long previousTime = 0;       // Previous time
const long timeoutTime = 2000;        // Define timeout time in milliseconds (example: 2000ms = 2s)

/***********************************************************/
void LedStart(){
/*  digitalWrite(RedPin,   LOW); // Set outputs to LOW
  digitalWrite(GreenPin, LOW);
  digitalWrite(BluePin,  LOW);  
  int * ledArray[3] = {&led00, &led01, &led02};*/

 const int *ledArray[3] = {&RedPin, &BluePin, &GreenPin};

for(int x = 0; x < 3; x++){
    Serial.print("Led Array ==== ");
    Serial.println(*ledArray[x]);

  for (int i = 0; i < 1024; i = i + 2){
    analogWrite(*ledArray[x], i);
    Serial.println(i);
    delay(1);
  }
  delay(10);
    for (int i = 1024; i > 0; i = i - 2){
    analogWrite(*ledArray[x], i);
    Serial.println(i);
    delay(1);}
    digitalWrite(*ledArray[x], LOW);
}
    
  delay(1000);
  
}



/****  Sensor Read  **********************/ 
void read_sensors(){
  dallasOneWire.requestTemperatures();          // Read the One Wire sensor
  onewire_temp         = dallasOneWire.getTempCByIndex(0); // Convert the reading into Celsius

  light_sensor         = analogRead(ldrPin);    // Read the Light Sensor

  bme_temperature      = bme.readTemperature(); // Read the various BME Sensor outputs
  bme_pressure         = bme.readPressure()/100;
  bme_altitude         = bme.readAltitude(SEALEVELPRESSURE_HPA);
  bme_humidity         = bme.readHumidity();

  // The following statements just print to the Serial port. 
  // They are not needed for the Merrivale Weather Station.
  Serial.print("Dallas OneWire temperature sensor reading : ");
  Serial.print(onewire_temp);
  Serial.println("ºC");
  
  Serial.print("Light Sensor Input = ");
  Serial.println(light_sensor);
  
  Serial.print("BME Temperature = ");
  Serial.print(bme_temperature);
  Serial.println(" *C");
    
  Serial.print("BME Pressure    = ");
  Serial.print(bme_pressure);
  Serial.println(" Pa");
    
  // Calculate altitude assuming 'standard' barometric
  // pressure of 1013.25 millibar = 101325 Pascal
  Serial.print("BME Altitude    = ");
  Serial.print(bme_altitude);
  Serial.println(" meters");

  Serial.print("BME Humidity    = ");
  Serial.print(bme_humidity);
  Serial.println(" %");

  // you can get a more precise measurement of altitude
  // if you know the current sea level pressure which will
  // vary with weather and such. If it is 1015 millibars
  // that is equal to 101500 Pascals.
 // Serial.print("BME Real altitude = ");
//  Serial.print(bme.readAltitude(102000));
//  Serial.println(" meters");   

  Serial.print("IP: ");  
  Serial.println(WiFi.localIP());

  Serial.println();

}

/**** WebPage  ***********************************/
void WebPage(){
  WiFiClient client = server.available();   // Listen for incoming clients

  if (client) {                             // If a new client connects,
    Serial.println("New Client.");          // print a message out in the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    currentTime = millis();
    previousTime = currentTime;
    while (client.connected() && currentTime - previousTime <= timeoutTime) { // loop while the client's connected
      currentTime = millis();         
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        Serial.write(c);                    // print it out the serial monitor
        header += c;
        if (c == '\n') {                    // if the byte is a newline character
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();

          //======== This section responds to the buttons, turning the LED on or off ============
            // turns the GPIOs on and off
            if (header.indexOf("GET /R/on") >= 0) {
              Serial.println("Red on");
              RedState = "on";
              digitalWrite(RedPin, HIGH);
            } else if (header.indexOf("GET /R/off") >= 0) {
              Serial.println("Red off");
              RedState = "off";
              digitalWrite(RedPin, LOW);

            } else if (header.indexOf("GET /G/on") >= 0) {
              Serial.println("GPIO G on");
              GreenState = "on";
              digitalWrite(GreenPin, HIGH);
            } else if (header.indexOf("GET /G/off") >= 0) {
              Serial.println("GPIO G off");
              GreenState = "off";
              digitalWrite(GreenPin, LOW);

            } else if (header.indexOf("GET /B/on") >= 0) {
              Serial.println("GPIO B on");
              BlueState = "on";
              digitalWrite(BluePin, HIGH);
            } else if (header.indexOf("GET /B/off") >= 0) {
              Serial.println("GPIO B off");
              BlueState = "off";
              digitalWrite(BluePin, LOW);
              
            } else if (header.indexOf("GET /I/on") >= 0) {
              Serial.println("LED Indicator on");
              IndicatorState = 1;
              // digitalWrite(BluePin, HIGH);
            } else if (header.indexOf("GET /I/off") >= 0) {
              Serial.println("LED Indicator off");
              IndicatorState = 0;
              digitalWrite(RedPin,   LOW);
              digitalWrite(GreenPin, LOW);
              digitalWrite(BluePin,  LOW);
            }

            // ==== This section builds the web page ======================
            // Display the HTML headers
            client.println("<!DOCTYPE html><html>");
            client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
      //      client.println("<link rel=\"icon\" href=\"data:,\">");
            client.println("<title>Merrivale Weather Station</title>\n");
            
            // Build the CSS to styles, mostly for the buttons 
            // Feel free to change the background-color and font-size attributes to fit your preferences
            client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");

            client.println(".button { background-color: #195B6A; border: none; color: white; padding: 16px 40px;");
            client.println("text-decoration: none; font-size: 30px; margin: 5px; cursor: pointer;");
            client.println("margin-left: 1.6em; margin-right: 1.6em;}");

            client.println(".button2 {background-color: #77878A;}");
                     
            client.println(".dot {height: 125px; width: 125px; background-color: red; border-radius: 50%; display: inline-block;}");
            client.println("</style></head>\n");
            
            // Web Page Heading
            client.println("<body><h1>" + student + "'s Web Server</h4>");

         read_sensors();  // Read the sensors, which we display next

            // ========= BME Sensor ===================================
            client.println("<br>\n<h3>BME180 Weather Station Data</h3>\n");
            client.print("<p>BME180 Temperature: ");
            client.print( bme_temperature);
            client.print(" &deg;C </p>\n"); 
            client.print("<p>BME_Humidity: ");
            client.print( bme_humidity ); 
            client.print(" %</p>\n "); 
            client.print("<p>BME180 Pressure: ");
            client.print( bme_pressure );
            client.print(" hPa</p>\n <p>BME180 Altitude: ");
            client.print( bme_altitude );
            client.print(" m</p>\n");

           // ========= OneWire Temp Sensor ==========================
            client.println("<br><h3>OneWire Weather Station Data</h1>");  //Heading
            client.print("<p>Dallas OneWire Temperature: ");
            client.print(onewire_temp);
            client.println("&deg;C</p>\n <br>");

            ledManager();

            // ========= RGB Buttons ====================================            
            // Build the buttons. These rely on the CSS styles    
            if (IndicatorState == 1) {
            client.println("<p><a href=\"/I/off\"><button class=\"button\" style=\"background-color: lightgrey\">ON</button></a>");
            } else { 
            
            if (RedState=="off")   {
            client.println("<p><a href=\"/R/on\"><button class=\"button\" style=\"background-color: lightcoral\">OFF</button></a>");
            } else { 
            client.println("<p><a href=\"/R/off\"><button class=\"button button2\" style=\"background-color: red\">ON</button></a>"); } 
                     
            if (GreenState=="off") {
            client.println("<a href=\"/G/on\"><button class=\"button\" style=\"background-color: lightgreen\">OFF</button></a>");
            } else { 
            client.println("<a href=\"/G/off\"><button class=\"button button2\" style=\"background-color: green\">ON</button></a>"); }

            if (BlueState=="off")  {
            client.println("<a href=\"/B/on\"><button class=\"button\" style=\"background-color: lightblue\">OFF</button></a></p>\n");
            } else { 
            client.println("<a href=\"/B/off\"><button class=\"button button2\" style=\"background-color: blue\">ON</button></a></p>\n"); }
            
            client.println("<p><a href=\"/I/on\"><button class=\"button button2\" style=\"background-color: darkgrey\">OFF</button></a>"); 

            // Display current LED state in text 
            client.print  ("<p> Red - State " + RedState   + "&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;");
            client.print  (  "Green - State " + GreenState + "&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;");
            client.println(   "Blue - State " + BlueState  + "</p>\n <br>\n <br>");
            }
           
            client.println("</body></html>");
            
            // The HTTP response ends with another blank line
            client.println();
            // Break out of the while loop
            break;
          } else { // if you got a newline, then clear currentLine
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }
      }
    }
    // Clear the header variable
    header = "";
    // Close the connection
    client.stop();
    Serial.println("Client disconnected.");  
    Serial.println("Waiting for next connection.");
    Serial.println("");
  }
}

/***********************************************************/
// LED Management
void ledManager(){
if(IndicatorState == 1){

  float iled = bme_temperature;
  if(iled > 30){ iled = 30;}
  if(iled < 5 ){ iled = 5 ;}
  iled = round((255 / 25) * (iled - 5));
  analogWrite(RedPin, iled);
  Serial.print("RedPin = ");
  Serial.println(iled);

  iled = bme_humidity;
  if(iled > 80){ iled = 80;}
  if(iled < 20 ){ iled = 20 ;}
  iled = round((255 / 60) * (iled - 20));
  analogWrite(BluePin, iled);
  Serial.print("BluePin = ");
  Serial.println(iled);

  iled = bme_pressure;
  if(iled > 1050){ iled = 1050;}
  if(iled < 920 ){ iled = 920 ;}
  iled = round((255 / 120) * (iled - 920));
  analogWrite(GreenPin, iled);
  Serial.print("GreenPin = ");
  Serial.println(iled);
  Serial.println();
  }
}

/****  Setup  **********************/ 
void setup() {
  Serial.begin(115200);    // Start the serial port
  delay(1000);             // A delay can help make sure we can reload boot code
    Serial.println(".");   // Add a couple of lines
    Serial.println("..");  // as there's always rubbish after a restart
    Serial.println("***");

    pinMode(ldrPin,   INPUT);  // Set the mode of the I/O pins
    pinMode(RedPin,   OUTPUT);
    pinMode(GreenPin, OUTPUT);
    pinMode(BluePin,  OUTPUT);

  digitalWrite(RedPin,   LOW); // Set outputs to LOW
  digitalWrite(GreenPin, LOW);
  digitalWrite(BluePin,  LOW);  

  // Start the DS18B20 sensor
  dallasOneWire.begin();
  Serial.println("DS18B20 started");

  // Start BME280
  bme.begin(0x76); 
  Serial.println("BME started");

#ifdef AccessPoint
  Serial.print("Setting soft-AP ... ");
  WiFi.mode(WIFI_AP); 
//  WiFi.mode(WIFI_AP_STA); // Set Wifi in Access Point mode
  boolean result = WiFi.softAP(student, pass);
  if(result == true){ 
    Serial.println("Ready"); } else { Serial.println("Failed!"); }

  delay(500);

  Serial.print("Soft-AP IP address = ");
  Serial.println(WiFi.softAPIP());
#else
 // Wifi connect code. Not used for an access point
  Serial.println(" ");
  Serial.println(" ");
  Serial.println("Connecting to ");
  Serial.println(ssid);

  int networkConnect = 1;
  // Connect to Wi-Fi
  WiFi.mode(WIFI_STA);                               // Set Wifi in Station Mode
//  WiFi.mode(WIFI_AP_STA); 
  WiFi.begin(ssid, pass);
  Serial.println("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print("Network connect attempt number : ");
    Serial.println(networkConnect);
    networkConnect++;
  }     
  
  Serial.println("");
  Serial.println("WiFi connected..!");
  Serial.print("Got IP: ");  Serial.println(WiFi.localIP());
#endif  

  LedStart();   // Run the LED Flash Routine

  server.begin();
  Serial.println("HTTP server started");

  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  // Start the display. 0x3C is the I2C address. (May vary between displays)
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
   Serial.println(F("SSD1306 allocation failed"));
  // for(;;); // If we don't find a display then don't proceed, loop forever
  }

  // Show initial display buffer contents on the screen --
  // the library initializes this with an Adafruit splash screen.
  display.display();
  delay(1000); // Pause for 2 seconds

  // Clear the buffer
  display.clearDisplay();     // Tell the display to clear
  // To show the display buffer on the screen. You MUST call display() after
  // drawing commands to make them visible on screen!
  display.display();          // This writes the changes to the display.
  delay(500);                // Pause for 2 seconds

  // display.display() is NOT necessary after every single drawing command,
  // unless that's what you want...rather, you can batch up a bunch of
  // drawing operations and then update the screen all at once by calling
  // display.display(). 

  // Run a subroutine to show the Merrivale logo
    MerrivaleScreen();

    Serial.print("Wifi Mode is : ");
    Serial.println(WiFi.getMode());
}

/***********************************************************/
//void screenTempRoutine(void) {  
void loop(){  
  // This is the amount of time we spend scrolling. It's enough time to move the text
  // from one side of the screen to the other
  int scrollDelay = 1200;              // The new screens need 1200, the old screen 2190
  int pauseTime = 200;                 // Dwell time at end of scroll

  read_sensors();                      // Go and read the temp sensor
  ledManager();

 //   display.setTextColor(SSD1306_WHITE);
  display.setTextColor(WHITE); // Draw white text
  display.cp437(true);                 // Use full 256 char 'Code Page 437' font

  int scroll = 0;                      // Used to track scroll direction
  int cursorX = 1;                     // Used to locate the x point on the cursor
  
  while(scroll < 2){                   // Can only be 0 or 1.
  display.clearDisplay();
//  int light = lightLevel(cursorX);     // Go and read the light sensor
//  ledLevel(light, cursorX);            // Go and set the LED level

  display.setCursor(cursorX + 4, 0);
  display.setTextSize(2);
  display.print(student);

  display.setTextSize(3);              // Normal 1:1 pixel scale
  display.setCursor(cursorX, 24);      // Start at top-left corner
  display.print(bme_temperature);

  display.setCursor(cursorX, 56);
  display.setTextSize(1);
  if(cycle == 0){ display.print("IP ");
        #ifdef AccessPoint 
                       display.print(WiFi.softAPIP());        
        #else          
                       display.print(WiFi.localIP()); cycle++; 
        #endif 
  }
  else if(cycle == 1){ display.print(" Pressure ");
                       display.print(bme_pressure);   cycle++;}
  else if(cycle == 2){ display.print(" Light Lvl ");
                       display.print(light_sensor);   cycle++;}    
  else if(cycle == 3){ display.print("1Wire Tmp ");
                       display.print(onewire_temp);   cycle++;}       
  else if(cycle == 4){ display.print(" Humidity ");
                       display.print(bme_humidity);   cycle++;}                
  else {display.print(" Altitude ");
        display.print(bme_altitude);   cycle=0;}

  display.display();                   // Write everything to the display
          
  if(scroll == 0){                     // Which way do we scroll?
      display.startscrollright(0x00, 0x0F);
      } else {
      display.startscrollleft(0x00, 0x0F);}
  
  for(int i = 0; i < 10; i++){
   WebPage();                          // Check to see if there a HTTP call
   delay(scrollDelay/10);              // Spend some time scrolling
  }
  display.stopscroll(); 
  delay(pauseTime);                    // Dwell after scrolling
  cursorX = 38;                        // Reset the cursor to the right side of screen
  scroll++;                            // Increment the scroll counter, so we scroll left
  }

}

  

/***********************************************************/
// Merrivale Splash Screen routine
void MerrivaleScreen() {
  int flash = 3000;              // How long is the screen displayed
  display.clearDisplay();
  display.setCursor(1,0);
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.print("Merrivale");
  display.setCursor(110,7);
  display.setTextSize(1);
  display.println(" PS");
  display.setCursor(5,20);
  display.setTextSize(1);
  display.println("Grade 4/5");
  display.setCursor(10,32);
  display.println("Mr. White");

  MerrivaleLogo();    // Draw a small bitmap image

  display.display();

// Flash the bit map. Make Mr White Famous!  //
while(flash > 3){
  // Invert and restore display, pausing in-between
  display.invertDisplay(false);
  delay(flash);
  display.invertDisplay(true);
  delay(flash);
  flash = flash / 3;
}
  display.invertDisplay(false);
}


/***********************************************************/
void MerrivaleLogo(void) {

// the process to generate a logo like this is described here:
// https://learn.adafruit.com/monochrome-oled-breakouts/arduino-library-and-examples
  
// define the size of the logo
#define LOGO_HEIGHT   50
#define LOGO_WIDTH    112

//------------------------------------------------------------------------------
// File generated by LCD Assistant
// http://en.radzio.dxp.pl/bitmap_converter/
//------------------------------------------------------------------------------
// The logo must be stored in 'PROGMEM'
static const unsigned char PROGMEM Merrivale_Logo_V5 [] = {

0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x1E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1F, 0x80,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1F, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3C, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x70, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x78,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1C, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x00, 0x1C, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x38, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0xFF, 0x00, 0x00, 0x00, 0x00,
0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x03, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x7F, 0xFF, 0xFF, 0xFF, 0xE0, 0x00, 0x00, 0x0C, 0x00, 0x00, 0x00,
0x00, 0x00, 0x07, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x78, 0x00, 0x00, 0x00, 0x00, 0x00,
0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xF8, 0x0F, 0xF0, 0x00, 0x0C, 0x00, 0x00, 0x07, 0xFF, 0xFF,
0xFF, 0xFF, 0xFF, 0xFF, 0xFE, 0x3F, 0xE0, 0x00, 0x03, 0x80, 0x00, 0x1F, 0xFF, 0xC0, 0x00, 0x00,
0x03, 0xFF, 0x8F, 0xFF, 0x80, 0x00, 0x00, 0x7C, 0x00, 0x7C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C,
0x7F, 0xFF, 0x00, 0x00, 0x00, 0x1F, 0xFF, 0xF8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0F, 0xFF, 0xF8,
0x00, 0x00, 0x00, 0x07, 0xFF, 0x7C, 0x00, 0x00, 0x00, 0x00, 0x03, 0xFF, 0xFF, 0xEA, 0x00, 0x00,
0x00, 0x01, 0xFF, 0xFF, 0xFF, 0x80, 0x00, 0x3F, 0xFF, 0xFF, 0xFF, 0x81, 0xC0, 0x00, 0x00, 0x00,
0x1F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFC, 0x00, 0x78, 0x00, 0x00, 0x00, 0x07, 0xFF,
0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xF8, 0x00, 0x1E, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF,
0xFF, 0xFF, 0xFF, 0xFF, 0xC0, 0x00, 0x01, 0x80, 0x00, 0x00, 0x00, 0x03, 0xFF, 0xFF, 0xFF, 0xFF,
0xFF, 0xF8, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x3F, 0xFF, 0xFF, 0xFF, 0xFF, 0x80,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xFF, 0xFF, 0xFF, 0xF0, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xFF, 0xF8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
};

  display.drawBitmap(
    (display.width() - LOGO_WIDTH ) / 2,
    (display.height() - LOGO_HEIGHT ),
    Merrivale_Logo_V5, LOGO_WIDTH, LOGO_HEIGHT, 1);
}
    
// The end.
    
