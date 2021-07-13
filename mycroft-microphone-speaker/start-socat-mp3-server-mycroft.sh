#!/bin/sh
socat -T 15 udp4-listen:18000,reuseaddr,fork stdout >> /ramdisk/virtmic &
sleep 1
cvlc pulse://alsa_output.platform-bcm2835_audio.analog-stereo.monitor --sout="#transcode{vcodec=none,acodec=mp3,ab=128,channels=2,samplerate=44100}:http{mux=mp3,host=127.0.0.1,dst=:8080/stream}" --sout-keep &
sleep 1
/home/pi/mycroft-core/start-mycroft.sh all
