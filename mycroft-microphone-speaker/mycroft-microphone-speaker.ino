#define Threshold 40

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include "AsyncUDP.h"
#include <driver/i2s.h>
#include <soc/i2s_reg.h>
#include "SPIFFS.h"
#include "FS.h"
#include <TinyPICO.h>

#include "AudioFileSourceHTTPStream.h"
#include "AudioFileSourceSPIFFS.h"
#include "AudioFileSourceBuffer.h"
#include "AudioGeneratorMP3.h"
#include "AudioOutputI2S.h"

TinyPICO tp = TinyPICO();
float GetBatteryVoltage();

AsyncUDP udp;
// Server to send microphone data to
IPAddress udpAddress(192, 168, 1, 100);
const int udpPort0 = 18000;// UDP port
boolean connected = false; // UDP state

// HTTPClient target server/Node-Red ip address with path
const char* targetServer = "http://192.168.1.100:1880/cmds";

// HTTP Server/Client
WebServer server(1801); //http://ipofesp32:1801/cmds
HTTPClient http;

// URL for playing sounds on target
const char *URL="http://192.168.1.100:8080/stream";

// Set youy WiFi network name and password:
const char* ssid = "*****";
const char* pswd = "*****";

// Bits to flip
int serverReady = 0;
int startMic = 0;
int stopMic = 0;
int startPlayback = 0;
int responseDone = 0;
int goToSleep = 0;

// Mic buffer
int32_t buffer[512];    // Buffer
volatile uint16_t rpt = 0; // Pointer
const int block_size = 128; // I2S read

// Mic I2S port
const i2s_port_t I2S_PORT = I2S_NUM_1;

// Warning/errors for esp8266audio (like a buffer underflow or decode hiccup)
void StatusCallback(void *cbData, int code, const char *string)
{
  const char *ptr = reinterpret_cast<const char *>(cbData);
  // Note that the string may be in PROGMEM, so copy it to RAM for printf
  char s1[64];
  strncpy_P(s1, string, sizeof(s1));
  s1[sizeof(s1)-1]=0;
  Serial.printf("STATUS(%s) '%d' = '%s'\n", ptr, code, s1);
  Serial.flush();
}


// Button setup + sleep
touch_pad_t touchPin;

void print_wakeup_touchpad(){
  touchPin = esp_sleep_get_touchpad_wakeup_status();

  switch(touchPin)
  {
    default  : Serial.println(""); break;
  }
}

// callback function for sleep
void callback(){
}


// Que up local files
void readFile(fs::FS &fs, const char * path){
    File file0 = fs.open(path);
    // File file1 = fs.open(path);
    file0.close();
}

// ESP8266audio
AudioGeneratorMP3 *mp3a; //start_listening.mp3
AudioGeneratorMP3 *mp3b; //response from server
AudioFileSourceHTTPStream *file;
AudioFileSourceSPIFFS *file0;
AudioFileSourceBuffer *buff;
AudioOutputI2S *out = new AudioOutputI2S();

RTC_DATA_ATTR int bootCount = 0;

void setup(){
  Serial.begin(115200);
  delay(100);

  ++bootCount;
  Serial.println("Boot number: " + String(bootCount));

  // Init filesystem/files
  SPIFFS.begin();
  readFile(SPIFFS, "/start_listening.mp3");

  // Setup button and interrupt on (GPIO4)
  print_wakeup_touchpad();
  touchAttachInterrupt(T0, callback, Threshold);
  esp_sleep_enable_touchpad_wakeup();

  WiFi.mode(WIFI_STA);

  WiFi.begin(ssid, pswd);
  while ( WiFi.status() != WL_CONNECTED ) {
    delay(100);
    Serial.print(".");
  }
  Serial.println("WiFi Connected!");


  // I2S microphone config
  //Serial.println("Configuring I2S port");
  esp_err_t err;

  i2s_config_t i2s_config = {
        mode: (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
        sample_rate: 16000,
        bits_per_sample: I2S_BITS_PER_SAMPLE_32BIT,
        channel_format: I2S_CHANNEL_FMT_ONLY_LEFT,
        communication_format: (i2s_comm_format_t)(I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_LSB),
        intr_alloc_flags: ESP_INTR_FLAG_LEVEL1,
        dma_buf_count: 8,
        dma_buf_len: 8
  };

  // Configure pins for microphone
  i2s_pin_config_t pin_config = {
      .bck_io_num = 21, // this is BCK pin
      .ws_io_num = 33, // this is LRCK pin
      .data_out_num = I2S_PIN_NO_CHANGE, // this is DATA output pin
      .data_in_num = 32   // DATA IN
  };

  // Load I2S driver.
  err = i2s_driver_install(I2S_PORT, &i2s_config, 1, NULL);
  if (err != ESP_OK) {
      //Serial.printf("Failed installing driver: %d\n", err);
      while (true);
  }

  err = i2s_set_pin(I2S_PORT, &pin_config);
  if (err != ESP_OK) {
      //Serial.printf("Failed setting pin: %d\n", err);
      while (true);
  }
  Serial.println("I2S driver OK");

  

  // Start webserver
  setup_routing(); // Start local server
  http.begin(targetServer);
  
  httpOut(); // Send startMic to server on wake
  
  
}// setup


// Main loop for broadcasting mic data
void loop(){

  server.handleClient();

  // Button press triggers startMic on server from testing
  if(touchRead(T0) <= 30){
    delay(300);
    Serial.println("button - sending startMic");
    httpOut();
  }

  // Wait for start_listening.mp3 to stop/then startMic
  while(serverReady == 1){

    server.handleClient();

    if (mp3a->isRunning()) {
    if (!mp3a->loop()) mp3a->stop();
    }
    else {
      if (startMic = 1);
      break;
    }
    break;
  }

  // recording loop
  while(startMic == 1){

    udp.connect(udpAddress, udpPort0);
    server.handleClient();

    static uint8_t state = 0;

    i2s_mic();

    // Stop recording with button from testing
    if(touchRead(T0) <= 30){
      delay(300);
      Serial.println("stop recording");
      stopMic = 1;
      postrecording();
      break;
    }

    // Stop recording with msg from server
    if(stopMic == 1){
      Serial.println("stop recording - msg from server");
      startMic = 0;
      postrecording();
      break;
    }


    if (!connected) {
        if (udp.connect(udpAddress, udpPort0)) {
            connected = true;
        }
    }

    else {
        switch (state) {
            case 0: // wait for index to pass halfway
                if (rpt > 1023) {
                state = 1;
                }
                break;
            case 1: // send the first half of the buffer
                state = 2;
                udp.write( (uint8_t *)buffer, 1024);
                break;
            case 2: // wait for index to wrap
                if (rpt < 1023) {
                    state = 3;
                }
                break;
            case 3: // send second half of the buffer
                state = 0;
                udp.write((uint8_t*)buffer+1024, 1024);
                break;
            }
        }

  }// recording loop

  delay(2);

}// main loop


// post recording clean up and play response
void postrecording(){

  delay(2);
  
  tp.DotStar_Clear();

  Serial.println("post recording");

  startPlayback = 1;

  // play mp3 stream/response
  while(startPlayback == 1){

    playRemoteMP3();

    while(1){

      server.handleClient();

      // ResponseDone msg from server
      if(responseDone == 1){
          startPlayback = 0;
          //http.end();
          goToSleep = 1;
          break;
        }

      if (mp3b->isRunning()) {
          if (!mp3b->loop()) mp3b->stop();
          }
          else {
          }

     }//play response

      while(goToSleep == 1){

        tp.DotStar_SetPower( false ); //needed for deep sleep
        WiFi.disconnect();
        WiFi.mode(WIFI_OFF);
        btStop();

        Serial.println("playback finished - go to sleep");
        delay(200);
        sleep();
      }

  }// startPlayback

}// postrecording()


// Read microphone data
void i2s_mic(){

    int num_bytes_read = i2s_read_bytes(I2S_PORT, (char*)buffer + rpt, block_size, portMAX_DELAY);
    rpt = rpt + num_bytes_read;
    if (rpt > 2043) rpt = 0;
}

// Play local mp3
void playSpiffsMP3(){
  file0 = new AudioFileSourceSPIFFS("/start_listening.mp3");
  mp3a = new AudioGeneratorMP3();
  out = new AudioOutputI2S();
  out -> SetGain(1.0);
  mp3a->begin(file0, out);
}

// Play remote mp3 stream
void playRemoteMP3(){

  HTTPClient http;

  file = new AudioFileSourceHTTPStream(URL);
  buff = new AudioFileSourceBuffer(file, 2048);
  out = new AudioOutputI2S();
  mp3b = new AudioGeneratorMP3();
  out -> SetGain(1.0);
  mp3b->begin(buff, out);

}

// Sleep
void sleep(){
  Serial.println("Going to sleep now");
  esp_deep_sleep_start();
}


// Endpoint and server start
void setup_routing() {
  server.on("/cmds", HTTP_POST, handlePost);
  server.begin();
}

// Handle POST data
void handlePost() {
  if (server.hasArg("plain") == false) {
    //not plain text/do nothing
  }

  String body = server.arg("plain");

  if(body == "stopMic"){
    Serial.println(body);
    stopMic = 1;
    server.send(200, "text/plain", "stopMic received");
  }

  if(body == "serverReady"){
    Serial.println(body);
    playSpiffsMP3();
    serverReady = 1;
    server.send(200, "text/plain", "serverReady received");
  }

    if(body == "startMic"){
    startMic = 1;
    tp.DotStar_SetPixelColor( 1, 10, 1 );
    server.send(200, "text/plain", "startMic received");
  }

    if(body == "responseDone"){
    Serial.println(body);
    responseDone = 1;
    server.send(200, "text/plain", "responseDone");
  }
}


    
// Send startMic to server responseDone
void httpOut(){

  http.addHeader("Content-Type", "text/plain");
  int httpCode = http.POST("startMic");
  Serial.println("http out sent");
  
  if (httpCode > 0) {
    String payload = http.getString();
    Serial.println(payload);
  }

  http.end();

}

void battvoltage(){

float parameter = tp.GetBatteryVoltage();
String myString = "";
myString.concat(parameter);

int boot = bootCount;
String myString2 = "";
myString2.concat(boot);

  http.addHeader("Content-Type", "text/plain");
  int httpCode = http.POST(myString);
  int httpCode2 = http.POST(myString2);
  
  if (httpCode > 0) {
    String payload = http.getString();
  }

  if (httpCode2 > 0) {
    String payload = http.getString();
  }

  http.end();
}
