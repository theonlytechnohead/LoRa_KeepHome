#include <WiFi.h>
#include <ESPmDNS.h>
#include <WebServer.h>
#include <DNSServer.h>
#include "TimeLib.h"

WebServer server(80);
DNSServer dnsServer;
WiFiUDP broadcast_udp;
char udpBuffer[255]; // buffer to hold incoming UDP packet
IPAddress ip;

void initNetwork () {
  if (checkFile("/ap_mode.txt")) {
    String ap_mode = readFile("/ap_mode.txt");
    if (ap_mode == "1") {
      printMessage("wifi", "Initializing AP mode");
      initAP();
    } else {
      printMessage("wifi", "Initializing client mode");
      initWiFi();
    }
  } else {
    writeFile("/ap_mode.txt", "1");
    printMessage("wifi", "Initializing AP mode");
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
    printMessage("wifi", "MDNS responder started: KeepHome.local");
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
  server.on("/post", HTTP_POST, handlePOST); // Call the 'handlePost' function when a client sends a POST request to URI "/post"

  server.begin();
  printMessage("web", "Webserver initialized");
  booted = true;
}

// Do POST (KeepHome APP)
void handlePOST () {
  printMessage("POST", "Got a POST request (from: " + server.client().remoteIP().toString() + ")");
}

// Utility functions
String getSSID () {
  if (checkFile("/ssid.txt")) {
    String ssid = readFile("/ssid.txt");
    return ssid;
  } else {
    writeFile("/ssid.txt", "KeepHome");
    return String("KeepHome");
  }
}

String getPassword () {
  if (checkFile("/password.txt")) {
    String password = readFile("/password.txt");
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
