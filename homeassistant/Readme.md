# Home Assistant Snippets

So, these files are just a collection of code snippets from my HA work to provide some control over the opensprinklette gadgets.

HA is running as a docker image on top of Hypriot.  I did play with HASSIO for a little while but too many problems, I found at least, with how "invisible"
this configurtion approach is, the HA is bad enough to debug.

The home server is running HA, node-red, xmqtt and deCONZ at the moment.  I am going to add REDIS.   I did break the server by adding a clusterHAT and had to 
keep that a separate project.

My docker-compose.yml is included.

Any sprinklerX is repeated sprinkler1..4.   

I am currently only running a quad board but that will change over time.

# Images
**allStop.png:** shows how the links in docker-compose file allow connecting between tools over docker container names.

**timer_hmi.png:** shows what the sprinkler HMI looks like.  
- Entities Card is used for sun tools.  
- A Vertical Stack frames the Entites Cards for each of the four instant on/timed off functions.
- Button card for the allStop.

