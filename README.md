# opensprinklette
ESP8266 code for singe or quad wifi driven relays for sprinkler systems.

I am using node-red and emqx to control the gadets.  Though I have also set up to use ESPHome within Home Assistant (see wiki pages).  The Home Assitant is much easier to set up, but there are as many people interested in node-red etc.  

The aim is to let the main host do all the scheduling and leave the sprinkler on/off to a sub AUS$50 sprinkler-server.  Some of that $50 is on a board I designed that acts as a motherboard for the Wemos and shields.   That motherboard does the work to convert the 24VAC of the sprinkler system to board level DC.  That will be 9VDC for the quad board and 5VDC for the single.

The current boards look like this: https://organicmonkeymotion.wordpress.com/2020/01/03/final-opensprinklette-designs-for-now/

Other drivel at: https://organicmonkeymotion.wordpress.com/category/opensprinklette/

## Board types (before you need hack the code):  
```
  - Wemos D1 R2 "UNO" with DIYMORE quad relay shield (needs addition of pullup resistors)
  - Wemos D1 MINI using Wemos D1 MINI relay shield  
  - ESP8266 220V 10A DC 7-30V Network Relay WIFI Module with opto coupler (various sources)
```

See example of the third gadget option at: https://organicmonkeymotion.wordpress.com/2018/10/15/opensprinklette-single/

For single D1 MINI gadgets set defines thus:
```
#define SINGLE_UNIT
//#define QUAD_UNIT
//#define YUSHAN
```

For single generic gadgets, with optocoupler inputs, set defines thus:
```
#define SINGLE_UNIT
//#define QUAD_UNIT
#define YUSHAN
```

For quad gadgets set defines thus:
```
//#define SINGLE_UNIT
#define QUAD_UNIT
// #define YUSHAN
```

## MQTT topics that gadgets subscribe to:
```
/os/soundOff                  ; All gadgets that are online send their meta-data record.
/os/allStop                   ; All sprinkler relays on all gadgets (single or quad) turned off to stop the water.

/os/gadget/gadgetID()/relayID()/0|1/time=milliseconds  

                              ; The gadgetID() returns the ESP8266 unique ChipID.
                              ; The relayID() returns 1..4 for quad, 1 for single gadget.
                              ; 0="OFF: and 1="ON".  So either turn the associated sprinkler relayID() on or off.
                              ;
                              ; The time is how long the relay will be set for in milliseconds.
                              ; A time less than 5 minutes (in milliseconds) is ignored.
                              ;
                              ; A time longer than 1 hour is ignored.  If you want sprinklers on for more than 1hr then send
                              ; second on command after the first expires.  This is a safety feature so that if anything goes 
                              ; wrong, the sprinkler-server will finish the watering but will not empty the dam.
                              ;
                              ; Any time at end of "OFF" command is not used currently.
```


## MQTT topics that gadgets publish to:
```
/os/debug    ; Only used during development.
/os/lwt      ; Sends a gadget meta-data record with gadget reported offline (online flag set to 0="OFF").
/os/herald   ; A herald is sent on powerup, to provide the chipID and gadget type (1=Single, 2=Quad).
             ; The meta-data record is used with online flag set to 1="ON".
             
/os/gadget/gadgetID()/opto/0|1  ; Publish debounced opto input (YUSHAN single gadget)
```

# PROTOYPING base boards!
I am prototyping base boards for the Wemos D1 R2 "UNO" and Wemos D1 MINI.  Why?  Well you will need convert the 24VAC to 3.3VDC, 5VDC or 9VDC.  

There will also be a vanilla 24VAC to 3.3/5/9VDC board without headers for Wemos boards.

Not to mention you need handle the spikes on the 24VAC when the sprinkler solenoids close/open.  Still some debugging of the hardware ongoing.  Boards will come out on OSH Park once problems resolved.
