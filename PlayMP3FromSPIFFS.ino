#include <Arduino.h>
#include <WiFi.h>
#include "SPIFFS.h"
#include "FS.h"

#include "AudioFileSourceSPIFFS.h"
#include "AudioGeneratorMP3.h"
#include "AudioOutputI2S.h"

void readFile(fs::FS &fs, const char * path){
    File file0 = fs.open(path);
    file0.close();
}

void setup(){
  WiFi.mode(WIFI_OFF); 
  Serial.begin(115200);
  delay(1000);
  SPIFFS.begin();
  Serial.printf("Sample MP3 playback begins...\n");

  readFile(SPIFFS, "/name-of-mp3-file.mp3");
  
  mp3playback();
  
}////

  AudioFileSourceSPIFFS *file0;
  AudioGeneratorMP3 *mp3;
  AudioOutputI2S *out = new AudioOutputI2S(0, 0, 32, 0);
  
void mp3playback(){
  
  file0 = new AudioFileSourceSPIFFS("/name-of-mp3-file.mp3");
  mp3 = new AudioGeneratorMP3();
  out = new AudioOutputI2S();
  mp3->begin(file0, out);
}

void loop()
{
  if (mp3->isRunning()) {
    if (!mp3->loop()) mp3->stop();
  } else {
    Serial.printf("MP3 done\n");
    delay(3000);
  }
}
