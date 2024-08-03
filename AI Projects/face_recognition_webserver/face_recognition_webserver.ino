#include <WiFi.h>
#include <WebServer.h>

// Replace with your network credentials
const char* ssid = "AlanAsh 2.4G";
const char* password = "Alan2760";

// Set web server port number to 80
WebServer server(80);

// Define LED pin
const int ledPin = 1;

void setup() {
  // Initialize serial monitor
  Serial.begin(115200);
  
  // Initialize LED pin as output
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);  // Turn off LED initially

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println();
  
  // Print local IP address
  Serial.print("Connected to Wi-Fi. IP address: ");
  Serial.println(WiFi.localIP());
  
  // Define the handling function for the root URL
  server.on("/", handleRoot);

  // Start the server
  server.begin();
  Serial.println("Web server started");
}

void loop() {
  // Handle client requests
  server.handleClient();
}

void handleRoot() {
  // Turn on the LED
  digitalWrite(ledPin, HIGH);

  // Send HTML response
  server.send(200, "text/html", "<h1>LED is ON</h1>");
}