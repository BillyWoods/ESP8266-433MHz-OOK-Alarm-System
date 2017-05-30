A program for the ESP8266 which lets it read from a 433mhz OOK (on-off keying) 
radio receiver. This has been extended to have the ESP8266 recognise sensors 
from the ID they broadcast and serve a webpage showing which have been detected.
It also sends out email alerts when sensors are triggered while the alarm is 
"armed".

See more at: http://billysprojects.blogspot.com.au/2016/04/combining-esp8266-wifi-module-with.html
   and also: http://billysprojects.blogspot.com.au/2017/05/the-esp8266-alarm-can-now-send-emails.html

If you want to use this code, go ahead. I'll license my code (under user/) as CC BY.
However, the Makefile is the work of: zarya, Jeroen Domburg (Sprite_tm), 
Christian Klippel (mamalala) and Tommie Gannert (tommie).

If you want to replicate this project by uploading the code to an ESP8266, you'll
have to recompile it, because the binaries in this repo are not current. Also, 
you'll have to put your own WiFi credentials into user/wifi_config.h .

For details on the build environment/SDK: https://github.com/esp8266/esp8266-wiki/wiki/Toolchain
For a quick setup of the environment:     https://github.com/pfalcon/esp-open-sdk

![ESP8266](https://3.bp.blogspot.com/-dE7WjOzv1cs/VwY4SQu82lI/AAAAAAAAAZc/H-UoFv7PiBwHAU1FsUp97-En4VSBJ8Urw/s320/IMG_1321.JPG)

![Example Sensor](https://3.bp.blogspot.com/-X3tzRxuR268/VwY4ft80KPI/AAAAAAAAAZs/2E7DH_TVJM02qMYSRgKdYn7zI_TKqltZA/s320/IMG_1372.JPG)

![SDR analysis](https://2.bp.blogspot.com/-mxK08ZOS45w/VwY4iWwRy5I/AAAAAAAAAZ8/tFiu94m3OokjNhL08Jq6MyqATq4yWVKHg/s640/sdr%2B433.jpg)

![Screenshot of Web Interface](https://1.bp.blogspot.com/-_u8JuiHXa0Q/WS1XMfgPvKI/AAAAAAAAAus/9qquS1aSsF4wkIyNPq8FN1voG9WEIpq1QCLcB/s320/ss2.png)
