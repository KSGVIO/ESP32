#include <WiFi.h>
#include <ESPAsyncWebServer.h>

// Set up the ESP32 AP credentials
const char *apSsid = "ESP32_WIFI";
const char *apPassword = "esp32devkit";  // 8-63 characters long

// Default WiFi credentials
const char *defaultSsid = "ANI-PC_Network";
const char *defaultPassword = "babacloanta";

// Onboard LED Pin
const int ledPin = 2;

// Create an instance of the web server
AsyncWebServer server(80);

// HTML and CSS for the web page (Firmware update section removed)
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
    #console {
      width: 100%;
      height: 200px;
      background-color: #333;
      color: #fff;
      padding: 10px;
      font-family: monospace;
      overflow-y: scroll;
    }
    input[type="text"] {
      padding: 10px;
      font-size: 16px;
      margin-top: 10px;
      width: 70%;
    }
    button.console-btn {
      background-color: #008CBA;
    }
  </style>
</head>
<body>
  <h1>Control Onboard LED</h1>
  <button onclick="toggleLED('on')">Turn ON</button>
  <button onclick="toggleLED('off')">Turn OFF</button>
  <p id="ledState">LED is OFF</p>

  <h2>Web Console</h2>
  <div id="console"></div>
  <input type="text" id="commandInput" placeholder="Enter command...">
  <button class="console-btn" onclick="sendCommand()">Send Command</button>

  <script>
    function toggleLED(state) {
      fetch('/toggleLED?state=' + state)
        .then(response => response.text())
        .then(data => {
          document.getElementById("ledState").innerHTML = "LED is " + data;
          logToConsole("LED is " + data);
        });
    }

    function sendCommand() {
      const command = document.getElementById("commandInput").value;
      if (command) {
        fetch('/command?cmd=' + command)
          .then(response => response.text())
          .then(data => {
            logToConsole(data);
            document.getElementById("commandInput").value = '';  // Clear input field
          });
      }
    }

    function logToConsole(message) {
      const consoleDiv = document.getElementById("console");
      consoleDiv.innerHTML += message + '<br>';
      consoleDiv.scrollTop = consoleDiv.scrollHeight;  // Auto-scroll to the bottom
    }
  </script>
</body>
</html>
)rawliteral";

// Tracks the last client activity time
unsigned long lastClientActivity = 0;
bool isAPModeActive = true;

// Function to connect to the default WiFi
void connectToDefaultWiFi() {
  Serial.println("Attempting to connect to default WiFi...");

  // Ensure WiFi is properly stopped before switching modes
  WiFi.softAPdisconnect(true);
  delay(100);  // Allow time for cleanup

  // Explicitly set WiFi mode to station
  WiFi.mode(WIFI_STA);
  WiFi.begin(defaultSsid, defaultPassword);

  unsigned long startAttemptTime = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 10000) {
    delay(500);
    Serial.print("Connecting...");
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nConnected to default WiFi!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
    isAPModeActive = false;
  } else {
    Serial.println("\nFailed to connect to default WiFi. Restarting AP.");
    // Restart Access Point
    WiFi.mode(WIFI_AP);
    WiFi.softAP(apSsid, apPassword);
    Serial.println("Access Point Restarted.");
    Serial.print("IP Address: ");
    Serial.println(WiFi.softAPIP());
    isAPModeActive = true;
  }
}

// Function to connect to a custom WiFi
void connectToWiFi(String ssid, String password) {
  Serial.println("Attempting to connect to WiFi: " + ssid);

  WiFi.softAPdisconnect(true);  // Disable Access Point
  WiFi.begin(ssid.c_str(), password.c_str());

  unsigned long startAttemptTime = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 10000) {
    delay(500);
    Serial.print("...");
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi Connected!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nFailed to connect to WiFi.");
  }
}

// Blink LED 5 times with 0.5 seconds delay
void blinkLEDBeforeUpdate() {
  for (int i = 0; i < 5; i++) {
    digitalWrite(ledPin, HIGH);
    delay(500); // LED ON for 0.5 seconds
    digitalWrite(ledPin, LOW);
    delay(500); // LED OFF for 0.5 seconds
  }
}

// Handle the LED toggle requests
void handleLEDRequest(AsyncWebServerRequest *request) {
  String state = request->getParam("state")->value();
  if (state == "on") {
    digitalWrite(ledPin, HIGH);  // Turn ON the LED
    request->send(200, "text/plain", "ON");
  } else if (state == "off") {
    digitalWrite(ledPin, LOW);  // Turn OFF the LED
    request->send(200, "text/plain", "OFF");
  }
}

// Handle the command request
String processCommand(String command) {
  String response = "Unknown Command\n";
  if (command == "ON") {
    digitalWrite(ledPin, HIGH);
    response = "LED is ON\n";
  } else if (command == "OFF") {
    digitalWrite(ledPin, LOW);
    response = "LED is OFF\n";
  } else if (command.startsWith("connect2wifi")) {
    int index = command.indexOf(' ');
    if (index != -1) {
      String wifiDetails = command.substring(index + 1);
      int separator = wifiDetails.indexOf(',');
      if (separator != -1) {
        String ssid = wifiDetails.substring(0, separator);
        String password = wifiDetails.substring(separator + 1);
        connectToWiFi(ssid, password);
        response = "Connecting to WiFi...\n";
      } else {
        response = "Error: Please provide SSID and password separated by a comma.\n";
      }
    } else {
      response = "Error: Missing SSID and password.\n";
    }
  } else if (command == "defaultwifi") {
    connectToDefaultWiFi();
    response = "Attempting to connect to default WiFi...\n";
  }
  return response;
}

void setup() {
  Serial.begin(115200);

  // Set up the LED pin as OUTPUT
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);  // Initially turn off the LED

  // Set up the ESP32 as an Access Point
  WiFi.softAP(apSsid, apPassword);

  Serial.println("Access Point Started");
  Serial.print("IP Address: ");
  Serial.println(WiFi.softAPIP());

  lastClientActivity = millis();

  // Serve the HTML content when a client connects
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/html", htmlContent);
    lastClientActivity = millis();
  });

  // Handle the LED toggle requests
  server.on("/toggleLED", HTTP_GET, handleLEDRequest);

  // Handle the command request
  server.on("/command", HTTP_GET, [](AsyncWebServerRequest *request) {
    String command = request->getParam("cmd")->value();
    String response = processCommand(command);
    request->send(200, "text/plain", response);
  });

  // Start the server
  server.begin();
}

void loop() {
  // Handle serial commands
  if (Serial.available()) {
    String serialCommand = Serial.readStringUntil('\n');
    String response = processCommand(serialCommand);
    Serial.println(response);
  }

  // Check for inactivity after 30 seconds
  if (isAPModeActive && millis() - lastClientActivity > 30000) {
    connectToDefaultWiFi();
  }
}
