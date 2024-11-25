#include <WiFi.h>
#include <ESPAsyncWebServer.h>

// Set up the ESP32 AP credentials
const char *ssid = "ESP32 WIFI";
const char *password = "esp32devkit";  // 8-63 characters long

// Onboard LED Pin
const int ledPin = 2;

// Create an instance of the web server
AsyncWebServer server(80);

// HTML and CSS for the web page
const char *htmlContent = R"rawliteral(
<!DOCTYPE HTML>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>ESP32 Web Server</title>
  <style>
    body {
      font-family: Arial, sans-serif;
      background-color: #f4f4f9;
      text-align: center;
      margin-top: 50px;
    }
    button {
      padding: 10px 20px;
      font-size: 16px;
      margin: 10px;
      cursor: pointer;
      border-radius: 5px;
      border: none;
      background-color: #4CAF50;
      color: white;
    }
    button:hover {
      background-color: #45a049;
    }
    #ledState {
      font-size: 20px;
      margin-top: 20px;
    }
  </style>
</head>
<body>
  <h1>Control Onboard LED</h1>
  <button onclick="toggleLED('on')">Turn ON</button>
  <button onclick="toggleLED('off')">Turn OFF</button>
  <p id="ledState">LED is OFF</p>

  <script>
    function toggleLED(state) {
      fetch('/toggleLED?state=' + state)
        .then(response => response.text())
        .then(data => {
          document.getElementById("ledState").innerHTML = "LED is " + data;
        });
    }
  </script>
</body>
</html>
)rawliteral";

// Setup the ESP32 as an Access Point
void setup() {
  Serial.begin(115200);

  // Set up the LED pin as OUTPUT
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);  // Initially turn off the LED

  // Set up the ESP32 as an Access Point
  WiFi.softAP(ssid, password);

  Serial.println("Access Point Started");
  Serial.print("IP Address: ");
  Serial.println(WiFi.softAPIP());

  // Serve the HTML content when a client connects
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/html", htmlContent);
  });

  // Handle the LED toggle requests
  server.on("/toggleLED", HTTP_GET, [](AsyncWebServerRequest *request){
    String state = request->getParam("state")->value();
    if (state == "on") {
      digitalWrite(ledPin, HIGH);  // Turn ON the LED
      request->send(200, "text/plain", "ON");
    } else if (state == "off") {
      digitalWrite(ledPin, LOW);  // Turn OFF the LED
      request->send(200, "text/plain", "OFF");
    } else {
      request->send(400, "text/plain", "Invalid State");
    }
  });

  // Start the web server
  server.begin();
}

void loop() {
  // Nothing needed here, all logic is handled by the web server
}
