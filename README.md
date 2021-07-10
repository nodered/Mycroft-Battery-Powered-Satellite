## Mycroft Battery Powered Satellite - ESP32 with I2S Microphone/Speaker

### This configuration works if you are using the I2S microphone is your only microphone

#### Help needed for setting up a second microphone using 2-mic-pulseaudio.setup.sh



1.  Create Ramdisk - Needed for Raspberry Pi

    Add the following to /etc/fstab
    ```tmpfs /ramdisk tmpfs rw,nodev,nosuid,size=20M 0 0```
    
    ```sudo mkdir /ramdisk```
    
    ```sudo mount -a```


2. Set up Pulseaudio to encode incoming microphone data

    ```pactl load-module module-pipe-source source_name=virtmic file=/ramdisk/virtmic format=S32LE rate=16000 channels=1```

    ```pactl set-default-source virtmic```


3. Capture Microphone Data  

    Use socat to listen on UDP port 18000 and append data to the Pulseaudio FIFO file
    
    ```socat -T 15 udp4-listen:18000,reuseaddr,fork stdout >> /ramdisk/virtmic```


4. Set up MP3 Server
    
    The ESP32 is using the ESP8266audio libraries and uses the StreamMP3FromHTTP
    example sketch.
    
    Simple VLC MP3 server - Transcodes all system sounds to MP3 stream
    
    ```cvlc pulse://<your default sink name>.monitor   --sout="#transcode{vcodec=none,acodec=mp3,ab=128,channels=2,samplerate=44100}:http{mux=mp3,host=127.0.0.1,dst=:8080/stream}" --sout-keep```
    
    example...
    
   ```cvlc pulse://alsa_output.pci-0000_0a_00.6.analog-stereo.monitor --sout="#transcode{vcodec=none,acodec=mp3,ab=128,channels=2,samplerate=44100}:http{mux=mp3,host=127.0.0.1,dst=:8080/stream}" --sout-keep```

  
---

#### Tests for the hardware and software


Test the microphone with i2s-microphone-udp-stream.ino and i2s-microphone-udp-stream.py on the pc/raspberry pi.
You may have to open UDP port 18000 on your firewall.

Test audio out with the ESP8266Audio.zip library above. I tweaked AudioOuptutI2S.cpp to reflect the breadboard layout.

To use as the only microphone for a Raspberry Pi or PC with pulseaudio...

The wifi-button-microphone-stream folder has the sketch for the ESP32 if you want to
add a button to start and stop the microphone stream and a bare bones node-red flow

```pactl load-module module-pipe-source source_name=virtmic file=/tmp/virtmic format=S32LE rate=16000 channels=1```

```pactl set-default-source virtmic```

```socat -T 15 udp4-listen:18000,reuseaddr,fork stdout >> /tmp/virtmic```

record with...

```arecord -d 5 --device=pulse -r 44100 -c 1 -f S16_LE test.wav```

```ffmpeg -f pulse -i default -f mp3 test.mp3```

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
