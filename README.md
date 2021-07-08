## Mycroft Battery Powered Satellite - ESP32 with I2S Microphone/Speaker



#### Status

Test the microphone with i2s-microphone-udp-stream.ino and i2s-microphone-udp-stream.py on the pc/raspberry pi.
You may have to open UDP port 18000 on your firewall.

Test audio out with the ESP8266Audio.zip library above. I tweaked AudioOuptutI2S.cpp to reflect the breadboard layout.

To use as the only microphone for a Raspberry Pi or PC with pulseaudio...

pactl load-module module-pipe-source source_name=virtmic file=/tmp/virtmic format=S32LE rate=16000 channels=1
set-default-source virtmic

socat -T 15 udp4-listen:18000,reuseaddr,fork stdout >> /tmp/virtmic

record with...
arecord -d 5 --device=pulse -r 44100 -c 1 -f S16_LE test.wav
ffmpeg -f pulse -i default -f mp3 test.mp3

### Parts

TinyPICO - ESP32 Development Board - V2

https://www.adafruit.com/product/4335

Adafruit I2S MEMS Microphone - SPH0645LM4H

https://www.adafruit.com/product/3421

Adafruit I2S 3W Class D Amplifier Breakout - MAX98357A

https://www.adafruit.com/product/3006

Mini Oval Speaker - 8 Ohm 1 Watt

https://www.adafruit.com/product/3923

Lithium Ion Polymer Battery - 3.7V 105mAh

https://www.adafruit.com/product/1570



### Code used from

https://github.com/earlephilhower/ESP8266Audio

https://github.com/atomic14

https://github.com/aleiei/ESP32-BUG-I2S-MIC

https://github.com/GrahamM/ESP32_I2S_Microphone
