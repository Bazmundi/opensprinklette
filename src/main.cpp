/*
main.cpp - Opensprinklette ESP8266 sprinkler client
Copyright (C) 2019 Asterion Daedalus (aka Bazmundi)

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

// ------------------------------------------------------------------------------------------ //
//                                                                                            //
// Board types (before you need hack the code):                                               //
//  - Wemos D1 R2 "UNO" with DIYMORE quad relay shield (needs addition of pullup resistors)   //
//  - Wemos D1 MINI using Wemos D1 MINI relay shield                                          //
//  - ESP8266 220V 10A DC 7-30V Network Relay WIFI Module with opto coupler (various sources) //
//                                                                                            //
// Open Source libraries used (other than frameworks provided by PlatformIO):                 //
//  - https://github.com/Imroy/pubsubclient                                                   //
//                                                                                            //
// Coded and built using:                                                                     //
//                                                                                            //
//   Visual Code Studio                                                                       //
//   Version: 1.29.0                                                                          //
//   Commit: 5f24c93878bd4bc645a4a17c620e2487b11005f9                                         //
//   Date: 2018-11-12T07:42:27.562Z                                                           //
//   Electron: 2.0.12                                                                         //
//   Chrome: 61.0.3163.100                                                                    //
//   Node.js: 8.9.3                                                                           //
//   V8: 6.1.534.41                                                                           //
//   Architecture: x64                                                                        //
//                                                                                            //
//   PlatformIO: Home 2.0.0·Core 3.6.3                                                        //
//                                                                                            //
// ------------------------------------------------------------------------------------------ //

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <string.h>

// Use one define or the other.  Not both.
//#define SINGLE_UNIT
#define QUAD_UNIT

// Update these with values suitable for your network.
const char* ssid        = "xxx";  // Put your wifi router ssid here.
const char* password    = "yyy";   // Put your wifi router password here.

// MQTT centric variables
IPAddress MQTTserver(192,168,0,100);  // Put your mqtt server IP address here.
WiFiClient wclient;
PubSubClient client(wclient, MQTTserver);  // using default port 1883

#ifdef SINGLE_UNIT
unsigned long lastOnMsg[] = {0}; // Time in milliseconds from milli(). One for single relay unit.
#else
unsigned long lastOnMsg[] = {0,0,0,0}; // Time in milliseconds from milli(). Four for a quad relay group
#endif


// relay management constants
const unsigned long onehr           = 3600000;  // In milliseconds.  Fail safe since water is expensive.
const unsigned long fivemin            = onehr / 12;
const unsigned long fifteenmin      = onehr / 4;
const unsigned long onemin          = onehr / 60;
const unsigned long watchdog        = onehr + fivemin;  // not currently used

#ifdef SINGLE_UNIT
// platformio.ini:
// [env:d1_mini]
// platform = espressif8266
// board = d1_mini
// framework = arduino

// #define YUSHAN
// Use this if single is a IoT Yunshan (or similar) ESP8266 250V 10A AC/DC WIFI Network Relay Module
// Good info at:
// https://ucexperiment.wordpress.com/2016/12/18/yunshan-esp8266-250v-15a-acdc-network-wifi-relay-module/

#ifdef YUSHAN
const int  relays[]        = {D2};          // Yushan single relay board pin uses D2 pin.
const int OPTOCOUPLER      = D1;            // Optically coupled input.
      int OPTOSTATE        = -1;            // State will be 0 or 1 except at boot (since we
                                            // wont know).
#else
const int  relays[]        = {D1};          // Wemos D1 mini single relay board pin uses D1 pin.
#endif

                                            // Use LED_BUILTIN for debugging, outherwise point
                                            // at GPIO port connected to relay.
#else
// platformio.ini:
// [env:d1_mini]
// platform = espressif8266
// board = d1_mini
// framework = arduino

const int  relays[]        = {D2,D3,D4,D5}; // When using a DIYMORE four relay board for UNO,
                                            // Wemos mini rely board pin uses D2, D3, D4 and D5 pins.
                                            // Use LED_BUILTIN for debugging, outherwise point
                                            // at GPIO port connected to relay.
#endif

// Make code more readable.
const int RELAY_ON = HIGH;
const int RELAY_OFF = LOW;

#ifdef SINGLE_UNIT
unsigned long sprinklerOnTime[] = {0}; // Needs one entry for every entry in relays[].
                              // Used also as relay ON/OFF "flag".
#else
unsigned long sprinklerOnTime[] = {0,0,0,0}; // Needs one entry for every entry in relays[].
                                    // Used also as relay ON/OFF "flag".
#endif

// root of topics
String OpenSprinklette = "/os";

// global listen on topics
String SoundOff        = OpenSprinklette + "/soundOff";
String AllStop         = OpenSprinklette + "/allStop";

// global talk on topics
String Debug           = OpenSprinklette + "/debug";
String LastWill        = OpenSprinklette + "/lwt";
String Herald          = OpenSprinklette + "/herald";

// workers

String gadgetID (void) {

  return String(ESP.getChipId());
}

String gadget ( void ) {
  
  return OpenSprinklette + "/gadget/" + gadgetID();
}

#ifdef YUSHAN

String optoTopic ( void ) {
  
  return gadget() + "/opto";
}

int debounceOpto (void) {
 
  if ( digitalRead( OPTOCOUPLER ) == HIGH ) {
    
    delay( 25 );
    if ( digitalRead( OPTOCOUPLER ) == HIGH )
      return HIGH;
  }
  return LOW;
}

void optoPublish ( void ) {

  int hilo = debounceOpto();
  
  if (OPTOSTATE < 0) {  // First pulse after startup, so just catch state.
    OPTOSTATE = hilo;
  } else {

    if (OPTOSTATE != hilo) {  // Just report on changes.
                              // The node-red code will have to know whether is is a rain gauge
                              // or a low rate flow sensor.
      OPTOSTATE = hilo;
      
      client.publish(optoTopic(), OPTOSTATE == 0 ? "0":"1");
    }
  }
}

#endif

String relay ( int relayID ) {

  return gadget() + "/" + relayID;
}

String relayState ( int relayID, char onoff) {

  return relay(relayID) + "/" + onoff;
}

void debugOut (String ouch) {

  client.publish(Debug, ouch);
}

String gadgetTypeIDVal (void) {

   return ((sizeof(relays)/sizeof(int)) == 1) ? String("1") : String("2");
}

String gadgetMetaData ( String onlineFlag ) {

  return  "{ \"gadgetType\": " + gadgetTypeIDVal() + "," +
            "\"gadgetId\": "   + gadgetID()        + "," +
            "\"online\": "     + onlineFlag        + "}";
}

String gadgetTypeVal (void) {

   return ((sizeof(relays)/sizeof(int)) == 1) ? String("unit: ") : String("group: ");
}

void heraldMessage (void) {
  
#ifdef YUSHAN
  String heraldDebugString = gadgetTypeVal() + gadgetID() + String(" with OPTOCOUPLER input online!");
#else
  String heraldDebugString = gadgetTypeVal() + gadgetID() + String(" online!");
#endif

  // As per story with lwt.  If all is good, this gets through to node-red graph first, and
  // a gadget entry will be made in global list - with gadget online!
  String heraldString = gadgetMetaData("1");


  // Why two separate topics? well one is for comfort on the debug console. 
  // The other is for pumping into node-red graph for automation aspects. 
  // Yes, if you where really clever you could parse debug string.

  // Debug stream can be lifted out if there is a preference for a quiet debug channel.  The more 
  // important aspect is the lwt in any event.  That should as likely go to email or sms to warm
  // system owner that there is a problem.  Herald on debug is being kept until a distributed
  // key value store is set up, with automated assignment of gadgets to unit or group ID at
  // user end.
  client.publish(Debug, heraldDebugString);
  client.publish(Herald, heraldString);
}

void registerGlobalListenOnTopics (void) {

  client.subscribe(SoundOff.c_str());
  client.subscribe(AllStop.c_str());
}

int relayCount (void) {
  
  return sizeof(relays)/sizeof(int);
}

void registerRelayTopics (void) {
 
  for (int i = 0; i < relayCount(); i++) {

    client.subscribe(relayState(i+1,'1').c_str());  // Listen for relays[i]=ON.
    client.subscribe(relayState(i+1,'0').c_str());  // Listen for relays[i]=OFF.
  }
}

int soundOffTopic (String topic) {

  if (SoundOff == topic) {
    
    heraldMessage(); 

    return true;
  } 

  return false;
}

void setSprinklerStateOFF(int relayID) {  // don't call without index check

   digitalWrite(relays[relayID],RELAY_OFF);
   sprinklerOnTime[relayID] = 0;  // Used also as relay ON/OFF "flag".
   lastOnMsg[relayID] = 0;  
}

void allStop (void) {
   
  for (int i = 0; i < relayCount(); i++) {

    setSprinklerStateOFF(i);
  }
}

int allStopTopic (String topic) {

  if (AllStop == topic) {
 
    allStop();

    return true;
  } 
  
  return false;
}

long payloadToTime(byte* payload, unsigned int length) {

  long result = 0;  // If all else fails.  Do not turn sprinkler on.  

  if (not (length > 7)) { // Payload is not simply too long to start with.
    
    String resultString = "";

    for (unsigned int i=0; i<length; i++) {

      resultString += (char)payload[i];
    }

    unsigned long temp = (unsigned long)resultString.toInt();  // If it is not a valid number it returns 0 so all good.

    if (temp>onehr) {

      debugOut(gadgetID() + ": Attempted to run sprinkler for more than 1hr, capped at 1hr.");
    }

    result = (temp > onehr) ? onehr : temp;
  } 
  else {

    debugOut(gadgetID() + ": Too large a payload, ignored.");
  }

  return result; // 0 if less than 5 mins, more than an hour, or invalid number.
                 // Otherwise a valid time (in milliseconds) up to an hour.
}

void setSprinklerStateOn (int relayID, long time, long minimumTime) { 
  // @assert(relayID in [0] or [0,1,2,3] as checked by caller)
  //     0 for units, between 0 and 3 inclusive for quad boards
  //     indexed arrays set to 1 or 4 cells at compile time using switches
  // @assert(time > minimum and < value set by caller then keep)
  //     Capped by caller currently at 1hr (in milliseconds)
  // @assert(time < minimum & element in [1,2,3,4] then keep) 
  // @assert(time < minimum & element in [15,30,45,60] then keep) 

  int valid = 1; // either time > minimum or one of [1,2,3,4] or [15,30,45,60]

  if (time>=minimumTime) { // set time in milliseconds > minimum
    sprinklerOnTime[relayID] = time;

  } else { // less than minimumTime in milliseconds

    // allow either 1..4*15 or literally 15 minute increments
    
    switch (time) { // set on time in equivalent milliseconds
      case 1 : 
      case 15: 
        sprinklerOnTime[relayID] = fifteenmin;
        break;

      case 2 :
      case 30:
        sprinklerOnTime[relayID] = fifteenmin * 2;
        break;

      case 3 :
      case 45:
        sprinklerOnTime[relayID] = fifteenmin * 3;
        break;

      case 4 :
      case 60:
        sprinklerOnTime[relayID] = onehr;
        break;

      default: 
        valid = 0;
        break;
    }
  }

  if (valid) {
    digitalWrite(relays[relayID],RELAY_ON);
    lastOnMsg[relayID] = millis();
  }
}

void relayActionTopic (const MQTT::Publish& pub) {

  // Index checks are likely over the top, since we only register for specific topics by relay index.
  // This is otherwise just defensive programmming.  Remove checks if you are inclined.  However, this
  // uses a third MQTT library since bugs in the other two were throwing up garbage, who knows also
  // with a distributed system between the mqtt server, node-red and wemos libraries were bugs'll 
  // pop up.

  int relayIndexAt = pub.topic().length() - 3; // we have to skip "/1" and "/0" for onoff at end of topic
  char relayIndexChar = pub.topic().charAt(relayIndexAt);

  if ((relayIndexChar < '1') | (relayIndexChar > '4')) {  

    debugOut(gadgetID() + ": Unrecognised relay index");
  } 
  else {

    int relayIndex = relayIndexChar - '1';
    int expectedIndices = sizeof(relays)/sizeof(int); // should be 1 or 4

    if (relayIndex<expectedIndices) { // should either be 1 or 4 indicies 
                                      // (0 for units or 0..3 for groups)
      // check for ON or OFF
      if (pub.topic().endsWith("/1")) {

        setSprinklerStateOn(relayIndex,payloadToTime(pub.payload(),pub.payload_len()),fivemin);
      }
      else if (pub.topic().endsWith("/0")) { // defensive, just in case something other than "0" sneaks in

        setSprinklerStateOFF(relayIndex);
      } else {

        debugOut(gadgetID() + ": Unrecognised onoff flag");
      }
    }
    else {

      debugOut(gadgetID() + ": Wrong relay index for unit");
    }
  }
}

#ifdef YUSHAN

void setupOpto(void) {

  pinMode(OPTOCOUPLER,INPUT);
}

#endif

void setupRelays(void) {
 
  for (int i = 0; i < relayCount(); i++) {

    pinMode(relays[i], OUTPUT);
    setSprinklerStateOFF(i);
  }
}

// The WEMOS D1 R2 UNO "compatable" (not really) board plus quad relay board 
// will use LED_BUILTIN as a relay port on D4.

// No biggy since the quad relay board will have on board led per relay, not
// to mention the click of the relay.  

// This also means LED_BUILTIN will follow relay on D4.  Note also, LED will be ON when
// relay D4 is OFF, and OFF when relay D4 is ON.  Side effect of wiring of WEMOS versus
// quad relay board.

#ifndef QUAD_UNIT
void setupLED(void) {

  pinMode(LED_BUILTIN,OUTPUT);
}

void setLED (void) {  // Might make more sense if  were not using the WEMOS D1 R2,
                      // which gives up the built in LED to also drive a relay, if a quad
                      // relay shield is used.

                      // Kept code on the off change some other board and shield combo is
                      // used and the built in LED port is not also driving a relay.

  int LEDFlag = 0;

  for (int i = 0; i < relayCount(); i++) { 

    LEDFlag = LEDFlag or (sprinklerOnTime[i]>0);
  }

  digitalWrite(LED_BUILTIN, LEDFlag ? LOW : HIGH);  // If at least one relay on, set LED on.
}

#endif

void timeOutRelays(unsigned long time) {
 
  for (int i = 0; i < relayCount(); i++) {

    if (not sprinklerOnTime[i]==0) {
       if (time - lastOnMsg[i] >= sprinklerOnTime[i]) { // This actually deals with milli wrap around,
                                                        // BECAUSE of the way unsigned long math works.
                                                        // https://oshlab.com/handle-millis-overflow-arduino/
      
         setSprinklerStateOFF(i);
       }
    }
  }
}

void setup_wifi() {

  delay(10);

  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}


void callback(const MQTT::Publish& pub) {

  if (not allStopTopic(pub.topic())) {
    if (not soundOffTopic(pub.topic())) {

      relayActionTopic(pub); 
    }
  }
}

void reconnect() {

  // loop until we're reconnected
  while (!client.connected()) {

    Serial.print("Attempting MQTT connection...");

    // create a client ID from chip id
    String clientId = "ESP8266Client:";
    clientId += gadgetID();

    // last will and testament
    String finalWords = gadgetMetaData("0");      // MQTT server will likely blurt this out during wemos boot.
                                                  // Ignore message during wemos boot, just a quirk
                                                  // of a distributed system and the timing.
                                                  // Take heed post wemos boot that something is wrong.
                                                  // The idea, in any event, even if this comes through
                                                  // before herald, an entry for the device will be
                                                  // created - defaulting to offline.

    // attempt a connection
    if (client.connect(clientId.c_str(),LastWill.c_str(),2,true,finalWords.c_str())) {

      Serial.println("connected");

      // once connected, publish an announcement
      heraldMessage();

      // resubscribe
      registerGlobalListenOnTopics();
      registerRelayTopics();
    }
    else {

      Serial.print("failed, try again in 5 seconds");

      // wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup() {

#ifdef YUSHAN
  setupOpto();
  OPTOSTATE = -1;
#endif

#ifndef QUAD_UNIT
  setupLED();
#endif

  setupRelays();     
  Serial.begin(115200);
  setup_wifi();
  client.set_callback(callback);
}

void loop() {

  if (!client.connected()) {

    allStop();  // Lost connection with server so stop all sprinklers.
    reconnect();
  }

  client.loop();  // Topics will set sprinklers on with associated timeouts.

  timeOutRelays(millis());  // Sprinklers timeout as necessary.

#ifdef YUSHAN
  optoPublish();
#endif

#ifndef QUAD_UNIT
  setLED();
#endif

}
