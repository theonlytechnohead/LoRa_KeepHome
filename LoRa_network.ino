#include <WiFi.h>
#include <ESPmDNS.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <TimeLib.h>

WebServer server(80);
DNSServer dnsServer;
WiFiUDP broadcast_udp;
char udpBuffer[255]; // buffer to hold incoming UDP packet
IPAddress ip;

enum WiFiMode {
  Client,
  AP
};
enum WiFiMode currentMode;

void initNetwork () {
  if (checkFile("/ap_mode.txt")) {
    String ap_mode = readFile("/ap_mode.txt");
    ap_mode.trim();
    if (ap_mode == "1") {
      printMessage("wifi", "Initializing AP mode");
      currentMode = AP;
      initAP();
    } else {
      printMessage("wifi", "Initializing client mode");
      currentMode = Client;
      initWiFi();
    }
  } else {
    writeFile("/ap_mode.txt", "1");
    printMessage("wifi", "Initializing AP mode");
    currentMode = AP;
    initAP();
  }
}

// Cuz Arduino IDE is a dum-dum (incorrect ordering in auto-header file)
void onWiFiIP (WiFiEvent_t event, WiFiEventInfo_t info);

// Connect to WiFi part 1
void initWiFi () {
  WiFi.onEvent(onWiFiIP, WiFiEvent_t::SYSTEM_EVENT_STA_GOT_IP);
  String ssid = getSSID();
  String password = getPassword();
  printMessage("wifi", "Connecting to " + ssid + "...");
  WiFi.begin(ssid.c_str(), password.c_str());
}

// Connect to WiFi part 2 (connected)
void onWiFiIP (WiFiEvent_t event, WiFiEventInfo_t info) {
  ip = WiFi.localIP();
  initMDNS();
}

// Make WiFi
void initAP () {
  WiFi.mode(WIFI_AP);
  WiFi.softAP(getSSID().c_str(), getPassword().c_str());
  ip = WiFi.softAPIP();
  dnsServer.start(53, "*", ip);
  initMDNS();
}

// WiFi done, do MDNS, update UI, etc...
void initMDNS () {
  printMessage("wifi", "Connected to " + WiFi.SSID());
  printMessage("wifi", "IP address: " + ip.toString());
  if (MDNS.begin("KeepHome")) {
    printMessage("wifi", "mDNS responder started: KeepHome.local/");
  }
  MDNS.addService("_http", "_tcp", 80);
  broadcast_udp.begin(8874);
  printMessage("wifi", "UDP responder started");
  initModule(initNTP, "NTP");
  
  //drawUI();
  //getDisplay() -> display();
  
  initWebserver();
}

// Do Webserver
void initWebserver () {
  server.on("/", handleRoot);
  server.on("/post", HTTP_POST, handlePOST); // Call the 'handlePost' function when a client sends a POST request to URI "/post"
  server.on("/restart", restart);

  server.begin();
  printMessage("web", "Webserver initialized");
  booted = true;
}

void handleRoot () {
  printMessage("web", "Got web client from: " + server.client().remoteIP().toString());
  String index = readFile("/web/index.txt");
  server.send(200, "text/html", index); // Send HTTP status 200 (Ok) and send some HTML to the browser/client
}

// Do POST (KeepHome APP)
void handlePOST () {
  printMessage("POST", "Got a POST request from: " + server.client().remoteIP().toString());
  DynamicJsonDocument doc(1024); // Space for 1kb of JSON data in the heap

  doc["time"] = millis() / 1000;
  doc["additional"] = "Additional text!";

//  for (int i = 0; i < server.args(); i++) {
//    printMessage("POST", "  " + server.argName(i) + ": " + server.arg(i));
//  }
  // SSID
  if (server.hasArg("SSID")) {
    if (server.arg("SSID") == "get") {
      doc["SSID"] = WiFi.SSID();
    } else if (server.arg("SSID") == "set") {
      setSSID(server.arg("newWiFiSSID"));
      printMessage("POST", "  set new WiFi SSID: " + server.arg("newWiFiSSID"));
    }
  }

  // WifiMode
  if (server.hasArg("WiFiMode")) {
    if (server.arg("WiFiMode") == "get") {
      doc["WiFiMode"] = currentMode;
    } else if (server.arg("WiFiMode") == "set") {
      setWiFiMode(server.arg("newWiFimode"));
      if (server.arg("newWiFimode") == "1") {
        printMessage("POST", "  now acting as WiFi AP");
      } else {
        printMessage("POST", "  now acting as WiFi client");
      }
    }
  }

  // Password
  if (server.hasArg("password")) {
    if (server.arg("password") == "get") {
      doc["password"] = getPassword();
    } else if (server.arg("password") == "set") {
      setPassword(server.arg("newPassword"));
      printMessage("POST", "  set new WiFi password");
    }
  }

  char output[measureJson(doc) + 1]; // output buffer, needs + 1 to fix off-by-one length error (for null-terminator)
  serializeJson(doc, output, sizeof(output)); // change the document into proper json, and put it into the output
  server.send(200, "text/json", output); // send the output to the client as json text
}

// Utility functions
String getSSID () {
  if (checkFile("/ssid.txt")) {
    String ssid = readFile("/ssid.txt");
    ssid.trim();
    return ssid;
  } else {
    writeFile("/ssid.txt", "KeepHome");
    return String("KeepHome");
  }
}

void setSSID (String newSSID) {
  newSSID.trim();
  writeFile("/ssid.txt", newSSID);
}

void setWiFiMode (String newMode) {
  newMode.trim();
  writeFile("/ap_mode.txt", newMode);
}

String getPassword () {
  if (checkFile("/password.txt")) {
    String password = readFile("/password.txt");
    password.trim();
    return password;
  } else {
    String password;
    for (int i = 0; i < 8; i++) {
      password += '0' + random(10);
    }
    writeFile("/password.txt", password);
    return password;
  }
}

void setPassword (String newPassword) {
  if (newPassword.length() >= 8) {
    newPassword.trim();
    writeFile("/password.txt", newPassword);
  }
}

IPAddress getIP (String server) {
  IPAddress result;
  int error = WiFi.hostByName(server.c_str(), result) ;
  if(error != 1) {
    printMessage("wifi", "getIP() error: " + String(error));
  }
  return result;
}

// Returns a string with the local IP address
String getLocalIP () {
  return ip.toString();
}

void handleWebClient () {
  server.handleClient();
  int packetSize = broadcast_udp.parsePacket();
  if (packetSize) {
    String logMessage = "Received packet of size " + String(packetSize) + ", from " + broadcast_udp.remoteIP().toString() + ":" + String(broadcast_udp.remotePort());
    printMessage("web", logMessage);
    // read the packet into packetBufffer
    int bytes_read = broadcast_udp.read(udpBuffer, 255);
    if (bytes_read > 0) {
      udpBuffer[bytes_read] = 0; // set null terminator
    }
    printMessage("web", "Received: " + String(udpBuffer));
    // send a reply, to the IP address and port that sent us the packet we received
    broadcast_udp.beginPacket(broadcast_udp.remoteIP(), broadcast_udp.remotePort());
    char reply[] = "ACK";
    int i = 0;
    while (i < sizeof(reply) - 1) { // don't send null terminator
      broadcast_udp.write((uint8_t)reply[i++]);
    }
    broadcast_udp.endPacket();
  }
}

void disconnectWiFi () {
  WiFi.disconnect();
}

void endMDNS () {
  MDNS.end();
}

// Info functions
int getWiFiMode() {
  return currentMode;
}

String getNetworkName() {
  return WiFi.SSID();
}
