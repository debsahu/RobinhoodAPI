# RobinhoodAPI

An Arduino library to talk to [Robin Hood](https://robinhood.com/) using [non-documented Rest API](https://support.robinhood.com/hc/en-us/articles/210216823-Robinhood-API-Integrations) made for ESP8266. Some unofficial documentaion by @sanko is available at [here](https://github.com/sanko/Robinhood)

## Using the Library
* Download this GitHub [library](https://github.com/debsahu/RobinhoodAPI/archive/master.zip).
* In Arduino, Goto Sketch -> Include Library -> Add .ZIP Library... and point to the zip file downloaded.
* Install [ArduinoJSON](https://github.com/bblanchon/ArduinoJson) (mandatory: Reply from RobinHood is JSON) using the same process.

To use in your sketch include these lines.
```
#include <ESP8266WiFi.h>
#include <RobinhoodAPI.h>
```
Declare WiFiClientSecure and pass it to RobinhoodAPI.
```
String fp_robinhood_042018 = "8F C1 46 FB 19 0A 16 FF F7 D1 E6 48 5C 74 54 0E 00 FF 36 A6"; // update with latest fp from https://www.grc.com/fingerprints.htm

WiFiClientSecure sclient;
//RobinhoodAPI rh(sclient);                     // Declare API with no fingerprint check!
RobinhoodAPI rh(sclient, fp_robinhood_042018);  // Declare API with fingerprint check
```
Using the API to get live stock values.
```
String stocksybl = "TSLA";
if(rh.getStockQuote(stocksybl)) {
  Serial.println(stocksybl);
  Serial.printf("Latest Price  : %.2f\n", rh.lastTradePrice());
  Serial.printf("Percent Change: %.2f%%\n", rh.percentDiff());
}
```