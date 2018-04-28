#include <FS.h>
#include <ESP8266WiFi.h>
#include <SPI.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ESP8266HTTPUpdateServer.h>
#include <RobinhoodAPI.h>            //https://github.com/debsahu/RobinhoodAPI
                                     //https://github.com/bblanchon/ArduinoJson
#include <MD_Parola.h>               //https://github.com/MajicDesigns/MD_Parola
#include <MD_MAX72xx.h>              //https://github.com/MajicDesigns/MD_MAX72xx
//#include "secret.h"

#define USE_WIFIMANAGER
#ifdef USE_WIFIMANAGER
  #include <WiFiManager.h>           //https://github.com/tzapu/WiFiManager
#endif
// Define the number of devices we have in the chain and the hardware interface
#define MAX_DEVICES 8
#define CLK_PIN   D5
#define DATA_PIN  D7
#define CS_PIN    D8

#ifndef SECRET
  // Update these with values suitable for your network.
  const char* host = "stock-scroller";
  const char* update_path = "/firmware";
  const char* update_username = "admin";
  const char* update_password = "password";
  const char* ssid = "wifi_ssid";
  const char* password = "wifi_pass";
  String fp_robinhood_042018 = "8F C1 46 FB 19 0A 16 FF F7 D1 E6 48 5C 74 54 0E 00 FF 36 A6"; // update with latest fp from https://www.grc.com/fingerprints.htm
#endif

#define WIFI_TIMEOUT_DEF 30
#define MAX_MSG_LENGTH 255

char starting_msg[MAX_MSG_LENGTH] = "TSLA, AAPL, P, AMZN";
char http_msg[MAX_MSG_LENGTH];
char** stock_list;
uint16_t size_stock_list=0;
bool updatingStockList = false;
bool updateFS =false;

ESP8266WebServer httpServer(80);
ESP8266HTTPUpdateServer httpUpdater;
WiFiClientSecure sclient;                       // Connects to Robinhood using HTTPS
//RobinhoodAPI rh(sclient);                     // Declare API with no fingerprint check!
RobinhoodAPI rh(sclient, fp_robinhood_042018);  // Declare API with fingerprint check
MD_Parola P = MD_Parola(DATA_PIN, CLK_PIN, CS_PIN, MAX_DEVICES);

uint8_t scrollSpeed = 25;    // default frame delay value
textEffect_t scrollEffect = PA_SCROLL_LEFT;
textPosition_t scrollAlign = PA_LEFT;
uint16_t scrollPause = 10; // in milliseconds
uint8_t nStatus = 0;

// Global message buffers shared by Serial and Scrolling functions
#define  BUF_SIZE  75
char curMessage[BUF_SIZE] = { "" };
char newMessage[BUF_SIZE] = { "Stock Prices" };
bool newMessageAvailable = true;

char** processStockMsg(char* stock_str) {
  updatingStockList = true;
  char **store = NULL;
  char **tmp = NULL;
  const char delims[] = " ,.-";
  char* result = strtok(stock_str, delims);

  size_stock_list = 0;
  if (result != NULL) {
    store = (char**) malloc((size_stock_list + 1) * sizeof(char *));
    store[size_stock_list] = result;
    result = strtok (NULL, delims);
    size_stock_list++;
  }
  while (result != NULL) {
    free(tmp);
    tmp = (char**) malloc(size_stock_list * sizeof(char *));
    for (int i=0; i< size_stock_list; i++)
      tmp[i] = store[i];
    free(store);
    store = (char**) malloc((size_stock_list + 1) * sizeof(char *));
    for (int i=0; i<size_stock_list; i++)
      store[i] = tmp[i];
    store[size_stock_list] = result;
    size_stock_list++;
    result = strtok(NULL, delims);
  }
  free(tmp);
  updatingStockList = false;
  return store;
}

String getStockStr(void){
  String reply;
  for(int i=0; i<size_stock_list; i++)
    reply = reply + String(stock_list[i]) + ",";
  reply.remove(reply.length()-1);
  return reply;
}


char* getStockPrice(const char* stocksybl) {
  if(rh.getStockQuote(stocksybl)) {
    char stockinfo[MAX_MSG_LENGTH];
    double ltp = rh.lastTradePrice();
    double pd = rh.percentDiff();
    Serial.printf("%6s\t%8.2f\t%3.2f%%\n", stocksybl, ltp, pd);
    snprintf(stockinfo, sizeof(stockinfo), "%s %.2f %.2f%%", stocksybl, ltp, pd);
    return stockinfo;
  }
  return "Error";
}

void stockList(void) {
  strcpy(newMessage,getStockPrice(stock_list[nStatus]));
  //Serial.println(newMessage);
  nStatus = (nStatus + 1 == size_stock_list) ? 0 : nStatus + 1;
  newMessageAvailable = true;
}

String escapeString(String input_str)
{
  std::string output;
  std::string input = std::string(input_str.c_str(), input_str.length());
  output.reserve(input.length());
  for (std::string::size_type i = 0; i < input.length(); ++i)
  {
    switch (input[i]) {
      case '"':
          output += "&#34;";
          break;
      case '\'':
          output += "&#39;";
          break;
      case '&':
          output += "&amp;";
          break;
      case '<':
          output += "&lt;";
          break;
      case '>':
          output += "&gt;";
          break;
//      case '/':
//          output += "\\/";
//          break;
//      case '\b':
//          output += "\\b";
//          break;
//      case '\f':
//          output += "\\f";
//          break;
//      case '\n':
//          output += "\\n";
//          break;
//      case '\r':
//          output += "\\r";
//          break;
//      case '\t':
//          output += "\\t";
//          break;
//      case '\\':
//          output += "\\\\";
//          break;
      default:
          output += input[i];
          break;
    }
  }
  return String(output.c_str());
}

void getMsg() {
  String webpage;
  webpage =  "<html>";
  webpage += "<meta name='viewport' content='width=device-width,initial-scale=1' />";
   webpage += "<head><title>Stock Scroller</title>";
    webpage += "<style>";
     webpage += "body { background-color: #E6E6FA; font-family: Arial, Helvetica, Sans-Serif; Color: blue;}";
    webpage += "</style>";
   webpage += "</head>";
   webpage += "<body>";
    webpage += "<br>";  
    webpage += "<form action='/processmsg' method='POST'>";
     webpage += "<center><input type='text' name='msg_input' value='" + escapeString(getStockStr()) + "' placeholder='Stock Symbols' size='75' style='text-align:center;'></center><br>";
     webpage += "<center><input type='submit' value='Update Message'></center>";
    webpage += "</form>";
    webpage += "<br><center><a href='" + String(update_path) +"'>Update Firmware</a></center>";
   webpage += "</body>";
  webpage += "</html>";
  httpServer.send(200, "text/html", webpage); // Send a response to the client asking for input
}

void processMsg(){
  if (httpServer.args() > 0 and httpServer.method() == HTTP_POST) { // Arguments were received
    for ( uint8_t i = 0; i < httpServer.args(); i++ ) {
      Serial.print(httpServer.argName(i)); // Display the argument
      if (httpServer.argName(i) == "msg_input") {
        Serial.print(" Input received was: ");
        //Serial.println(httpServer.arg(i));
        //char http_msg[MAX_MSG_LENGTH];
        String tmp_rcv_arg = httpServer.arg(i);
        tmp_rcv_arg.toUpperCase();
        memcpy(http_msg, tmp_rcv_arg.c_str(), tmp_rcv_arg.length());
        Serial.println(http_msg);
        Serial.println((writeStateFS(http_msg)) ? "\nWrote Successfully" : "\nWriting Failure ^");
        stock_list = processStockMsg(http_msg);
      }
    }
  }
  
  String t;
  t += "<html>";
  t += "<head>";
  t += "<meta name='viewport' content='width=device-width,initial-scale=1' />";
  t += "<meta http-equiv='Content-Type' content='text/html; charset=utf-8' />";
  t += "</had>";
  t += "<body>";
  t += "<center><p>Updated</p></center>";
  t += "<br><center><a href='/'>Update again?</a></center>";
  t += "</form>";
  t += "</body>";
  t += "</html>";
  httpServer.send(200, "text/html", t);
}

#ifndef USE_WIFIMANAGER
void connectWiFiConfig(void) {
  Serial.println("\n"); Serial.print("Connecting to "); Serial.print(ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  int wifi_loops=0, wifi_timeout = WIFI_TIMEOUT_DEF;
  while (WiFi.status() != WL_CONNECTED) {
    wifi_loops++; Serial.print("."); delay(500);
    if (wifi_loops>wifi_timeout) software_reset();
  }
  Serial.println("connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}
#else
void connectWiFiManager() {
  WiFiManager wifiManager;
  if (!wifiManager.autoConnect(const_cast<char*>(host))) {
    Serial.println("failed to connect and hit timeout");
    delay(3000);
    //reset and try again, or maybe put it to deep sleep
    ESP.restart();
    delay(5000);
  }
  Serial.println("connected...yay :)");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}
#endif

void setup() {
  Serial.begin(115200);
  
  #ifndef USE_WIFIMANAGER
    connectWiFiConfig();  // Connect to WiFi
  #else
    connectWiFiManager(); // Connect to WiFi
  #endif

  P.begin();
  P.setInvert(false);
  P.setIntensity(9);
  P.displayText(curMessage, scrollAlign, scrollSpeed, scrollPause, scrollEffect, scrollEffect);

  //char starting_msg[MAX_MSG_LENGTH] = "TSLA, AAPL, P, AMZN";
  if(readStateFS(starting_msg)) {
    Serial.println("\nRead Successfully");
  } else {
    Serial.println("\nReading Failure ^");
    Serial.println((writeStateFS(starting_msg)) ? "\nWrote Successfully" : "\nWriting Failure ^");
  }
  Serial.println(starting_msg);
  stock_list = processStockMsg(starting_msg);
  
  //Set Robinhood Fingerprint Check
  //Not needed if already setting up API with Finger Print
  //rh.fpCheck(fp_robinhood_042018);

  MDNS.begin(host);

  httpUpdater.setup(&httpServer, update_path, update_username, update_password);
  httpServer.on("/", getMsg);
  httpServer.on("/processmsg", processMsg);
  httpServer.begin();

  MDNS.addService("http", "tcp", 80);
}

void loop() {
  httpServer.handleClient();
  if (WiFi.status() != WL_CONNECTED) software_reset();

  if (P.displayAnimate()) {
    if(!updatingStockList and !updateFS) stockList();
    if (newMessageAvailable) {
      strcpy(curMessage, newMessage);
      newMessageAvailable = false;
    }
    P.displayReset();
  }
}

void software_reset() { // Restarts program from beginning but does not reset the peripherals and registers
  Serial.println("resetting");
  ESP.restart(); 
}

bool writeStateFS(char* stockListF){
  updateFS = true;
  //save the strip state to FS JSON
  Serial.print("Saving cfg: ");
  DynamicJsonBuffer jsonBuffer(JSON_OBJECT_SIZE(7));
//    StaticJsonBuffer<JSON_OBJECT_SIZE(7)> jsonBuffer;
  JsonObject& json = jsonBuffer.createObject();
  json["stock_list"] = stockListF;

//      SPIFFS.remove("/stock_list.json") ? Serial.println("removed file") : Serial.println("failed removing file");
  File configFile = SPIFFS.open("/stock_list.json", "w");
  if (!configFile) {
    Serial.println("Failed!");
    updateFS = false;
    return false;
  }
  json.printTo(Serial);
  json.printTo(configFile);
  configFile.close();
  updateFS = false;
  return true;
  //end save
}

bool readStateFS(char* stockListF) {
  //read strip state from FS JSON
  updateFS = true;
  if (SPIFFS.begin()) {
    Serial.println("mounted file system");
    //if (resetsettings) { SPIFFS.begin(); SPIFFS.remove("/stock_list.json"); SPIFFS.format(); delay(1000);}
    if (SPIFFS.exists("/stock_list.json")) {
      //file exists, reading and loading
      Serial.print("Read cfg: ");
      File configFile = SPIFFS.open("/stock_list.json", "r");
      if (configFile) {
        size_t size = configFile.size();
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);
        configFile.readBytes(buf.get(), size);
        DynamicJsonBuffer jsonBuffer(JSON_OBJECT_SIZE(1)+500);
  //      StaticJsonBuffer<JSON_OBJECT_SIZE(1)+500> jsonBuffer;
        JsonObject& json = jsonBuffer.parseObject(buf.get());
        json.printTo(Serial);
        if (json.success()) {
          const char* tmp_stock_list = json["stock_list"];
          strncpy(stockListF, tmp_stock_list, MAX_MSG_LENGTH);
  
          updateFS = false;
          return true;
        } else {
          Serial.println("Failed to parse JSON!");
        }
      } else {
        Serial.println("Failed to open \"/stock_list.json\"");
      }
    } else {
      Serial.println("Coudnt find \"/stock_list.json\"");
    }
    //end read
  }
  updateFS = false;
  return false;
}
