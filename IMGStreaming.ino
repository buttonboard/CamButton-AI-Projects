#include "esp_camera.h"
#include <WiFi.h>
#include <WebServer.h>

// Replace with your network credentials
const char* ssid = "AlanAsh 2.4G";
const char* password = "Alan2760";
#define PWDN_GPIO_NUM  -1
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM  14
#define SIOD_GPIO_NUM  8 
#define SIOC_GPIO_NUM  9 
#define Y9_GPIO_NUM    16
#define Y8_GPIO_NUM    15
#define Y7_GPIO_NUM    12
#define Y6_GPIO_NUM    7
#define Y5_GPIO_NUM    11
#define Y4_GPIO_NUM    6
#define Y3_GPIO_NUM    10
#define Y2_GPIO_NUM    3
#define VSYNC_GPIO_NUM 4
#define HREF_GPIO_NUM  18
#define PCLK_GPIO_NUM  13

WebServer server(80);

void startCameraServer();

void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();

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
  config.pixel_format = PIXFORMAT_RGB565;

  // Use the lowest resolution and quality
  config.frame_size = FRAMESIZE_QQVGA; // Lowest resolution
  config.jpeg_quality = 63; // Lowest quality
  config.fb_count = 1; // Use one frame buffer if no PSRAM

  // Camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");

  // Start server
  startCameraServer();
  Serial.print("Camera Ready! Use 'http://");
  Serial.print(WiFi.localIP());
  Serial.println("' to connect");
}

void loop() {
  server.handleClient();
}

void handle_jpg_stream() {
  camera_fb_t * fb = NULL;
  esp_err_t res = ESP_OK;
  size_t _jpg_buf_len = 0;
  uint8_t * _jpg_buf = NULL;
  char * part_buf[64];
  static const char* _STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=frame";
  static const char* _STREAM_BOUNDARY = "\r\n--frame\r\n";
  static const char* _STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

  WiFiClient client = server.client();
  if (!client.connected()) {
    return;
  }

  client.printf("HTTP/1.1 200 OK\r\n");
  client.printf("Content-Type: %s\r\n\r\n", _STREAM_CONTENT_TYPE);

  while (true) {
    fb = esp_camera_fb_get();
    if (!fb) {
      Serial.println("Camera capture failed");
      break;
    }
    if (fb->format != PIXFORMAT_JPEG) {
      bool converted = frame2jpg(fb, 80, &_jpg_buf, &_jpg_buf_len);
      if (!converted) {
        Serial.println("JPEG compression failed");
        esp_camera_fb_return(fb);
        break;
      }
    } else {
      _jpg_buf_len = fb->len;
      _jpg_buf = fb->buf;
    }

    size_t hlen = snprintf((char *)part_buf, 64, _STREAM_PART, _jpg_buf_len);
    client.write((const char *)part_buf, hlen);
    client.write((const char *)_jpg_buf, _jpg_buf_len);
    client.write(_STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY));

    if (fb->format != PIXFORMAT_JPEG) {
      free(_jpg_buf);
    }
    esp_camera_fb_return(fb);

    if (!client.connected()) {
      break;
    }
  }
}

void startCameraServer() {
  server.on("/", HTTP_GET, [](){
    server.send(200, "text/html", "<html><body><h1>ESP32 Camera</h1><img src=\"/stream\"/></body></html>");
  });
  server.on("/stream", HTTP_GET, handle_jpg_stream);
  server.begin();
}