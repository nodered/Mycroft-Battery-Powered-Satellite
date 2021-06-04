import sys
import socket
import wave

UDP_IP = "0.0.0.0"
UDP_PORT = 18000

if len(sys.argv)>1:
    srate = int(sys.argv[1])
else:
    print("recording")
    srate=16000

WAVE_OUTPUT_FILENAME = "output_"+str(srate)+".wav"

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.bind((UDP_IP, UDP_PORT))

frames = b''

try:
    while True:
        data, addr = sock.recvfrom(1024) 
        frames = frames+data
    
except KeyboardInterrupt:  
    #print("Writing for rate ",srate)
    wf = wave.open(WAVE_OUTPUT_FILENAME, 'wb')
    wf.setnchannels(1)
    wf.setsampwidth(4)
    wf.setframerate(srate)
    wf.writeframes(frames)
    wf.close()
