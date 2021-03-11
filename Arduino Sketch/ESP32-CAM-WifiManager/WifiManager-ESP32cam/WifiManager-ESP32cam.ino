// LED will blink when in config mode
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <WiFiClient.h>

#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager
//for LED status
#include <Ticker.h>
Ticker ticker;

#define FLASH_LED_PIN 4
#define CAMERA_MODEL_AI_THINKER
#include "pins.h"

const char* host = "camera-backyard";
#include <ESPmDNS.h>

#include <WebServer.h>
#include <Update.h>
WebServer server(80);
#include "html.h"

#include <UniversalTelegramBot.h>

// Camera 1
 #define BOTtoken "1397681018:AAEraQzvh3jQHE2wGfzOMAwpXpR3LS6c-V4"  // your Bot Token (Get from Botfather)

// Camera 2
// #define BOTtoken "1369477523:AAFd-mUGm__WTvzTmZPwQGPfJgUIm2pqLqs"  // your Bot Token (Get from Botfather)

// Camera 3
// #define BOTtoken "1330173488:AAEWnRBGjTfzTxUGSkJ40a3VHy4m6Af4eE0"  // your Bot Token (Get from Botfather)

WiFiClientSecure client;
UniversalTelegramBot bot(BOTtoken, client);

#include "esp_camera.h"

camera_fb_t * fb;
uint8_t* fb_buffer;
size_t fb_length;
int currentByte;
#include "telegram.h"

void tick()
{
  //toggle state
  digitalWrite(FLASH_LED_PIN, !digitalRead(FLASH_LED_PIN));     // set pin to the opposite state
}

//gets called when WiFiManager enters configuration mode
void configModeCallback (WiFiManager *myWiFiManager) {
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());
  //if you used auto generated SSID, print it
  Serial.println(myWiFiManager->getConfigPortalSSID());
  //entered config mode, make led toggle faster
  ticker.attach(0.2, tick);
}

// from https://github.com/eloquentarduino/EloquentArduino/blob/master/examples/ESP32CameraNaiveMotionDetection/ESP32CameraNaiveMotionDetection.ino
/**
 * Capture image and do down-sampling
 */

#define WIDTH 320
#define HEIGHT 240
#define BLOCK_SIZE 10
#define W (WIDTH / BLOCK_SIZE)
#define H (HEIGHT / BLOCK_SIZE)
#define BLOCK_DIFF_THRESHOLD 0.2
#define IMAGE_DIFF_THRESHOLD 0.1
#define DEBUG 1
bool capture_still();
bool motion_detect();
void update_frame();
void print_frame(uint16_t frame[H][W]);


uint16_t prev_frame[H][W] = { 0 };
uint16_t current_frame[H][W] = { 0 };
 
bool capture_still() {
    camera_fb_t *frame_buffer = esp_camera_fb_get();

    if (!frame_buffer)
        return false;

    // set all 0s in current frame
    for (int y = 0; y < H; y++)
        for (int x = 0; x < W; x++)
            current_frame[y][x] = 0;


    // down-sample image in blocks
    for (uint32_t i = 0; i < WIDTH * HEIGHT; i++) {
        const uint16_t x = i % WIDTH;
        const uint16_t y = floor(i / WIDTH);
        const uint8_t block_x = floor(x / BLOCK_SIZE);
        const uint8_t block_y = floor(y / BLOCK_SIZE);
        const uint8_t pixel = frame_buffer->buf[i];
        const uint16_t current = current_frame[block_y][block_x];

        // average pixels in block (accumulate)
        current_frame[block_y][block_x] += pixel;
    }

    // average pixels in block (rescale)
    for (int y = 0; y < H; y++)
        for (int x = 0; x < W; x++)
            current_frame[y][x] /= BLOCK_SIZE * BLOCK_SIZE;

    #if DEBUG
      Serial.println("Current frame:");
      print_frame(current_frame);
      Serial.println("---------------");
    #endif

    return true;
}


/**
 * Compute the number of different blocks
 * If there are enough, then motion happened
 */
bool motion_detect() {
    uint16_t changes = 0;
    const uint16_t blocks = (WIDTH * HEIGHT) / (BLOCK_SIZE * BLOCK_SIZE);

    for (int y = 0; y < H; y++) {
        for (int x = 0; x < W; x++) {
            float current = current_frame[y][x];
            float prev = prev_frame[y][x];
            float delta = abs(current - prev) / prev;

            if (delta >= BLOCK_DIFF_THRESHOLD) {
#if DEBUG
                Serial.print("diff\t");
                Serial.print(y);
                Serial.print('\t');
                Serial.println(x);
#endif

                changes += 1;
            }
        }
    }

    Serial.print("Changed ");
    Serial.print(changes);
    Serial.print(" out of ");
    Serial.println(blocks);

    return (1.0 * changes / blocks) > IMAGE_DIFF_THRESHOLD;
}


/**
 * Copy current frame to previous
 */
void update_frame() {
    for (int y = 0; y < H; y++) {
        for (int x = 0; x < W; x++) {
            prev_frame[y][x] = current_frame[y][x];
        }
    }
}

/**
 * For serial debugging
 * @param frame
 */
void print_frame(uint16_t frame[H][W]) {
    for (int y = 0; y < H; y++) {
        for (int x = 0; x < W; x++) {
            Serial.print(frame[y][x]);
            Serial.print('\t');
        }

        Serial.println();
    }
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);

  // Random boot time delay to prevent all of them trying dhcp at the same time
  delay(random(100, 10000));
    
  WiFi.mode(WIFI_STA); // explicitly set mode, esp defaults to STA+AP    
  WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE); // Workaround from https://github.com/espressif/arduino-esp32/issues/2537 to set DHCP Hostname
  char hname[] = "INTERFACE";
  WiFi.setHostname(hname);
    
  //set led pin as output
  pinMode(FLASH_LED_PIN, OUTPUT);
  // start ticker with 0.5 because we start in AP mode and try to connect
  ticker.attach(0.6, tick);

  //WiFiManager
  //Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wm;
  //reset settings - for testing
  //wm.resetSettings();
  
  wm.setTimeout(60);
  wm.setConfigPortalTimeout(60);
  //set callback that gets called when connecting to previous WiFi fails, and enters Access Point mode
  wm.setAPCallback(configModeCallback);

  //fetches ssid and pass and tries to connect
  //if it does not connect it starts an access point with the specified name
  //here  "AutoConnectAP"
  //and goes into a blocking loop awaiting configuration
  if (!wm.autoConnect()) {
    Serial.println("failed to connect and hit timeout");
    //reset and try again, or maybe put it to deep sleep
    ESP.restart();
  }

  //if you get here you have connected to the WiFi
  Serial.println("connected...yeey :)");
  ticker.detach();
  //keep LED on
  digitalWrite(FLASH_LED_PIN, LOW);

  if (!MDNS.begin(host)) { //http://esp32.local
    Serial.println("Error setting up MDNS responder!");
    while (1) {
      delay(1000);
    }
  }

  server.on("/", HTTP_GET, []() {
    server.sendHeader("Connection", "close");
    server.send(200, "text/html", serverIndex);
  });

  server.on("/message", HTTP_GET, []() {
    take_send_photo("@the30bird");
    server.send(200, "text/html", serverIndex);
  });

  server.on("/update", HTTP_POST, []() {
    server.sendHeader("Connection", "close");
    server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
    ESP.restart();
  }, []() {
    HTTPUpload& upload = server.upload();
    if (upload.status == UPLOAD_FILE_START) {
      Serial.printf("Update: %s\n", upload.filename.c_str());
      if (!Update.begin(UPDATE_SIZE_UNKNOWN)) { //start with max available size
        Update.printError(Serial);
      }
    } else if (upload.status == UPLOAD_FILE_WRITE) {
      /* flashing firmware to ESP*/
      if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
        Update.printError(Serial);
      }
    } else if (upload.status == UPLOAD_FILE_END) {
      if (Update.end(true)) { //true to set the size to the current progress
        Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
      } else {
        Update.printError(Serial);
      }
    }
  });
  server.begin();

  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  //init with high specs to pre-allocate larger buffers
  if (psramFound()) {
    config.frame_size = FRAMESIZE_UXGA;
    config.jpeg_quality = 10;
    config.fb_count = 1;  // Trying to reduce memory use
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    ESP.restart();
    return;
  }

  sensor_t * s = esp_camera_sensor_get();

  s->set_contrast(s, 2);                      // -2 to 2
  s->set_brightness(s, 1);                    // -2 to 2
  s->set_saturation(s, -2);                    // -2 to 2
//  s->set_sharpness(s, val);                     // -2 to 2
//  s->set_gainceiling(s, (gainceiling_t)val);    // 0 to 6
//  s->set_colorbar(s, val);                      // 0 = disable , 1 = enable
  s->set_whitebal(s, 1);                      // 0 = disable , 1 = enable
  s->set_gain_ctrl(s, 1);                     // 0 = disable , 1 = enable
//  s->set_exposure_ctrl(s, val);                 // 0 = disable , 1 = enable
//  s->set_hmirror(s, val);                       // 0 = disable , 1 = enable
//  s->set_vflip(s, val);                         // 0 = disable , 1 = enable
  s->set_awb_gain(s, 1);                      // 0 = disable , 1 = enable
//  s->set_agc_gain(s, val);                      // 0 to 30
//  s->set_aec_value(s, val);                     // 0 to 1200
  s->set_aec2(s, 1);                          // 0 = disable , 1 = enable
//  s->set_denoise(s, val);                       // 0 = disable , 1 = enable
//  s->set_dcw(s, val);                           // 0 = disable , 1 = enable
//  s->set_bpc(s, val);                           // 0 = disable , 1 = enable
//  s->set_wpc(s, val);                           // 0 = disable , 1 = enable
//  s->set_raw_gma(s, val);                       // 0 = disable , 1 = enable
//  s->set_lenc(s, val);                          // 0 = disable , 1 = enable
//  s->set_special_effect(s, 2);                // 0 to 6 (0 - No Effect, 1 - Negative, 2 - Grayscale, 3 - Red Tint, 4 - Green Tint, 5 - Blue Tint, 6 - Sepia)
  s->set_wb_mode(s, 0);                       // 0 to 4 - if awb_gain enabled (0 - Auto, 1 - Sunny, 2 - Cloudy, 3 - Office, 4 - Home)
//  s->set_ae_level(s, val);                      // -2 to 2

  bot.sendMessage("@the30bird", "Camera 1 started up:  IP:  " + WiFi.localIP().toString(), "");

  
}

void loop() {
  // put your main code here, to run repeatedly:
  server.handleClient();
  delay(1);
  if ( WiFi.status() !=  WL_CONNECTED ) {
    // wifi down, reconnect here
   ESP.restart();
  }

  if (!capture_still()) {
        Serial.println("Failed capture");
        delay(3000);

        return;
    }

    if (motion_detect()) {
        Serial.println("Motion detected");
        take_send_photo("@the30bird");
    }

    update_frame();
}
