# opensprinklette
ESP8266 code for singe or quad wifi driven relays for sprinkler systems.

I am using node-red and emqx to control the gadets.

The aim is to let the main host do all the scheduling and leave the sprinkler on/off to a sub AUS$50 sprinkler-server.

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
/os/gadgetID()/relayID()/0|1  ; The gadgetID() returns the ESP8266 unique ChipID.
                              ; The relayID() returns 1..4 for quad, 1 for single gadget.
                              ; 0="OFF: and 1="ON".  So either turn the associated sprinkler relayID() on or off.
```


## MQTT topics that gadgets publish to:
```
/os/debug    ; Only used during development.
/os/lwt      ; Sends a gadget meta-data record with gadget reported offline (online flag set to 0="OFF").
/os/herald   ; A herald is sent on powerup, to provide the chipID and gadget type (1=Single, 2=Quad).
             ; The meta-data record is used with online flag set to 1="ON".
             
/os/gadgetID()/opto/0|1  ; Publish debounced opto input (YUSHAN single gadget)
```

# PROTOYPING base boards!
I am prototyping base boards for the Wemos D1 R2 "UNO" and Wemos D1 MINI.  Why?  Well you will need convert the 24VAC to 3.3VDC, 5VDC or 9VDC.  

There will also be a vanilla 24VAC to 3.3/5/9VDC board without headers for Wemos boards.

Not to mention you need handle the spikes on the 24VAC when the sprinkler solenoids close/open.  Still some debugging of the hardware ongoing.  Boards will come out on OSH Park once problems resolved.
