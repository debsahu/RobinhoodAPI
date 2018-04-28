#include <ESP8266WiFi.h>
#include <RobinhoodAPI.h>
//#include "secret.h"

#ifndef SECRET
  // Update these with values suitable for your network.
  const char* ssid = "wifi_ssid";
  const char* password = "wifi_pass";
  String fp_robinhood_042018 = "8F C1 46 FB 19 0A 16 FF F7 D1 E6 48 5C 74 54 0E 00 FF 36 A6"; // update with latest fp from https://www.grc.com/fingerprints.htm
#endif

#define WIFI_TIMEOUT_DEF 30
WiFiClientSecure sclient;                       // Connects to Robinhood using HTTPS
//RobinhoodAPI rh(sclient);                     // Declare API with no fingerprint check!
RobinhoodAPI rh(sclient, fp_robinhood_042018);  // Declare API with fingerprint check

void setup() {
  Serial.begin(115200);

  // Connect to WiFi
  Serial.println(); Serial.print("Connecting to "); Serial.print(ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  int wifi_loops=0, wifi_timeout = WIFI_TIMEOUT_DEF;
  while (WiFi.status() != WL_CONNECTED) {
    wifi_loops++; Serial.print("."); delay(500);
    if (wifi_loops>wifi_timeout) software_reset();
  } Serial.println("connected");

  //Set Robinhood Fingerprint Check
  //Not needed if already setting up API with Finger Print
  //rh.fpCheck(fp_robinhood_042018);
  
  //Get Stock price
  String stocksybl = "TSLA";
  if(rh.getStockQuote(stocksybl)) {
    Serial.println(stocksybl);
    Serial.printf("Latest Price  : %.2f\n", rh.lastTradePrice());
    Serial.printf("Percent Change: %.2f%%\n", rh.percentDiff());
  }
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) software_reset();
}

void software_reset() { // Restarts program from beginning but does not reset the peripherals and registers
  Serial.println("resetting");
  ESP.restart(); 
}
