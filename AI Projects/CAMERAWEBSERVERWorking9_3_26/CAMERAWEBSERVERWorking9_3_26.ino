#include <Arduino.h>
#include "esp_camera.h"
#include "img_converters.h"

#include <esp32DetectionNEW_inferencing.h>
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
#define Y7_GPIO_NUM 15
#define Y8_GPIO_NUM 1
#define Y9_GPIO_NUM 3

#define VSYNC_GPIO_NUM 38
#define HREF_GPIO_NUM 47
#define PCLK_GPIO_NUM 13

// ============================================================
// CAMERA SIZE
// ============================================================

#define EI_CAMERA_RAW_FRAME_BUFFER_COLS 320
#define EI_CAMERA_RAW_FRAME_BUFFER_ROWS 240
#define EI_CAMERA_FRAME_BYTE_SIZE 3

// ============================================================
// CAMERA STATE
// ============================================================

static bool camera_initialized = false;

// This is the buffer Edge Impulse will actually read.
static uint8_t *snapshot_buf = nullptr;

// ============================================================
// CAMERA CONFIG
// KEEP YOUR SETTINGS EXACTLY AS PROVIDED
// ============================================================

static camera_config_t camera_config;

// ============================================================
// EDGE IMPULSE CAMERA DATA CALLBACK
// ============================================================
//
// This is taken from the ESP32 camera example included
// inside YOUR Edge Impulse ZIP.
//
// IMPORTANT:
// ESP32 camera RGB conversion produces BGR ordering.
// Edge Impulse expects RGB.
//
// Therefore:
//
// B G R
//
// becomes:
//
// R G B
// ============================================================

static int ei_camera_get_data(
    size_t offset,
    size_t length,
    float *out_ptr
)
{
    size_t pixel_ix = offset * 3;

    size_t pixels_left = length;

    size_t out_ptr_ix = 0;

    while (pixels_left != 0)
    {
        // BGR -> RGB
        out_ptr[out_ptr_ix] =
            (snapshot_buf[pixel_ix + 2] << 16) +
            (snapshot_buf[pixel_ix + 1] << 8) +
            snapshot_buf[pixel_ix + 0];

        out_ptr_ix++;

        pixel_ix += 3;

        pixels_left--;
    }

    return 0;
}

// ============================================================
// PRINT MEMORY
// ============================================================

void print_memory()
{
    Serial.println();
    Serial.println("MEMORY:");

    Serial.printf(
        "Free heap : %u bytes\n",
        ESP.getFreeHeap()
    );

    Serial.printf(
        "Free PSRAM: %u bytes\n",
        ESP.getFreePsram()
    );

    Serial.printf(
        "Min heap  : %u bytes\n",
        ESP.getMinFreeHeap()
    );
}

// ============================================================
// INITIALIZE CAMERA
// ============================================================

bool init_camera()
{
    Serial.println();
    Serial.println("================================");
    Serial.println("CAMERA INITIALIZATION");
    Serial.println("================================");

    if (!psramFound())
    {
        Serial.println(
            "ERROR: PSRAM NOT FOUND!"
        );

        return false;
    }

    Serial.printf(
        "PSRAM size: %u bytes\n",
        ESP.getPsramSize()
    );

    // --------------------------------------------------------
    // YOUR EXACT CAMERA CONFIGURATION
    // --------------------------------------------------------

    camera_config.ledc_channel =
        LEDC_CHANNEL_0;

    camera_config.ledc_timer =
        LEDC_TIMER_0;

    camera_config.pin_d0 =
        Y2_GPIO_NUM;

    camera_config.pin_d1 =
        Y3_GPIO_NUM;

    camera_config.pin_d2 =
        Y4_GPIO_NUM;

    camera_config.pin_d3 =
        Y5_GPIO_NUM;

    camera_config.pin_d4 =
        Y6_GPIO_NUM;

    camera_config.pin_d5 =
        Y7_GPIO_NUM;

    camera_config.pin_d6 =
        Y8_GPIO_NUM;

    camera_config.pin_d7 =
        Y9_GPIO_NUM;

    camera_config.pin_xclk =
        XCLK_GPIO_NUM;

    camera_config.pin_pclk =
        PCLK_GPIO_NUM;

    camera_config.pin_vsync =
        VSYNC_GPIO_NUM;

    camera_config.pin_href =
        HREF_GPIO_NUM;

    camera_config.pin_sccb_sda =
        SIOD_GPIO_NUM;

    camera_config.pin_sccb_scl =
        SIOC_GPIO_NUM;

    camera_config.pin_pwdn =
        PWDN_GPIO_NUM;

    camera_config.pin_reset =
        RESET_GPIO_NUM;

    camera_config.xclk_freq_hz =
        20000000;

    camera_config.pixel_format =
        PIXFORMAT_JPEG;

    camera_config.frame_size =
        FRAMESIZE_QVGA;

    camera_config.jpeg_quality =
        15;

    camera_config.fb_count =
        2;

    camera_config.fb_location =
        CAMERA_FB_IN_PSRAM;

    camera_config.grab_mode =
        CAMERA_GRAB_LATEST;

    // --------------------------------------------------------
    // CAMERA INIT
    // --------------------------------------------------------

    Serial.println(
        "Calling esp_camera_init()..."
    );

    esp_err_t err =
        esp_camera_init(
            &camera_config
        );

    if (err != ESP_OK)
    {
        Serial.printf(
            "CAMERA INIT FAILED: 0x%x\n",
            err
        );

        return false;
    }

    Serial.println(
        "CAMERA INIT SUCCESS"
    );

    // --------------------------------------------------------
    // SENSOR
    // --------------------------------------------------------

    sensor_t *sensor =
        esp_camera_sensor_get();

    if (sensor == nullptr)
    {
        Serial.println(
            "ERROR: sensor is NULL"
        );

        return false;
    }

    Serial.println();
    Serial.println(
        "CAMERA SENSOR INFORMATION:"
    );

    Serial.printf(
        "PID  : 0x%02X\n",
        sensor->id.PID
    );

    Serial.printf(
        "VER  : 0x%02X\n",
        sensor->id.VER
    );

    Serial.printf(
        "MIDH : 0x%02X\n",
        sensor->id.MIDH
    );

    Serial.printf(
        "MIDL : 0x%02X\n",
        sensor->id.MIDL
    );

    // --------------------------------------------------------
    // YOUR ORIGINAL ORIENTATION
    // --------------------------------------------------------

    sensor->set_vflip(
        sensor,
        1
    );

    sensor->set_hmirror(
        sensor,
        0
    );

    camera_initialized = true;

    Serial.println(
        "Camera sensor ready."
    );

    return true;
}

// ============================================================
// CAPTURE IMAGE
// ============================================================
//
// IMPORTANT:
//
// This follows the structure of the official camera example
// contained in your Edge Impulse ZIP.
// ============================================================

bool capture_camera_image()
{
    if (!camera_initialized)
    {
        Serial.println(
            "ERROR: Camera not initialized"
        );

        return false;
    }

    Serial.println();
    Serial.println(
        "Capturing camera frame..."
    );

    camera_fb_t *fb =
        esp_camera_fb_get();

    if (fb == nullptr)
    {
        Serial.println(
            "ERROR: esp_camera_fb_get() FAILED"
        );

        return false;
    }

    Serial.printf(
        "Camera frame received:\n"
        "  Width : %u\n"
        "  Height: %u\n"
        "  Length: %u bytes\n"
        "  Format: %d\n",
        fb->width,
        fb->height,
        fb->len,
        fb->format
    );

    // --------------------------------------------------------
    // JPEG -> RGB888
    // --------------------------------------------------------

    bool converted =
        fmt2rgb888(
            fb->buf,
            fb->len,
            PIXFORMAT_JPEG,
            snapshot_buf
        );

    // Return camera buffer immediately
    esp_camera_fb_return(fb);

    if (!converted)
    {
        Serial.println(
            "ERROR: JPEG -> RGB888 FAILED"
        );

        return false;
    }

    Serial.println(
        "JPEG -> RGB888 SUCCESS"
    );

    // --------------------------------------------------------
    // Resize 320x240 -> 96x96
    //
    // IMPORTANT:
    //
    // Edge Impulse's official example performs this IN PLACE.
    //
    // snapshot_buf:
    //
    // 320x240x3
    //
    // becomes
    //
    // 96x96x3
    // --------------------------------------------------------

    Serial.printf(
        "Resizing %ux%u -> %ux%u...\n",
        EI_CAMERA_RAW_FRAME_BUFFER_COLS,
        EI_CAMERA_RAW_FRAME_BUFFER_ROWS,
        EI_CLASSIFIER_INPUT_WIDTH,
        EI_CLASSIFIER_INPUT_HEIGHT
    );

    int resize_result =
        ei::image::processing::
        crop_and_interpolate_rgb888(
            snapshot_buf,
            EI_CAMERA_RAW_FRAME_BUFFER_COLS,
            EI_CAMERA_RAW_FRAME_BUFFER_ROWS,
            snapshot_buf,
            EI_CLASSIFIER_INPUT_WIDTH,
            EI_CLASSIFIER_INPUT_HEIGHT
        );

    if (resize_result != 0)
    {
        Serial.printf(
            "ERROR: Resize failed: %d\n",
            resize_result
        );

        return false;
    }

    Serial.println(
        "Resize SUCCESS"
    );

    return true;
}

// ============================================================
// IMAGE DATA TEST
// ============================================================

void print_image_sample()
{
    if (snapshot_buf == nullptr)
    {
        Serial.println(
            "snapshot_buf is NULL"
        );

        return;
    }

    Serial.println();
    Serial.println(
        "IMAGE SAMPLE:"
    );

    for (int i = 0; i < 5; i++)
    {
        int p = i * 3;

        uint8_t b =
            snapshot_buf[p + 0];

        uint8_t g =
            snapshot_buf[p + 1];

        uint8_t r =
            snapshot_buf[p + 2];

        Serial.printf(
            "Pixel %d: R=%3u G=%3u B=%3u\n",
            i,
            r,
            g,
            b
        );
    }
}

// ============================================================
// EDGE IMPULSE CLASSIFICATION
// ============================================================

bool run_edge_impulse()
{
    Serial.println();
    Serial.println(
        "========================================"
    );

    Serial.println(
        "STARTING EDGE IMPULSE CLASSIFIER"
    );

    Serial.println(
        "========================================"
    );

    // --------------------------------------------------------
    // Check buffer
    // --------------------------------------------------------

    if (snapshot_buf == nullptr)
    {
        Serial.println(
            "ERROR: snapshot_buf == NULL"
        );

        return false;
    }

    // --------------------------------------------------------
    // Signal
    // --------------------------------------------------------

    ei::signal_t signal;

    signal.total_length =
        EI_CLASSIFIER_INPUT_WIDTH *
        EI_CLASSIFIER_INPUT_HEIGHT;

    signal.get_data =
        &ei_camera_get_data;

    Serial.printf(
        "Signal length: %u\n",
        (unsigned)signal.total_length
    );

    Serial.printf(
        "Expected: %u\n",
        (unsigned)(
            EI_CLASSIFIER_INPUT_WIDTH *
            EI_CLASSIFIER_INPUT_HEIGHT
        )
    );

    // --------------------------------------------------------
    // Print memory BEFORE classifier
    // --------------------------------------------------------

    print_memory();

    Serial.println();
    Serial.println(
        "ABOUT TO CALL run_classifier()"
    );

    Serial.println(
        "========================================"
    );

    // --------------------------------------------------------
    // Run classifier
    // --------------------------------------------------------

    ei_impulse_result_t result = { 0 };

    unsigned long start =
        millis();

    EI_IMPULSE_ERROR err =
        run_classifier(
            &signal,
            &result,
            false
        );

    unsigned long elapsed =
        millis() - start;

    Serial.println();
    Serial.println(
        "RETURNED FROM run_classifier()"
    );

    Serial.printf(
        "Error code: %d\n",
        err
    );

    Serial.printf(
        "Time: %lu ms\n",
        elapsed
    );

    // --------------------------------------------------------
    // Check error
    // --------------------------------------------------------

    if (err != EI_IMPULSE_OK)
    {
        Serial.printf(
            "ERROR: run_classifier failed: %d\n",
            err
        );

        return false;
    }

    // ========================================================
    // CLASSIFICATION
    // ========================================================

    Serial.println();
    Serial.println(
        "PREDICTIONS:"
    );

    int best_index = -1;

    float best_value = -1.0f;

    for (
        uint16_t i = 0;
        i < EI_CLASSIFIER_LABEL_COUNT;
        i++
    )
    {
        const char *label =
            ei_classifier_inferencing_categories[i];

        float value =
            result.classification[i].value;

        Serial.printf(
            "  %s: %.5f (%.2f%%)\n",
            label,
            value,
            value * 100.0f
        );

        if (value > best_value)
        {
            best_value = value;
            best_index = i;
        }
    }

    // ========================================================
    // BEST RESULT
    // ========================================================

    if (best_index >= 0)
    {
        const char *label =
            ei_classifier_inferencing_categories[
                best_index
            ];

        Serial.println();
        Serial.println(
            "========================================"
        );

        Serial.printf(
            "BEST RESULT: %s\n",
            label
        );

        Serial.printf(
            "CONFIDENCE : %.2f%%\n",
            best_value * 100.0f
        );

        Serial.printf(
            "THRESHOLD  : %.2f%%\n",
            EI_CLASSIFIER_THRESHOLD * 100.0f
        );

        if (
            best_value >=
            EI_CLASSIFIER_THRESHOLD
        )
        {
            Serial.println(
                "STATUS     : CONFIDENT"
            );

            if (
                strcmp(
                    label,
                    "Keyboard"
                ) == 0
            )
            {
                Serial.println();
                Serial.println(
                    "*** KEYBOARD DETECTED! ***"
                );
            }

            else if (
                strcmp(
                    label,
                    "Mouse"
                ) == 0
            )
            {
                Serial.println();
                Serial.println(
                    "*** MOUSE DETECTED! ***"
                );
            }
        }
        else
        {
            Serial.println(
                "STATUS     : BELOW THRESHOLD"
            );
        }

        Serial.println(
            "========================================"
        );
    }

    // ========================================================
    // TIMING
    // ========================================================

    Serial.println();
    Serial.println(
        "TIMING:"
    );

    Serial.printf(
        "DSP            : %d ms\n",
        result.timing.dsp
    );

    Serial.printf(
        "Classification : %d ms\n",
        result.timing.classification
    );

    Serial.printf(
        "Anomaly        : %d ms\n",
        result.timing.anomaly
    );

    print_memory();

    return true;
}

// ============================================================
// SETUP
// ============================================================

void setup()
{
    Serial.begin(
        115200
    );

    delay(1500);

    Serial.println();
    Serial.println();

    Serial.println(
        "========================================"
    );

    Serial.println(
        "CAMBUTTON ESP32-S3"
    );

    Serial.println(
        "EDGE IMPULSE CAMERA CLASSIFICATION"
    );

    Serial.println(
        "========================================"
    );

    Serial.printf(
        "Chip: %s\n",
        ESP.getChipModel()
    );

    Serial.printf(
        "CPU: %u MHz\n",
        ESP.getCpuFreqMHz()
    );

    Serial.printf(
        "PSRAM: %s\n",
        psramFound()
            ? "YES"
            : "NO"
    );

    print_memory();

    // --------------------------------------------------------
    // Camera
    // --------------------------------------------------------

    if (!init_camera())
    {
        Serial.println(
            "FATAL: Camera initialization failed."
        );

        while (true)
        {
            delay(1000);
        }
    }

    // --------------------------------------------------------
    // Allocate ONE snapshot buffer
    // --------------------------------------------------------
    //
    // 320 * 240 * 3
    //
    // = 230,400 bytes
    //
    // We deliberately use malloc here, matching the official
    // Edge Impulse ESP32 camera example in your ZIP.
    // --------------------------------------------------------

    snapshot_buf =
        (uint8_t *)malloc(
            EI_CAMERA_RAW_FRAME_BUFFER_COLS *
            EI_CAMERA_RAW_FRAME_BUFFER_ROWS *
            EI_CAMERA_FRAME_BYTE_SIZE
        );

    if (snapshot_buf == nullptr)
    {
        Serial.println(
            "FATAL: snapshot buffer allocation failed."
        );

        while (true)
        {
            delay(1000);
        }
    }

    Serial.println();
    Serial.println(
        "Snapshot buffer allocated."
    );

    Serial.printf(
        "Snapshot buffer size: %u bytes\n",
        (unsigned)(
            EI_CAMERA_RAW_FRAME_BUFFER_COLS *
            EI_CAMERA_RAW_FRAME_BUFFER_ROWS *
            EI_CAMERA_FRAME_BYTE_SIZE
        )
    );

    // ========================================================
    // MODEL INFORMATION
    // ========================================================

    Serial.println();
    Serial.println(
        "========================================"
    );

    Serial.println(
        "EDGE IMPULSE MODEL"
    );

    Serial.println(
        "========================================"
    );

    Serial.printf(
        "Project ID       : %d\n",
        EI_CLASSIFIER_PROJECT_ID
    );

    Serial.printf(
        "Project name     : %s\n",
        EI_CLASSIFIER_PROJECT_NAME
    );

    Serial.printf(
        "Input width      : %d\n",
        EI_CLASSIFIER_INPUT_WIDTH
    );

    Serial.printf(
        "Input height     : %d\n",
        EI_CLASSIFIER_INPUT_HEIGHT
    );

    Serial.printf(
        "Input frames     : %d\n",
        EI_CLASSIFIER_INPUT_FRAMES
    );

    Serial.printf(
        "Raw sample count : %d\n",
        EI_CLASSIFIER_RAW_SAMPLE_COUNT
    );

    Serial.printf(
        "Label count      : %d\n",
        EI_CLASSIFIER_LABEL_COUNT
    );

    Serial.printf(
        "Object detection : %s\n",
        EI_CLASSIFIER_OBJECT_DETECTION
            ? "YES"
            : "NO"
    );

    Serial.printf(
        "Threshold        : %.2f\n",
        EI_CLASSIFIER_THRESHOLD
    );

    Serial.println();
    Serial.println(
        "LABELS:"
    );

    for (
        uint16_t i = 0;
        i < EI_CLASSIFIER_LABEL_COUNT;
        i++
    )
    {
        Serial.printf(
            "  [%d] %s\n",
            i,
            ei_classifier_inferencing_categories[i]
        );
    }

    Serial.println(
        "========================================"
    );

    Serial.println();
    Serial.println(
        "EXPECTED MODEL:"
    );

    Serial.println(
        "  Label 0 = Keyboard"
    );

    Serial.println(
        "  Label 1 = Mouse"
    );

    Serial.println(
        "  Classification model"
    );

    Serial.println(
        "  96x96 RGB input"
    );

    Serial.println(
        "========================================"
    );

    delay(2000);
}

// ============================================================
// LOOP
// ============================================================

void loop()
{
    // --------------------------------------------------------
    // Capture
    // --------------------------------------------------------

    if (!capture_camera_image())
    {
        Serial.println(
            "Capture failed."
        );

        delay(1000);

        return;
    }

    // --------------------------------------------------------
    // Show a few pixels
    // --------------------------------------------------------

    print_image_sample();

    // --------------------------------------------------------
    // Run Edge Impulse
    // --------------------------------------------------------

    run_edge_impulse();

    Serial.println();
    Serial.println(
        "========================================"
    );

    Serial.println(
        "WAITING FOR NEXT FRAME..."
    );

    Serial.println(
        "========================================"
    );

    delay(1000);
}