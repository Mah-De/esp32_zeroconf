#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <EEPROM.h>

#define EEPROM_SIZE 512 // Define the EEPROM size
#define STRING_SIZE 64  // Max length for strings
#define MAGIC_VALUE 12345  // Magic value to indicate that the parameters have been initialized

String server_ip = "192.168.1.1";     // Example default IP address
String server_port = "8080";          // Example default port
String tag = "ESP32_Device";          // Example default tag
// String wifi_ssid = "MINE5213";        // Example default WiFi SSID
// String wifi_password = "qwertyuiop";  // Example default WiFi password
String wifi_ssid = "Cloner";        // Example default WiFi SSID
String wifi_password = "Shp32424139";  // Example default WiFi password
String ap_ssid = "ESP32AP";        // Example default WiFi SSID
String ap_password = "12345678";  // Example default WiFi password
String panel_username = "admin";      // Example default panel username
String panel_password = "admin123";   // Example default panel password
int sample_rate = 44100;              // Example default sample rate
int bit_resolution = 16;              // Example default bit resolution

// Create AsyncWebServer objects for AP mode
AsyncWebServer serverAP(80);

std::vector<uint32_t> AuthorizedIPAddresses;  // Vector to store IP addresses as 32-bit integers

void handleNotFoundRequest(AsyncWebServerRequest* request);
void handleLogoutGetRequest(AsyncWebServerRequest* request);
void handleLoginPostRequest(AsyncWebServerRequest* request);
void handleLoginGetRequest(AsyncWebServerRequest* request);
void handleConfigPostRequest(AsyncWebServerRequest* request);
void handleConfigGetRequest(AsyncWebServerRequest* request);
void handleRootGetRequest(AsyncWebServerRequest* request);
void setupSTAMode();
void setupAPMode();
String getLoginPage();
String getConfigPage();
void removeIPAddress(IPAddress ip);
bool checkIPAddress(IPAddress ip);
void insertIPAddress(IPAddress ip);
uint32_t ipToInt(IPAddress ip);

// Helper functions for writing and reading data
void writeStringToEEPROM(int address, String value);
String readStringFromEEPROM(int address);
void writeIntToEEPROM(int address, int value);
int readIntFromEEPROM(int address);



void setup() {
  // Start serial communication
  Serial.begin(115200);

  // Allow some time for STA mode to connect
  delay(1000);  // Optional, gives some time for the STA connection to stabilize

  // Initialize EEPROM
  if (!EEPROM.begin(EEPROM_SIZE)) {
    Serial.println("Failed to initialize EEPROM");
    return;
  }

  // Check if parameters are already saved in EEPROM by checking for MAGIC_VALUE
  int magic_value = readIntFromEEPROM(0);
  if (magic_value != MAGIC_VALUE)
  {
    // If MAGIC_VALUE is not found, write default values to EEPROM
    Serial.println("EEPROM not initialized, writing default values.");
    
    writeIntToEEPROM(0, MAGIC_VALUE); // Store the magic value to indicate initialization

    writeStringToEEPROM(4, server_ip);
    writeStringToEEPROM(4 + STRING_SIZE, server_port);
    writeStringToEEPROM(4 + 2 * STRING_SIZE, tag);
    writeStringToEEPROM(4 + 3 * STRING_SIZE, wifi_ssid);
    writeStringToEEPROM(4 + 4 * STRING_SIZE, wifi_password);
    writeStringToEEPROM(4 + 5 * STRING_SIZE, panel_username);
    writeStringToEEPROM(4 + 6 * STRING_SIZE, panel_password);
    writeIntToEEPROM(4 + 7 * STRING_SIZE, sample_rate);
    writeIntToEEPROM(4 + 7 * STRING_SIZE + 4, bit_resolution);

    EEPROM.commit(); // Commit changes to EEPROM
  }
  else
  {
    // If MAGIC_VALUE is found, read the parameters into variables
    Serial.println("EEPROM already initialized, reading values.");

    server_ip = readStringFromEEPROM(4);
    server_port = readStringFromEEPROM(4 + STRING_SIZE);
    tag = readStringFromEEPROM(4 + 2 * STRING_SIZE);
    wifi_ssid = readStringFromEEPROM(4 + 3 * STRING_SIZE);
    wifi_password = readStringFromEEPROM(4 + 4 * STRING_SIZE);
    panel_username = readStringFromEEPROM(4 + 5 * STRING_SIZE);
    panel_password = readStringFromEEPROM(4 + 6 * STRING_SIZE);
    sample_rate = readIntFromEEPROM(4 + 7 * STRING_SIZE);
    bit_resolution = readIntFromEEPROM(4 + 7 * STRING_SIZE + 4);
  }

  // Print the values read from EEPROM
  Serial.println("Server IP: " + server_ip);
  Serial.println("Server Port: " + server_port);
  Serial.println("Tag: " + tag);
  Serial.println("Wi-Fi SSID: " + wifi_ssid);
  Serial.println("Wi-Fi Password: " + wifi_password);
  Serial.println("Panel Username: " + panel_username);
  Serial.println("Panel Password: " + panel_password);
  Serial.println("Sample Rate: " + String(sample_rate));
  Serial.println("Bit Resolution: " + String(bit_resolution));


  // Set the ESP32 to work in both AP and STA modes
  WiFi.mode(WIFI_MODE_APSTA);  // Enable both AP and STA modes

  // Start AP mode server
  setupAPMode();  // Start AP mode server

  // Allow some time for STA mode to connect
  delay(1000);  // Optional, gives some time for the STA connection to stabilize

  // Start STA mode after connection is established
  setupSTAMode();  // Start STA mode

  

}

void loop() {
  // Main loop can stay empty, AsyncWebServer handles everything in the background
}








// Function to convert IP address to a 32-bit integer
uint32_t ipToInt(IPAddress ip) {
  return (static_cast<uint32_t>(ip[0]) << 24) | (static_cast<uint32_t>(ip[1]) << 16) | (static_cast<uint32_t>(ip[2]) << 8) | static_cast<uint32_t>(ip[3]);
}

// Function to insert an IP address into the global list (stored as 32-bit integers)
void insertIPAddress(IPAddress ip) {
  uint32_t ipInt = ipToInt(ip);
  AuthorizedIPAddresses.push_back(ipInt);
  Serial.println("IP Added: " + ip.toString());
}

// Function to check if a specific IP address exists in the list (as 32-bit integer)
bool checkIPAddress(IPAddress ip) {
  uint32_t ipInt = ipToInt(ip);

  // Search for the IP in the vector
  for (uint32_t storedIP : AuthorizedIPAddresses) {
    if (storedIP == ipInt) {
      return true;  // IP found
    }
  }
  return false;  // IP not found
}

// Function to remove an IP address from the vector
void removeIPAddress(IPAddress ip) {
  uint32_t ipInt = ipToInt(ip);

  // Iterate through the vector to find and remove the IP address
  for (auto it = AuthorizedIPAddresses.begin(); it != AuthorizedIPAddresses.end(); ++it) {
    if (*it == ipInt) {
      AuthorizedIPAddresses.erase(it);  // Remove the IP from the vector
      Serial.println("IP Removed: " + ip.toString());
      return;
    }
  }
  Serial.println("IP not found in the list: " + ip.toString());
}


String getConfigPage() {
  String configPage = R"(<!DOCTYPE html><html lang="en"><head><meta charset="UTF-8"><meta name="viewport" content="width=device-width, initial-scale=1.0"><title>Configuration Page</title><style>body{font-family:Arial,sans-serif;background-color:#121212;color:#ddd;margin:0;padding:0}header{background-color:#333;padding:10px;text-align:center}header a{color:#fff;margin:0 15px;text-decoration:none}header a:hover{color:#28a745}main{padding:20px}.form-group{margin-bottom:15px}label{display:block;font-size:14px;margin-bottom:5px}input,select{width:100%;padding:10px;margin-top:5px;background-color:#333;border:1px solid #444;color:#ddd;box-sizing:border-box;font-size:14px}input[type="submit"]{width:100%;padding:15px;background-color:#28a745;border:none;cursor:pointer;font-size:16px}input[type="submit"]:hover{background-color:#218838}.container{max-width:600px;margin:0 auto;background-color:#222;padding:20px;border-radius:8px;box-sizing:border-box}</style></head><body><header><a href="/logout">Logout</a><a href="/config">Config</a></header><main><div class="container"><h2>Configuration Settings</h2><form action="/config" method="POST"><div class="form-group"><label for="server_ip">Server IP</label><input type="text" id="server_ip" name="server_ip" value="%SERVER_IP%" placeholder="Enter Server IP" required></div><div class="form-group"><label for="server_port">Server Port</label><input type="number" id="server_port" name="server_port" value="%SERVER_PORT%" placeholder="Enter Server Port" required></div><div class="form-group"><label for="tag">Tag</label><input type="text" id="tag" name="tag" value="%TAG%" placeholder="Enter Tag" required></div><div class="form-group"><label for="wifi_ssid">WiFi SSID</label><input type="text" id="wifi_ssid" name="wifi_ssid" value="%WIFI_SSID%" placeholder="Enter WiFi SSID" required></div><div class="form-group"><label for="wifi_password">WiFi Password</label><input type="password" id="wifi_password" name="wifi_password" value="%WIFI_PASSWORD%" placeholder="Enter WiFi Password" required></div><div class="form-group"><label for="sample_rate">Sample Rate</label><select id="sample_rate" name="sample_rate" required><option value="8000" %SAMPLE_RATE_8000%>8000</option><option value="16000" %SAMPLE_RATE_16000%>16000</option><option value="44100" %SAMPLE_RATE_44100%>44100</option><option value="48000" %SAMPLE_RATE_48000%>48000</option></select></div><div class="form-group"><label for="bit_resolution">Bit Resolution</label><select id="bit_resolution" name="bit_resolution" required><option value="16" %BIT_RESOLUTION_16%>16</option><option value="32" %BIT_RESOLUTION_32%>32</option></select></div><div class="form-group"><label for="panel_username">Panel Username</label><input type="text" id="panel_username" name="panel_username" value="%PANEL_USERNAME%" placeholder="Enter Panel Username" required></div><div class="form-group"><label for="panel_password">Panel Password</label><input type="password" id="panel_password" name="panel_password" value="%PANEL_PASSWORD%" placeholder="Enter Panel Password" required></div><div class="form-group"><input type="submit" value="Save Config"></div></form></div></main></body></html>)";

  // Replace placeholders with actual values
  configPage.replace("%SERVER_IP%", server_ip);
  configPage.replace("%SERVER_PORT%", String(server_port));
  configPage.replace("%TAG%", tag);
  configPage.replace("%WIFI_SSID%", wifi_ssid);
  configPage.replace("%WIFI_PASSWORD%", wifi_password);
  configPage.replace("%PANEL_USERNAME%", panel_username);
  configPage.replace("%PANEL_PASSWORD%", panel_password);
  configPage.replace("%SAMPLE_RATE_8000%", sample_rate == 8000 ? "selected" : "");
  configPage.replace("%SAMPLE_RATE_16000%", sample_rate == 16000 ? "selected" : "");
  configPage.replace("%SAMPLE_RATE_44100%", sample_rate == 44100 ? "selected" : "");
  configPage.replace("%SAMPLE_RATE_48000%", sample_rate == 48000 ? "selected" : "");
  configPage.replace("%BIT_RESOLUTION_16%", bit_resolution == 16 ? "selected" : "");
  configPage.replace("%BIT_RESOLUTION_32%", bit_resolution == 32 ? "selected" : "");
  return configPage;
}

String getLoginPage() {
  String page = R"(<!DOCTYPE html><html lang="en"><head><meta charset="UTF-8"><meta name="viewport" content="width=device-width, initial-scale=1.0"><title>Login</title><style>body{font-family:Arial,sans-serif;background-color:#121212;color:#ddd;margin:0;padding:0}header{background-color:#333;padding:10px;text-align:center}header a{color:#fff;margin:0 15px;text-decoration:none}header a:hover{color:#28a745}main{padding:20px}.form-group{margin-bottom:15px}label{display:block;font-size:14px;margin-bottom:5px}input[type="text"],input[type="password"]{width:100%;padding:10px;margin-top:5px;background-color:#333;border:1px solid #444;color:#ddd;box-sizing:border-box;font-size:14px}input[type="submit"]{width:100%;padding:10px;background-color:#28a745;border:none;cursor:pointer;color:white;font-size:16px}input[type="submit"]:hover{background-color:#218838}.container{max-width:600px;margin:0 auto;background-color:#222;padding:20px;border-radius:8px;box-sizing:border-box}</style></head><body><header><a href="/logout">Logout</a><a href="/config">Config</a></header><main><div class="container"><h2>Login</h2><form action="/login" method="POST"><div class="form-group"><label for="panel_username">Username</label><input type="text" id="panel_username" name="panel_username" placeholder="Enter Username" required></div><div class="form-group"><label for="panel_password">Password</label><input type="password" id="panel_password" name="panel_password" placeholder="Enter Password" required></div><div class="form-group"><input type="submit" value="Login"></div></form></div></main></body></html>)";
  return page;
}


// Create the web server for AP mode
void setupAPMode() {
  // Set the ESP32 to AP mode
  WiFi.softAP(ap_ssid, ap_password);

  // Define the handler for the login page
  serverAP.on("/", HTTP_GET, handleRootGetRequest);
  // Handle the POST request from the login form
  serverAP.on("/login", HTTP_GET, handleLoginGetRequest);
  // Handle the POST request from the login form
  serverAP.on("/login", HTTP_POST, handleLoginPostRequest);
  // Define the handler for the configuration page
  serverAP.on("/config", HTTP_GET, handleConfigGetRequest);
  // Handle the POST request from the configuration form
  serverAP.on("/config", HTTP_POST, handleConfigPostRequest);
  // Handle the logout request
  serverAP.on("/logout", HTTP_GET, handleLogoutGetRequest);

  serverAP.onNotFound(handleNotFoundRequest);

  // Start the server for AP mode
  serverAP.begin();
  Serial.println("AP Mode: Web server started");
}

void setupSTAMode() {
  // Set the ESP32 to STA mode and connect to the Wi-Fi network
  WiFi.begin(wifi_ssid, wifi_password);

  // Wait until connected
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());  // Print the local IP address
}

// Function to handle the /config request
void handleRootGetRequest(AsyncWebServerRequest* request) {
  // request->send(200, "text/html", "hello from root get");
  Serial.print("[GET] Request to [/] from ");
  Serial.println(request->client()->remoteIP());
  if (checkIPAddress(request->client()->remoteIP())) {
    request->redirect("/config");
  } else {
    request->redirect("/login");
  }
}

// Function to handle the /config request
void handleConfigGetRequest(AsyncWebServerRequest* request) {
  Serial.print("[GET] Request to [/config] from ");
  Serial.println(request->client()->remoteIP());
  if (checkIPAddress(request->client()->remoteIP())) {
    request->send(200, "text/html", getConfigPage());
  } else {
    request->redirect("/login");
  }
}

void handleConfigPostRequest(AsyncWebServerRequest* request) {
  Serial.print("[POST] Request to [/config] from ");
  Serial.println(request->client()->remoteIP());

  if (checkIPAddress(request->client()->remoteIP())) {
    // Get the parameters from the form
    if (request->hasParam("server_ip", true)) {
      server_ip = request->getParam("server_ip", true)->value();
      writeStringToEEPROM(4, server_ip);  // Save to EEPROM
    }
    if (request->hasParam("server_port", true)) {
      server_port = request->getParam("server_port", true)->value();
      writeStringToEEPROM(4 + STRING_SIZE, server_port);  // Save to EEPROM
    }
    if (request->hasParam("tag", true)) {
      tag = request->getParam("tag", true)->value();
      writeStringToEEPROM(4 + 2 * STRING_SIZE, tag);  // Save to EEPROM
    }
    if (request->hasParam("wifi_ssid", true)) {
      wifi_ssid = request->getParam("wifi_ssid", true)->value();
      writeStringToEEPROM(4 + 3 * STRING_SIZE, wifi_ssid);  // Save to EEPROM
    }
    if (request->hasParam("wifi_password", true)) {
      wifi_password = request->getParam("wifi_password", true)->value();
      writeStringToEEPROM(4 + 4 * STRING_SIZE, wifi_password);  // Save to EEPROM
    }
    if (request->hasParam("panel_username", true)) {
      panel_username = request->getParam("panel_username", true)->value();
      writeStringToEEPROM(4 + 5 * STRING_SIZE, panel_username);  // Save to EEPROM
    }
    if (request->hasParam("panel_password", true)) {
      panel_password = request->getParam("panel_password", true)->value();
      writeStringToEEPROM(4 + 6 * STRING_SIZE, panel_password);  // Save to EEPROM
    }
    if (request->hasParam("sample_rate", true)) {
      sample_rate = request->getParam("sample_rate", true)->value().toInt();
      writeIntToEEPROM(4 + 7 * STRING_SIZE, sample_rate);  // Save to EEPROM
    }
    if (request->hasParam("bit_resolution", true)) {
      bit_resolution = request->getParam("bit_resolution", true)->value().toInt();
      writeIntToEEPROM(4 + 7 * STRING_SIZE + 4, bit_resolution);  // Save to EEPROM
    }

    // Optionally, print out the saved values
    Serial.println("Configuration Saved:");
    Serial.println("Server IP: " + server_ip);
    Serial.println("Server Port: " + server_port);
    Serial.println("Tag: " + tag);
    Serial.println("WiFi SSID: " + wifi_ssid);
    Serial.println("WiFi Password: " + wifi_password);
    Serial.println("Panel Username: " + panel_username);
    Serial.println("Panel Password: " + panel_password);
    Serial.println("Sample Rate: " + String(sample_rate));
    Serial.println("Bit Resolution: " + String(bit_resolution));

    EEPROM.commit();  // Commit changes to EEPROM

    request->redirect("/config");
  } else {
    request->redirect("/login");
  }
}

// Function to handle the /config request
void handleLoginGetRequest(AsyncWebServerRequest* request) {
  Serial.print("[GET] Request to [/login] from ");
  Serial.println(request->client()->remoteIP());
  if (checkIPAddress(request->client()->remoteIP())) {
    request->redirect("/config");
  } else {
    request->send(200, "text/html", getLoginPage());
  }
}



// Function to handle the /config request
void handleLoginPostRequest(AsyncWebServerRequest* request) {
  Serial.print("[POST] Request to [/login] from ");
  Serial.println(request->client()->remoteIP());
  if (checkIPAddress(request->client()->remoteIP())) {
    request->redirect("/config");
  } else {
    if (request->hasParam("panel_username", true) && request->getParam("panel_username", true)->value() == panel_username && request->hasParam("panel_password", true) && request->getParam("panel_password", true)->value() == panel_password) {
      insertIPAddress(request->client()->remoteIP());
      Serial.println("Login successfully");
      request->redirect("/config");
    } else {
      Serial.println("Login Not successfully");
      request->redirect("/login");
    }
  }
}

// Function to handle the /config request
void handleLogoutGetRequest(AsyncWebServerRequest* request) {
  Serial.print("[GET] Request to [/logout] from ");
  Serial.println(request->client()->remoteIP());
  if (checkIPAddress(request->client()->remoteIP())) {
    removeIPAddress(request->client()->remoteIP());
  }
  request->redirect("/login");
}

// Function to handle the /config request
void handleNotFoundRequest(AsyncWebServerRequest* request) {
  Serial.print("[GET/POST] Request to [NOT FOUND] from ");
  Serial.println(request->client()->remoteIP());
  request->redirect("/login");
}




// Helper functions for writing and reading data
void writeStringToEEPROM(int address, String value) {
  int len = value.length();
  EEPROM.write(address, len); // Store string length at first byte
  for (int i = 0; i < len; i++) {
    EEPROM.write(address + 1 + i, value[i]); // Store each character
  }
}

String readStringFromEEPROM(int address) {
  int len = EEPROM.read(address); // Get string length from first byte
  String value = "";
  for (int i = 0; i < len; i++) {
    value += char(EEPROM.read(address + 1 + i)); // Read each character
  }
  return value;
}

void writeIntToEEPROM(int address, int value) {
  EEPROM.write(address, value >> 8);      // Write high byte
  EEPROM.write(address + 1, value & 0xFF); // Write low byte
}

int readIntFromEEPROM(int address) {
  int value = (EEPROM.read(address) << 8) | EEPROM.read(address + 1);
  return value;
}
