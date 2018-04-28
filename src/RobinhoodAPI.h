#ifndef HARestAPI_h
#define HARestAPI_h

#include <Arduino.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>

class RobinhoodAPI
{
  public:
	RobinhoodAPI(WiFiClientSecure &client);
	RobinhoodAPI(WiFiClientSecure &client, String);
	void debugOn(void);
	void fpCheck(String);
	bool getStockQuote(String);
	double lastTradePrice(void);
	double percentDiff(void);

  private:
	WiFiClientSecure *wsclient;	
	JsonObject& sendGetReq(String, DynamicJsonBuffer&);
	
    String 
	  _server_add = "api.robinhood.com",
	  _fingerprint;
	  
	double
	  _last_trade_price,
	  _previous_close,
	  _percent_diff;
	  	
	bool
	  _debug = false,
	  _fp_chk = false;
	  
	uint16_t
	  _port = 443;
};

#endif