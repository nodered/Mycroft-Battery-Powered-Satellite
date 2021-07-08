/*
pulseauido microphone setup on server/raspberry pi
  pactl load-module module-pipe-source source_name=virtmic file=/tmp/virtmic format=S32LE rate=16000 channels=1
  pactl set-default-source virtmic

listen on port 18000 and append data to pulseaudio fifo file
  socat udp-listen:18000,reuseaddr,fork stdout >> /tmp/virtmic
  test with...  cat /tmp/virtmic  ...if no data firewall config needed

arecord -d 5 --device=pulse -r 44100 -c 1 -f S16_LE test.wav
ffmpeg -f pulse -i default -f mp3 test.mp3

*/

#define Threshold 40

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include "AsyncUDP.h"
#include <driver/i2s.h>
#include <soc/i2s_reg.h>

// UDP
AsyncUDP udp;
IPAddress udpAddress(192, 168, 3, 201); // broadcast target
const int udpPort0 = 18000; // UDP Port:
boolean connected = false; // UDP State:

// HTTP/target server ip address with path
HTTPClient http;
const char* targetServer = "http://192.168.3.201:1880/cmds";

// device webserver port
WebServer server(1801);

// Wifi AP/password
const char* ssid = "*****";
const char* password = "*****";

// mic flags
int startMic = 0;
int stopMic = 0;

// i2s
const i2s_port_t I2S_PORT = I2S_NUM_0;
const int block_size = 128;


void setup() {
  Serial.begin(115200);

  // button setup/sleep
  touchAttachInterrupt(T0, callback, Threshold);

  WiFi.begin(ssid, password);

  while(WiFi.status() != WL_CONNECTED) {
    delay(100);
    Serial.print(".");
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("WiFi Connected");
    }
  }

  // i2s config
  esp_err_t err;

  i2s_config_t i2s_config = {
        mode: (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
        sample_rate: 16000,
        bits_per_sample: I2S_BITS_PER_SAMPLE_32BIT,
        channel_format: I2S_CHANNEL_FMT_ONLY_LEFT,
        communication_format: I2S_COMM_FORMAT_I2S,
        intr_alloc_flags: ESP_INTR_FLAG_LEVEL1,
        dma_buf_count: 8, //8-16 good
        dma_buf_len: 16
  };

  // configure pins for microphone
  i2s_pin_config_t pin_config = {
      .bck_io_num = 21, // BCLK pin
      .ws_io_num = 33, //  LRCK pin
      .data_out_num = I2S_PIN_NO_CHANGE,
      .data_in_num = 32   // DOUT
  };

  // install i2s driver and set pins
  err = i2s_driver_install(I2S_PORT, &i2s_config, 1, NULL);
  if (err != ESP_OK) {
      Serial.printf("Failed installing driver: %d\n", err);
      while (true);
  }

  err = i2s_set_pin(I2S_PORT, &pin_config);
  if (err != ESP_OK) {
      Serial.printf("Failed setting pin: %d\n", err);
      while (true);
  }
  Serial.println("I2S driver OK");

  // start server and http client
  setup_routing();
  http.begin(targetServer);


}// setup


// micrphone buffer and read mic function
int32_t buffer[512]; // Buffer
volatile uint16_t rpt = 0; // Pointer

void i2s_mic() {
    int num_bytes_read = i2s_read_bytes(I2S_PORT, (char*)buffer + rpt, block_size, portMAX_DELAY);
    rpt = rpt + num_bytes_read;
    if (rpt > 2043) rpt = 0;
}

// main
void loop() {

  server.handleClient();

  if(touchRead(T0) <= 30){
    delay(300);
    Serial.println("button - startMic");
    startMic = 1;
    httpOut();// send "startMic" to server
  }

      // read microphone data and send to UDP target
      while(startMic == 1){

        udp.connect(udpAddress, udpPort0);
        server.handleClient();

        static uint8_t state = 0;

        i2s_mic();


        // stop recording with button
        if(touchRead(T0) <= 30){
          delay(300);
          Serial.println("button - stopMic");
          startMic = 0;
          stopMic = 0;
          //postrecording();
          break;
        }

        // stop recording with msg from server
        if(stopMic == 1){
          Serial.println("server sent - stopMic");
          startMic = 0;
          stopMic = 0;
          //postrecording();
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

}// main


// api endpoint and server start
void setup_routing() {
  server.on("/cmds", HTTP_POST, handlePost);
  server.begin();
}

// handle incoming server messages
void handlePost() {
  if (server.hasArg("plain") == false) {
    // if post isn't plain text do nothing
  }

  String body = server.arg("plain");

  if(body == "stopMic"){
    stopMic = 1;
    server.send(200, "text/plain", "stopMic received");

  }

/*
  if(body == "serverMsg"){

    //serverMsg = 1;
    server.send(200, "text/plain", "serverMsg received");
  }
*/
}

// post to server
void httpOut(){

  http.addHeader("Content-Type", "text/plain");
  int httpCode = http.POST("startMic");

  if (httpCode > 0) {
    String payload = http.getString();
    Serial.println(payload);
  }
  http.end();

}
// callback for button
void callback(){
}
