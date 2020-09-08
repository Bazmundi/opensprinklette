/*
  https://pegjs.org/ 
  Parser to parse text strings to help build mqtt topics for turning sprinklers on/off.   
  
  Parser works but needs to be integrated.
  
  The original idea was to set sprinkler zones by use of google calender event titles, so you just set the event time and period and the title tells the system
  with sprinklers to turn on (the event duration turns them off).  Another idea is a week calendar etc.
  
  For groups (quad relays) you can use commands like group1=1;group1=[1];group1=[2,2,2,4,4,4,4,1,1,1,3].  The stuttering is just to show that you can swallow silly typos.  
  You could add addition error checks (I suppose) to reject such sillyness.
  
  The following:
  
  group1=[1];unit1;group2=[1,3]
  
  At the moment, this is parsed to:
  
  [
   "group1/1",
   "unit1/1",
   "group2/1",
   "group2/3"
  ]
  
  The idea is the convert the group/unit ID to a chip ID in the process of parsing, so parser will need an array of gadget chipIDs as an input.  There is a default "options" parameter
  that the parser generator adds to the generated function so I assume that is accessed by inline code in the parser description (still needs to be tackled).
  
  I was playing with an auto register mechanism.  Each gadget heralds with:
  
  {
    gadgetType: 2
    gadgetId: 12678832
    online: 1
  }
  
  And the will is:
  
  {
    gadgetType: 2
    gadgetId: 12678832
    online: 0
  }
  
  
  The toy registration method was with node-red global arrays, but I am changing that to redis to allow for fiddling with Kotlin apps to access the info.
  
  The system at home is otherwise running on a rpi running hypriot with node-red, xmqtt, homeassistant and deCONZ.  With redis still to come.
  
  
*/

SPRINKLETTE_text

  = ws value:zones ws { return value; }

name_separator  = ws "=" ws
value_separator = ws "," ws
zone_separator = ws ";" ws
begin_array = ws "[" ws
end_array = ws "]" ws

ws "whitespace" = [ \t\n\r]*

zones

  = members:

    (

      head:zone

      tail:(zone_separator m:zone { return m; })*

      {

        var result = [];


        [head].concat(tail).forEach(function(element) {


          if (element.name == "unit") {
            var unitpush = element.name+element.value+"/1";
            if (result.includes(unitpush)==false) {
              result.push(unitpush);
            }
          }
          else {

            if (Array.isArray(element.value)) {
              var index = 0;
              element.value.forEach(
                function(subelement) 
                {
                  
                    var itempush = element.name+"/"+subelement;
                    if (result.includes(itempush)==false) {
                      result.push(itempush);
                    
                    
                  }  
                }
              );
            }
            else {

              if (element.value == "all") {

                var i;

                for (i = 1; i < 5 ; i++) {
                  var allpush = element.name+"/"+i;
                  if (result.includes(allpush)==false) {
                    result.push(allpush);
                  }
                }

              }
              else {
              
                var zonepush = element.name+"/"+element.value;
                if (result.includes(zonepush)==false) {
                  result.push();
                }

              }
            }
          }

        });

        return result;

      }

    )?

    {

      return (members.length != 0) ? members: {};   // trick is to be able to test for null object if array empty, to ease checking, otherwise return array

    }

zone
  = groupzone
  / groupzonearray
  / groupall
  / unitzone
  / lazyunitzone

groupzone
    = name:group name_separator value:group1_4
    {

      return { name: name , value: value };

    }
groupzonearray
  = name:group name_separator values:array
    {

      return { name: name, value: values };

    }

groupall
  = name:group name_separator all
    {

      return { name: name, value: "all" };

    }

unitzone
  = name:unit name_separator value:unit0_9
    {

      return { name: name, value: value };

    }
lazyunitzone
  = name:unit ws value:unit0_9
    {8n  

      return { name: name, value: value };

    }

group = head: "group" ws tail: group1_4 { return head + tail }
unit = "unit"
all = "all"

array
  = begin_array
    values:(
      head:group1_4
      tail:(value_separator v:group1_4 { return v; })*
      { return  [head].concat(tail) ; }
    )?
    end_array
    { return values !== null ? values : []; }

group1_4
  = [1-4]

unit0_9
  = [0-9]
