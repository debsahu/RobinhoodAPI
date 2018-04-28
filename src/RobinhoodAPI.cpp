#include "RobinhoodAPI.h"

RobinhoodAPI::RobinhoodAPI(WiFiClientSecure &client) {
	this->wsclient = &client;
}

RobinhoodAPI::RobinhoodAPI(WiFiClientSecure &client, String Fingerprint) {
	this->wsclient = &client;
	_fingerprint = Fingerprint;
	_fp_chk = true;
}

void RobinhoodAPI::debugOn(void) {
	_debug = true;
}

void RobinhoodAPI::fpCheck(String Fingerprint) {
	_fingerprint = Fingerprint;
	_fp_chk = true;
}

bool RobinhoodAPI::getStockQuote(String Stock) {
	String QString = "/quotes/" + Stock + "/";
	
	const size_t Capacity = JSON_OBJECT_SIZE(15) + 600;
	DynamicJsonBuffer replyRH(Capacity); // Allocate JsonBuffer
	JsonObject& processedReply = sendGetReq(QString, replyRH);

	if(processedReply.success()) {
	  _last_trade_price = processedReply["last_trade_price"];
	  _previous_close = processedReply["previous_close"];
	  return true;
	} else return false;
}

JsonObject& RobinhoodAPI::sendGetReq(String Request, DynamicJsonBuffer& jsonBuffer) {
	*wsclient = WiFiClientSecure();
	//Build URL
	String posturl = "GET  " + Request + " HTTP/1.1\r\n" +
                     "Host: " + _server_add + "\r\n" +
	                 "User-Agent: ESP8266 Rest API Client\r\n" +
                     "Accept: application/json\r\n\r\n";

	if(_debug) {  Serial.println(F("////////////// Sending ///////////////")); Serial.println(posturl); Serial.println(F("////////////////////////////////"));}
	
	//Connect to RobinHood
	wsclient->setTimeout(1000);
	if(_debug){ Serial.print(F("Connecting to ")); Serial.println(_server_add); }
	if (!wsclient->connect(_server_add.c_str(), _port)) {
	  if(_debug) Serial.println("connection failed");
	  return JsonObject::invalid();
	}
	
	//Check if SSL fingerprint matches
	if(_fingerprint.length() > 0 ) {
	  if (wsclient->verify(_fingerprint.c_str(), _server_add.c_str())) {
		if(_debug) Serial.println(F("Certificate matches"));
	  } else {
		if(_debug) Serial.println(F("Certificate doesn't match"));
		if(_fp_chk) return JsonObject::invalid();
	  }
	}
	wsclient->print(posturl);  //Send Data to Server
	if(_debug) Serial.println("HTTPS HA Request Sent!");
	
	// Check HTTP status
	char status[32] = {0};
	wsclient->readBytesUntil('\r', status, sizeof(status));
	if (strcmp(status, "HTTP/1.1 200 OK") != 0) {
		if(_debug) {
			Serial.print(F("Unexpected response: "));
			Serial.println(status);
		}
		return JsonObject::invalid();
	}
	
	// Skip HTTP headers
	if (!wsclient->find("\r\n\r\n")) {
		if(_debug) Serial.println(F("Invalid response"));
		return JsonObject::invalid();
	}

	// Parse JSON object
	JsonObject& root = jsonBuffer.parseObject(*wsclient);
	if (!root.success()) {
		if(_debug) Serial.println(F("Parsing failed!"));
		return JsonObject::invalid();
	}
	
	wsclient->stop();
	return root;
}

double RobinhoodAPI::lastTradePrice(void){
	return _last_trade_price;
}

double RobinhoodAPI::percentDiff(void) {
	_percent_diff = ((_last_trade_price - _previous_close) * 100 )/ _previous_close;
	return _percent_diff;
}