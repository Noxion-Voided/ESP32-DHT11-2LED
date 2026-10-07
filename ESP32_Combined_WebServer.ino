/*********
  Rui Santos
  Complete project details at https://randomnerdtutorials.com

  Combined sketch: ESP32 Web Server (GPIO 26/27 control)
  + ESP32 DHT Server (temperature & humidity)
*********/

// Import required libraries
#include "WiFi.h"
#include "ESPAsyncWebServer.h"
#include <Adafruit_Sensor.h>
#include <DHT.h>

// Replace with your network credentials
const char* ssid = "THE BLACK SILENCE";
const char* password = "ersansinksan4322";

// DHT sensor (GPIO 27 is used by the LED/relay output below, so the DHT is moved to GPIO 4)
#define DHTPIN 4    // Digital pin connected to the DHT sensor
// Uncomment the type of sensor in use:
#define DHTTYPE DHT11   // DHT 11
//#define DHTTYPE DHT22   // DHT 22  (AM2302)
//#define DHTTYPE DHT21   // DHT 21  (AM2301)

DHT dht(DHTPIN, DHTTYPE);

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

// Auxiliar variables to store the current output state
String output26State = "off";
String output27State = "off";

// Assign output variables to GPIO pins
const int output26 = 26;
const int output27 = 27;

String readDHTTemperature() {
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  // Read temperature as Celsius (the default)
  float t = dht.readTemperature();
  // Read temperature as Fahrenheit (isFahrenheit = true)
  //float t = dht.readTemperature(true);
  // Check if any reads failed and exit early (to try again).
  if (isnan(t)) {
    Serial.println("Failed to read from DHT sensor!");
    return "--";
  }
  else {
    Serial.println(t);
    return String(t);
  }
}

String readDHTHumidity() {
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  float h = dht.readHumidity();
  if (isnan(h)) {
    Serial.println("Failed to read from DHT sensor!");
    return "--";
  }
  else {
    Serial.println(h);
    return String(h);
  }
}

const char index_html[] PROGMEM = R"rawliteral(<!DOCTYPE HTML><html><head>
<meta name="viewport" content="width=device-width, initial-scale=1">
<link rel="stylesheet" href="https://use.fontawesome.com/releases/v5.7.2/css/all.css" integrity="sha384-fnmOCqbTlWIlj8LyTjo7mOUStjsKC4pOpQbqyi7RrhN7udi9RwhKkMHpvLbHG9Sr" crossorigin="anonymous">
<style>
html {
  font-family: Arial;
  display: inline-block;
  margin: 0px auto;
  text-align: center;
}
h2 { font-size: 3.0rem; }
p { font-size: 3.0rem; }
.units { font-size: 1.2rem; }
.dht-labels{
  font-size: 1.5rem;
  vertical-align:middle;
  padding-bottom: 15px;
}
.button { background-color: #4CAF50; border: none; color: white; padding: 16px 40px;
  text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}
.button2 {background-color: #555555;}
</style></head><body>
<h2>ESP32 Web Server</h2>

<p>
<i class="fas fa-thermometer-half" style="color:#059e8a;"></i>
<span class="dht-labels">Temperature</span>
<span id="temperature">%TEMPERATURE%</span>
<sup class="units">&deg;C</sup>
</p>

<p>
<i class="fas fa-tint" style="color:#00add6;"></i>
<span class="dht-labels">Humidity</span>
<span id="humidity">%HUMIDITY%</span>
<sup class="units">&percnt;</sup>
</p>

<p>GPIO 26 - State %GPIO26_STATE%</p>
%GPIO26_BUTTON%

<p>GPIO 27 - State %GPIO27_STATE%</p>
%GPIO27_BUTTON%

</body><script>
setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("temperature").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/temperature", true);
  xhttp.send();
}, 10000 ) ;

setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("humidity").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/humidity", true);
  xhttp.send();
}, 10000 ) ;
</script></html>)rawliteral";

// Replaces placeholder with DHT values and GPIO button states
String processor(const String& var){
  //Serial.println(var);
  if(var == "TEMPERATURE"){
    return readDHTTemperature();
  }
  else if(var == "HUMIDITY"){
    return readDHTHumidity();
  }
  else if(var == "GPIO26_STATE"){
    return output26State;
  }
  else if(var == "GPIO27_STATE"){
    return output27State;
  }
  else if(var == "GPIO26_BUTTON"){
    // If the output26State is off, it displays the ON button
    if (output26State == "off") {
      return "<p><a href=\"/26/on\"><button class=\"button\">ON</button></a></p>";
    } else {
      return "<p><a href=\"/26/off\"><button class=\"button button2\">OFF</button></a></p>";
    }
  }
  else if(var == "GPIO27_BUTTON"){
    // If the output27State is off, it displays the ON button
    if (output27State == "off") {
      return "<p><a href=\"/27/on\"><button class=\"button\">ON</button></a></p>";
    } else {
      return "<p><a href=\"/27/off\"><button class=\"button button2\">OFF</button></a></p>";
    }
  }
  return String();
}

void setup(){
  // Serial port for debugging purposes
  Serial.begin(115200);

  // Initialize the output variables as outputs and set them to LOW
  pinMode(output26, OUTPUT);
  pinMode(output27, OUTPUT);
  digitalWrite(output26, LOW);
  digitalWrite(output27, LOW);

  dht.begin();

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }

  // Print ESP32 Local IP Address
  Serial.println(WiFi.localIP());

  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html, processor);
  });

  // Turn GPIO 26 on and off
  server.on("/26/on", HTTP_GET, [](AsyncWebServerRequest *request){
    Serial.println("GPIO 26 on");
    output26State = "on";
    digitalWrite(output26, HIGH);
    request->send_P(200, "text/html", index_html, processor);
  });
  server.on("/26/off", HTTP_GET, [](AsyncWebServerRequest *request){
    Serial.println("GPIO 26 off");
    output26State = "off";
    digitalWrite(output26, LOW);
    request->send_P(200, "text/html", index_html, processor);
  });

  // Turn GPIO 27 on and off
  server.on("/27/on", HTTP_GET, [](AsyncWebServerRequest *request){
    Serial.println("GPIO 27 on");
    output27State = "on";
    digitalWrite(output27, HIGH);
    request->send_P(200, "text/html", index_html, processor);
  });
  server.on("/27/off", HTTP_GET, [](AsyncWebServerRequest *request){
    Serial.println("GPIO 27 off");
    output27State = "off";
    digitalWrite(output27, LOW);
    request->send_P(200, "text/html", index_html, processor);
  });

  // DHT readings as plain text (used by the JavaScript to update every 10 seconds)
  server.on("/temperature", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", readDHTTemperature().c_str());
  });
  server.on("/humidity", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", readDHTHumidity().c_str());
  });

  // Start server
  server.begin();
}

void loop(){

}
