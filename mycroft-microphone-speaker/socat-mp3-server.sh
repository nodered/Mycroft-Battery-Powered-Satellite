#!/bin/bash
socat -T 15 udp4-listen:18000,reuseaddr,fork stdout >> /tmp/virtmic &

cvlc pulse://alsa_output.platform-bcm2835_audio.analog-stereo.monitor --sout="#transcode{vcodec=none,acodec=mp3,ab=128,channels=2,samplerate=44100}:http{mux=mp3,host=127.0.0.1,dst=:8080/stream}" --sout-keep &
