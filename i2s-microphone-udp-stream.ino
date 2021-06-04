/*
   
    Tinypico(ESP32) I2S MEMS Microphone Audio Streaming via UDP

    Needs a UDP listener on listener PC

    To test the broadcast on the target use...
    netcat -u -p 18000 -l

    To record a wav file on the target use this...
    python receive.py
    

    TinyPICO - ESP32 Development Board - V2
    https://www.adafruit.com/product/4335

    Adafruit I2S MEMS Microphone - SPH0645LM4H
    https://www.adafruit.com/product/3421
    
*/

#include <Arduino.h>
#include "WiFi.h"
#include "AsyncUDP.h"
#include <driver/i2s.h>
#include <soc/i2s_reg.h>

//Set youy WiFi network name and password:
const char* ssid = "ssid";
const char* pswd = "password";

//Set your broadcast target here i.e. 192.168.1.40
IPAddress udpAddress(192, 168, 1, 40);
const int udpPort = 18000; //UDP Port:

boolean connected = false; //UDP State:

AsyncUDP udp; //Class UDP:

const i2s_port_t I2S_PORT = I2S_NUM_0;

const int block_size = 128;

void setup() {
    Serial.begin(115200);
    Serial.println("Configuring WiFi");
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, pswd);
    if (WiFi.waitForConnectResult() != WL_CONNECTED) {
        Serial.println("WiFi Failed");
        while (1) {
            delay(1000);
        }
    }

    // I2S CONFIG
    Serial.println("Configuring I2S port");
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

//  Pins from the microphone to GPIO on the ESP32 Tinypico
i2s_pin_config_t pin_config = {
    .bck_io_num = 21, //BCLK to GPIO21
    .ws_io_num = 25,  //LRCL to GPIO25
    .data_out_num = I2S_PIN_NO_CHANGE, // this is DATA output pin
    .data_in_num = 32 //DOUT to GPIO32
};

    // CONFIGURE I2S DRIVER AND PINS.
    err = i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);
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
}

int32_t buffer[512]; // Buffer
volatile uint16_t rpt = 0; // Pointer

void i2s_mic() {
    int num_bytes_read = i2s_read_bytes(I2S_PORT, (char*)buffer + rpt, block_size, portMAX_DELAY);
    rpt = rpt + num_bytes_read;
    if (rpt > 2043) rpt = 0;
    
}


void loop() {
    static uint8_t state = 0; 
    i2s_mic();


    if (!connected) {
        if (udp.connect(udpAddress, udpPort)) {
            connected = true;
            Serial.println("Broadcasting to UDP Listener");
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
}
