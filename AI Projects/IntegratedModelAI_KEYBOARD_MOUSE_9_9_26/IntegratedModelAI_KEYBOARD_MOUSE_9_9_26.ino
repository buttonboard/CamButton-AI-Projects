// #include <Arduino.h>
// #include "esp_camera.h"
// #include "img_converters.h"

// #include <esp32DetectionNEW_inferencing.h>
// #include "edge-impulse-sdk/dsp/image/image.hpp"

// // ============================================================
// // CAMBUTTON / FREENOVE ESP32-S3 CAMERA PINS
// // DO NOT CHANGE
// // ============================================================

// #define PWDN_GPIO_NUM -1
// #define RESET_GPIO_NUM -1
// #define XCLK_GPIO_NUM 10
// #define SIOD_GPIO_NUM 8
// #define SIOC_GPIO_NUM 9

// #define Y2_GPIO_NUM 18
// #define Y3_GPIO_NUM 14
// #define Y4_GPIO_NUM 12
// #define Y5_GPIO_NUM 16
// #define Y6_GPIO_NUM 17
// #define Y7_GPIO_NUM 15
// #define Y8_GPIO_NUM 1
// #define Y9_GPIO_NUM 3

// #define VSYNC_GPIO_NUM 38
// #define HREF_GPIO_NUM 47
// #define PCLK_GPIO_NUM 13

// // ============================================================
// // CAMERA SIZE
// // ============================================================

// #define EI_CAMERA_RAW_FRAME_BUFFER_COLS 320
// #define EI_CAMERA_RAW_FRAME_BUFFER_ROWS 240
// #define EI_CAMERA_FRAME_BYTE_SIZE 3
// #define EI_BBCLASSIFIER_THRESHOLD 0.7
// // ============================================================
// // CAMERA STATE
// // ============================================================

// static bool camera_initialized = false;

// // This is the buffer Edge Impulse will actually read.
// static uint8_t *snapshot_buf = nullptr;

// // ============================================================
// // CAMERA CONFIG
// // KEEP YOUR SETTINGS EXACTLY AS PROVIDED
// // ============================================================

// static camera_config_t camera_config;

// // ============================================================
// // EDGE IMPULSE CAMERA DATA CALLBACK
// // ============================================================
// //
// // This is taken from the ESP32 camera example included
// // inside YOUR Edge Impulse ZIP.
// //
// // IMPORTANT:
// // ESP32 camera RGB conversion produces BGR ordering.
// // Edge Impulse expects RGB.
// //
// // Therefore:
// //
// // B G R
// //
// // becomes:
// //
// // R G B
// // ============================================================

// static int ei_camera_get_data(
//     size_t offset,
//     size_t length,
//     float *out_ptr
// )
// {
//     size_t pixel_ix = offset * 3;

//     size_t pixels_left = length;

//     size_t out_ptr_ix = 0;

//     while (pixels_left != 0)
//     {
//         // BGR -> RGB
//         out_ptr[out_ptr_ix] =
//             (snapshot_buf[pixel_ix + 2] << 16) +
//             (snapshot_buf[pixel_ix + 1] << 8) +
//             snapshot_buf[pixel_ix + 0];

//         out_ptr_ix++;

//         pixel_ix += 3;

//         pixels_left--;
//     }

//     return 0;
// }

// // ============================================================
// // PRINT MEMORY
// // ============================================================

// void print_memory()
// {
//     Serial.println();
//     Serial.println("MEMORY:");

//     Serial.printf(
//         "Free heap : %u bytes\n",
//         ESP.getFreeHeap()
//     );

//     Serial.printf(
//         "Free PSRAM: %u bytes\n",
//         ESP.getFreePsram()
//     );

//     Serial.printf(
//         "Min heap  : %u bytes\n",
//         ESP.getMinFreeHeap()
//     );
// }

// // ============================================================
// // INITIALIZE CAMERA
// // ============================================================

// bool init_camera()
// {
//     Serial.println();
//     Serial.println("================================");
//     Serial.println("CAMERA INITIALIZATION");
//     Serial.println("================================");

//     if (!psramFound())
//     {
//         Serial.println(
//             "ERROR: PSRAM NOT FOUND!"
//         );

//         return false;
//     }

//     Serial.printf(
//         "PSRAM size: %u bytes\n",
//         ESP.getPsramSize()
//     );

//     // --------------------------------------------------------
//     // YOUR EXACT CAMERA CONFIGURATION
//     // --------------------------------------------------------

//     camera_config.ledc_channel =
//         LEDC_CHANNEL_0;

//     camera_config.ledc_timer =
//         LEDC_TIMER_0;

//     camera_config.pin_d0 =
//         Y2_GPIO_NUM;

//     camera_config.pin_d1 =
//         Y3_GPIO_NUM;

//     camera_config.pin_d2 =
//         Y4_GPIO_NUM;

//     camera_config.pin_d3 =
//         Y5_GPIO_NUM;

//     camera_config.pin_d4 =
//         Y6_GPIO_NUM;

//     camera_config.pin_d5 =
//         Y7_GPIO_NUM;

//     camera_config.pin_d6 =
//         Y8_GPIO_NUM;

//     camera_config.pin_d7 =
//         Y9_GPIO_NUM;

//     camera_config.pin_xclk =
//         XCLK_GPIO_NUM;

//     camera_config.pin_pclk =
//         PCLK_GPIO_NUM;

//     camera_config.pin_vsync =
//         VSYNC_GPIO_NUM;

//     camera_config.pin_href =
//         HREF_GPIO_NUM;

//     camera_config.pin_sccb_sda =
//         SIOD_GPIO_NUM;

//     camera_config.pin_sccb_scl =
//         SIOC_GPIO_NUM;

//     camera_config.pin_pwdn =
//         PWDN_GPIO_NUM;

//     camera_config.pin_reset =
//         RESET_GPIO_NUM;

//     camera_config.xclk_freq_hz =
//         20000000;

//     camera_config.pixel_format =
//         PIXFORMAT_JPEG;

//     camera_config.frame_size =
//         FRAMESIZE_QVGA;

//     camera_config.jpeg_quality =
//         15;

//     camera_config.fb_count =
//         2;

//     camera_config.fb_location =
//         CAMERA_FB_IN_PSRAM;

//     camera_config.grab_mode =
//         CAMERA_GRAB_LATEST;

//     // --------------------------------------------------------
//     // CAMERA INIT
//     // --------------------------------------------------------

//     Serial.println(
//         "Calling esp_camera_init()..."
//     );

//     esp_err_t err =
//         esp_camera_init(
//             &camera_config
//         );

//     if (err != ESP_OK)
//     {
//         Serial.printf(
//             "CAMERA INIT FAILED: 0x%x\n",
//             err
//         );

//         return false;
//     }

//     Serial.println(
//         "CAMERA INIT SUCCESS"
//     );

//     // --------------------------------------------------------
//     // SENSOR
//     // --------------------------------------------------------

//     sensor_t *sensor =
//         esp_camera_sensor_get();

//     if (sensor == nullptr)
//     {
//         Serial.println(
//             "ERROR: sensor is NULL"
//         );

//         return false;
//     }

//     Serial.println();
//     Serial.println(
//         "CAMERA SENSOR INFORMATION:"
//     );

//     Serial.printf(
//         "PID  : 0x%02X\n",
//         sensor->id.PID
//     );

//     Serial.printf(
//         "VER  : 0x%02X\n",
//         sensor->id.VER
//     );

//     Serial.printf(
//         "MIDH : 0x%02X\n",
//         sensor->id.MIDH
//     );

//     Serial.printf(
//         "MIDL : 0x%02X\n",
//         sensor->id.MIDL
//     );

//     // --------------------------------------------------------
//     // YOUR ORIGINAL ORIENTATION
//     // --------------------------------------------------------

//     sensor->set_vflip(
//         sensor,
//         1
//     );

//     sensor->set_hmirror(
//         sensor,
//         0
//     );

//     camera_initialized = true;

//     Serial.println(
//         "Camera sensor ready."
//     );

//     return true;
// }

// // ============================================================
// // CAPTURE IMAGE
// // ============================================================
// //
// // IMPORTANT:
// //
// // This follows the structure of the official camera example
// // contained in your Edge Impulse ZIP.
// // ============================================================

// bool capture_camera_image()
// {
//     if (!camera_initialized)
//     {
//         Serial.println(
//             "ERROR: Camera not initialized"
//         );

//         return false;
//     }

//     Serial.println();
//     Serial.println(
//         "Capturing camera frame..."
//     );

//     camera_fb_t *fb =
//         esp_camera_fb_get();

//     if (fb == nullptr)
//     {
//         Serial.println(
//             "ERROR: esp_camera_fb_get() FAILED"
//         );

//         return false;
//     }

//     Serial.printf(
//         "Camera frame received:\n"
//         "  Width : %u\n"
//         "  Height: %u\n"
//         "  Length: %u bytes\n"
//         "  Format: %d\n",
//         fb->width,
//         fb->height,
//         fb->len,
//         fb->format
//     );

//     // --------------------------------------------------------
//     // JPEG -> RGB888
//     // --------------------------------------------------------

//     bool converted =
//         fmt2rgb888(
//             fb->buf,
//             fb->len,
//             PIXFORMAT_JPEG,
//             snapshot_buf
//         );

//     // Return camera buffer immediately
//     esp_camera_fb_return(fb);

//     if (!converted)
//     {
//         Serial.println(
//             "ERROR: JPEG -> RGB888 FAILED"
//         );

//         return false;
//     }

//     Serial.println(
//         "JPEG -> RGB888 SUCCESS"
//     );

//     // --------------------------------------------------------
//     // Resize 320x240 -> 96x96
//     //
//     // IMPORTANT:
//     //
//     // Edge Impulse's official example performs this IN PLACE.
//     //
//     // snapshot_buf:
//     //
//     // 320x240x3
//     //
//     // becomes
//     //
//     // 96x96x3
//     // --------------------------------------------------------

//     Serial.printf(
//         "Resizing %ux%u -> %ux%u...\n",
//         EI_CAMERA_RAW_FRAME_BUFFER_COLS,
//         EI_CAMERA_RAW_FRAME_BUFFER_ROWS,
//         EI_CLASSIFIER_INPUT_WIDTH,
//         EI_CLASSIFIER_INPUT_HEIGHT
//     );

//     int resize_result =
//         ei::image::processing::
//         crop_and_interpolate_rgb888(
//             snapshot_buf,
//             EI_CAMERA_RAW_FRAME_BUFFER_COLS,
//             EI_CAMERA_RAW_FRAME_BUFFER_ROWS,
//             snapshot_buf,
//             EI_CLASSIFIER_INPUT_WIDTH,
//             EI_CLASSIFIER_INPUT_HEIGHT
//         );

//     if (resize_result != 0)
//     {
//         Serial.printf(
//             "ERROR: Resize failed: %d\n",
//             resize_result
//         );

//         return false;
//     }

//     Serial.println(
//         "Resize SUCCESS"
//     );

//     return true;
// }

// // ============================================================
// // IMAGE DATA TEST
// // ============================================================

// void print_image_sample()
// {
//     if (snapshot_buf == nullptr)
//     {
//         Serial.println(
//             "snapshot_buf is NULL"
//         );

//         return;
//     }

//     Serial.println();
//     Serial.println(
//         "IMAGE SAMPLE:"
//     );

//     for (int i = 0; i < 5; i++)
//     {
//         int p = i * 3;

//         uint8_t b =
//             snapshot_buf[p + 0];

//         uint8_t g =
//             snapshot_buf[p + 1];

//         uint8_t r =
//             snapshot_buf[p + 2];

//         Serial.printf(
//             "Pixel %d: R=%3u G=%3u B=%3u\n",
//             i,
//             r,
//             g,
//             b
//         );
//     }
// }

// // ============================================================
// // EDGE IMPULSE CLASSIFICATION
// // ============================================================

// bool run_edge_impulse()
// {
//     Serial.println();
//     Serial.println(
//         "========================================"
//     );

//     Serial.println(
//         "STARTING EDGE IMPULSE CLASSIFIER"
//     );

//     Serial.println(
//         "========================================"
//     );

//     // --------------------------------------------------------
//     // Check buffer
//     // --------------------------------------------------------

//     if (snapshot_buf == nullptr)
//     {
//         Serial.println(
//             "ERROR: snapshot_buf == NULL"
//         );

//         return false;
//     }

//     // --------------------------------------------------------
//     // Signal
//     // --------------------------------------------------------

//     ei::signal_t signal;

//     signal.total_length =
//         EI_CLASSIFIER_INPUT_WIDTH *
//         EI_CLASSIFIER_INPUT_HEIGHT;

//     signal.get_data =
//         &ei_camera_get_data;

//     Serial.printf(
//         "Signal length: %u\n",
//         (unsigned)signal.total_length
//     );

//     Serial.printf(
//         "Expected: %u\n",
//         (unsigned)(
//             EI_CLASSIFIER_INPUT_WIDTH *
//             EI_CLASSIFIER_INPUT_HEIGHT
//         )
//     );

//     // --------------------------------------------------------
//     // Print memory BEFORE classifier
//     // --------------------------------------------------------

//     print_memory();

//     Serial.println();
//     Serial.println(
//         "ABOUT TO CALL run_classifier()"
//     );

//     Serial.println(
//         "========================================"
//     );

//     // --------------------------------------------------------
//     // Run classifier
//     // --------------------------------------------------------

//     ei_impulse_result_t result = { 0 };

//     unsigned long start =
//         millis();

//     EI_IMPULSE_ERROR err =
//         run_classifier(
//             &signal,
//             &result,
//             false
//         );

//     unsigned long elapsed =
//         millis() - start;

//     Serial.println();
//     Serial.println(
//         "RETURNED FROM run_classifier()"
//     );

//     Serial.printf(
//         "Error code: %d\n",
//         err
//     );

//     Serial.printf(
//         "Time: %lu ms\n",
//         elapsed
//     );

//     // --------------------------------------------------------
//     // Check error
//     // --------------------------------------------------------

//     if (err != EI_IMPULSE_OK)
//     {
//         Serial.printf(
//             "ERROR: run_classifier failed: %d\n",
//             err
//         );

//         return false;
//     }

//     // ========================================================
//     // CLASSIFICATION
//     // ========================================================

//     Serial.println();
//     Serial.println(
//         "PREDICTIONS:"
//     );

//     int best_index = -1;

//     float best_value = -1.0f;

//     for (
//         uint16_t i = 0;
//         i < EI_CLASSIFIER_LABEL_COUNT;
//         i++
//     )
//     {
//         const char *label =
//             ei_classifier_inferencing_categories[i];

//         float value =
//             result.classification[i].value;

//         Serial.printf(
//             "  %s: %.5f (%.2f%%)\n",
//             label,
//             value,
//             value * 100.0f
//         );

//         if (value > best_value)
//         {
//             best_value = value;
//             best_index = i;
//         }
//     }

//     // ========================================================
//     // BEST RESULT
//     // ========================================================

//     if (best_index >= 0)
//     {
//         const char *label =
//             ei_classifier_inferencing_categories[
//                 best_index
//             ];

//         Serial.println(
//             "========================================"
//         );

//         Serial.printf(
//             "BEST RESULT: %s\n",
//             label
//         );

//         Serial.printf(
//             "CONFIDENCE : %.2f%%\n",
//             best_value * 100.0f
//         );

//         Serial.printf(
//             "THRESHOLD  : %.2f%%\n",
//             EI_BBCLASSIFIER_THRESHOLD * 100.0f
//         );

//         if (
//             best_value >=
//             EI_BBCLASSIFIER_THRESHOLD
//         )
//         {
//             Serial.println(
//                 "STATUS     : CONFIDENT"
//             );

//             if (
//                 strcmp(
//                     label,
//                     "Keyboard"
//                 ) == 0
//             )
//             {
//                 Serial.println();
//                 Serial.println(
//                     "*** KEYBOARD DETECTED! ***"
//                 );
//             }

//             else if (
//                 strcmp(
//                     label,
//                     "Mouse"
//                 ) == 0
//             )
//             {
//                 Serial.println();
//                 Serial.println(
//                     "*** MOUSE DETECTED! ***"
//                 );
//             }
//         }
//         else
//         {
//             Serial.println(
//                 "STATUS     : BELOW THRESHOLD"
//             );
//         }

//         Serial.println(
//             "========================================"
//         );
//     }

//     // ========================================================
//     // TIMING
//     // ========================================================

//     Serial.println();
//     Serial.println(
//         "TIMING:"
//     );

//     Serial.printf(
//         "DSP            : %d ms\n",
//         result.timing.dsp
//     );

//     Serial.printf(
//         "Classification : %d ms\n",
//         result.timing.classification
//     );

//     Serial.printf(
//         "Anomaly        : %d ms\n",
//         result.timing.anomaly
//     );

//     print_memory();

//     return true;
// }

// // ============================================================
// // SETUP
// // ============================================================

// void setup()
// {
//     Serial.begin(
//         115200
//     );

//     delay(1500);

//     Serial.println();
//     Serial.println();

//     Serial.println(
//         "========================================"
//     );

//     Serial.println(
//         "CAMBUTTON ESP32-S3"
//     );

//     Serial.println(
//         "EDGE IMPULSE CAMERA CLASSIFICATION"
//     );

//     Serial.println(
//         "========================================"
//     );

//     Serial.printf(
//         "Chip: %s\n",
//         ESP.getChipModel()
//     );

//     Serial.printf(
//         "CPU: %u MHz\n",
//         ESP.getCpuFreqMHz()
//     );

//     Serial.printf(
//         "PSRAM: %s\n",
//         psramFound()
//             ? "YES"
//             : "NO"
//     );

//     print_memory();

//     // --------------------------------------------------------
//     // Camera
//     // --------------------------------------------------------

//     if (!init_camera())
//     {
//         Serial.println(
//             "FATAL: Camera initialization failed."
//         );

//         while (true)
//         {
//             delay(1000);
//         }
//     }

//     // --------------------------------------------------------
//     // Allocate ONE snapshot buffer
//     // --------------------------------------------------------
//     //
//     // 320 * 240 * 3
//     //
//     // = 230,400 bytes
//     //
//     // We deliberately use malloc here, matching the official
//     // Edge Impulse ESP32 camera example in your ZIP.
//     // --------------------------------------------------------

//     snapshot_buf =
//         (uint8_t *)malloc(
//             EI_CAMERA_RAW_FRAME_BUFFER_COLS *
//             EI_CAMERA_RAW_FRAME_BUFFER_ROWS *
//             EI_CAMERA_FRAME_BYTE_SIZE
//         );

//     if (snapshot_buf == nullptr)
//     {
//         Serial.println(
//             "FATAL: snapshot buffer allocation failed."
//         );

//         while (true)
//         {
//             delay(1000);
//         }
//     }

//     Serial.println();
//     Serial.println(
//         "Snapshot buffer allocated."
//     );

//     Serial.printf(
//         "Snapshot buffer size: %u bytes\n",
//         (unsigned)(
//             EI_CAMERA_RAW_FRAME_BUFFER_COLS *
//             EI_CAMERA_RAW_FRAME_BUFFER_ROWS *
//             EI_CAMERA_FRAME_BYTE_SIZE
//         )
//     );

//     // ========================================================
//     // MODEL INFORMATION
//     // ========================================================

//     Serial.println();
//     Serial.println(
//         "========================================"
//     );

//     Serial.println(
//         "EDGE IMPULSE MODEL"
//     );

//     Serial.println(
//         "========================================"
//     );

//     Serial.printf(
//         "Project ID       : %d\n",
//         EI_CLASSIFIER_PROJECT_ID
//     );

//     Serial.printf(
//         "Project name     : %s\n",
//         EI_CLASSIFIER_PROJECT_NAME
//     );

//     Serial.printf(
//         "Input width      : %d\n",
//         EI_CLASSIFIER_INPUT_WIDTH
//     );

//     Serial.printf(
//         "Input height     : %d\n",
//         EI_CLASSIFIER_INPUT_HEIGHT
//     );

//     Serial.printf(
//         "Input frames     : %d\n",
//         EI_CLASSIFIER_INPUT_FRAMES
//     );

//     Serial.printf(
//         "Raw sample count : %d\n",
//         EI_CLASSIFIER_RAW_SAMPLE_COUNT
//     );

//     Serial.printf(
//         "Label count      : %d\n",
//         EI_CLASSIFIER_LABEL_COUNT
//     );

//     Serial.printf(
//         "Object detection : %s\n",
//         EI_CLASSIFIER_OBJECT_DETECTION
//             ? "YES"
//             : "NO"
//     );

//     Serial.printf(
//         "Threshold        : %.2f\n",
//         EI_BBCLASSIFIER_THRESHOLD
//     );

//     Serial.println();
//     Serial.println(
//         "LABELS:"
//     );

//     for (
//         uint16_t i = 0;
//         i < EI_CLASSIFIER_LABEL_COUNT;
//         i++
//     )
//     {
//         Serial.printf(
//             "  [%d] %s\n",
//             i,
//             ei_classifier_inferencing_categories[i]
//         );
//     }

//     Serial.println(
//         "========================================"
//     );

//     Serial.println();
//     Serial.println(
//         "EXPECTED MODEL:"
//     );

//     Serial.println(
//         "  Label 0 = Keyboard"
//     );

//     Serial.println(
//         "  Label 1 = Mouse"
//     );

//     Serial.println(
//         "  Classification model"
//     );

//     Serial.println(
//         "  96x96 RGB input"
//     );

//     Serial.println(
//         "========================================"
//     );

//     delay(2000);
// }

// // ============================================================
// // LOOP
// // ============================================================

// void loop()
// {
//     // --------------------------------------------------------
//     // Capture
//     // --------------------------------------------------------

//     if (!capture_camera_image())
//     {
//         Serial.println(
//             "Capture failed."
//         );

//         delay(1000);

//         return;
//     }

//     // --------------------------------------------------------
//     // Show a few pixels
//     // --------------------------------------------------------

//     print_image_sample();

//     // --------------------------------------------------------
//     // Run Edge Impulse
//     // --------------------------------------------------------

//     run_edge_impulse();

//     Serial.println();
//     Serial.println(
//         "========================================"
//     );

//     Serial.println(
//         "WAITING FOR NEXT FRAME..."
//     );

//     Serial.println(
//         "========================================"
//     );

//     delay(1000);
// }

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include <Arduino.h>
#include "esp_camera.h"
#include "img_converters.h"

// Updated header for the new Edge Impulse library
#include <ONEObject_inferencing.h>
#include "edge-impulse-sdk/dsp/image/image.hpp"

// ============================================================
// CAMBUTTON / FREENOVE ESP32-S3 CAMERA PINS
// DO NOT CHANGE
// ============================================================

#define PWDN_GPIO_NUM -1
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM 10
#define SIOD_GPIO_NUM 8
#define SIOC_GPIO_NUM 9

#define Y2_GPIO_NUM 18
#define Y3_GPIO_NUM 14
#define Y4_GPIO_NUM 12
#define Y5_GPIO_NUM 16
#define Y6_GPIO_NUM 17
#define Y7_GPIO_NUM 1
#define Y8_GPIO_NUM 2
#define Y9_GPIO_NUM 35
#define VSYNC_GPIO_NUM 38
#define HREF_GPIO_NUM 47
#define PCLK_GPIO_NUM 13

// Camera and buffer state variables
static camera_fb_t *fb = NULL;
static bool is_initialised = false;

// Signal buffer conversion for Edge Impulse SDK
static int raw_feature_get_data(size_t offset, size_t length, float *out_ptr) {
    size_t pixel_count = length;
    size_t out_ptr_ix = 0;

    // Convert RGB565 / JPEG payload to RGB888 float format
    uint8_t *rgb888_buf = (uint8_t *)malloc(EI_CLASSIFIER_INPUT_WIDTH * EI_CLASSIFIER_INPUT_HEIGHT * 3);
    if (!rgb888_buf) {
        ei_printf("ERR: Failed to allocate RGB888 buffer\n");
        return -1;
    }

    bool converted = fmt2rgb888(fb->buf, fb->len, fb->format, rgb888_buf);
    if (!converted) {
        free(rgb888_buf);
        ei_printf("ERR: Failed to convert frame buffer format\n");
        return -1;
    }

    for (size_t i = offset * 3; i < (offset + pixel_count) * 3; i += 3) {
        uint8_t r = rgb888_buf[i];
        uint8_t g = rgb888_buf[i + 1];
        uint8_t b = rgb888_buf[i + 2];

        // Pack RGB888 values into standard float representation (0xRRGGBB)
        float pixel_f = (r << 16) | (g << 8) | b;
        out_ptr[out_ptr_ix] = pixel_f;
        out_ptr_ix++;
    }

    free(rgb888_buf);
    return 0;
}

void setup() {
    Serial.begin(115200);
    while (!Serial);

    // Configure camera hardware settings
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
    config.frame_size = FRAMESIZE_QVGA;
    config.pixel_format = PIXFORMAT_JPEG;
    config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
    config.fb_location = CAMERA_FB_IN_PSRAM;
    config.jpeg_quality = 12;
    config.fb_count = 1;

    // Initialize OV2640 camera
    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        Serial.printf("Camera init failed with error 0x%x\n", err);
        return;
    }

    sensor_t *s = esp_camera_sensor_get();
    if (s->id.PID == OV2640_PID) {
        s->set_vflip(s, 1);
        s->set_hmirror(s, 1);
    }

    is_initialised = true;
    Serial.println("Camera initialized successfully.");
}

void loop() {
    if (!is_initialised) {
        return;
    }

    fb = esp_camera_fb_get();
    if (!fb) {
        Serial.println("Camera capture failed");
        return;
    }

    // Set up signal object for Edge Impulse SDK
    ei::signal_t signal;
    signal.total_length = EI_CLASSIFIER_INPUT_WIDTH * EI_CLASSIFIER_INPUT_HEIGHT;
    signal.get_data = &raw_feature_get_data;

    ei_impulse_result_t result = { 0 };
    EI_IMPULSE_ERROR res = run_classifier(&signal, &result, false);

    if (res != EI_IMPULSE_OK) {
        Serial.printf("ERR: Failed to run classifier (%d)\n", res);
        esp_camera_fb_return(fb);
        return;
    }

    // Object Detection output evaluation
#if EI_CLASSIFIER_OBJECT_DETECTION == 1
    bool found_gps = false;
    bool found_no_gps = false;

    for (size_t ix = 0; ix < result.bounding_boxes_count; ix++) {
        auto bb = result.bounding_boxes[ix];
        if (bb.value < 0.5) continue; // Filter confidence below threshold

        Serial.printf("Detected: %s (%f) [x:%u, y:%u, w:%u, h:%u]\n",
                      bb.label, bb.value, bb.x, bb.y, bb.width, bb.height);

        if (strcmp(bb.label, "gps") == 0) {
            found_gps = true;
        } else if (strcmp(bb.label, "no gps") == 0 || strcmp(bb.label, "no_gps") == 0) {
            found_no_gps = true;
        }
    }

    if (found_gps) {
        Serial.println("--> Action: 'gps' detected!");
    } else if (found_no_gps) {
        Serial.println("--> Action: 'no gps' detected!");
    } else {
        Serial.println("--> Action: No target labels identified.");
    }

    // Classification output evaluation (for non-object detection models)
#else
    for (size_t ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT; ix++) {
        const char *label = result.classification[ix].label;
        float value = result.classification[ix].value;

        if (strcmp(label, "gps") == 0 && value >= 0.6) {
            Serial.printf("Label: %s | Confidence: %.2f --> Target: GPS\n", label, value);
        } else if ((strcmp(label, "no gps") == 0 || strcmp(label, "no_gps") == 0) && value >= 0.6) {
            Serial.printf("Label: %s | Confidence: %.2f --> Target: NO GPS\n", label, value);
        }
    }
#endif

    esp_camera_fb_return(fb);
    delay(1000);
}