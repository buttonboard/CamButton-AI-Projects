/*
 * ============================================================
 * CAMBUTTON / FREENOVE ESP32-S3
 * EDGE IMPULSE FOMO CAMERA TEST
 *
 * Tests:
 *   1. ESP32-S3 starts
 *   2. PSRAM is detected
 *   3. Camera initializes
 *   4. Camera sensor responds
 *   5. Camera captures JPEG frames
 *   6. JPEG -> RGB888 works
 *   7. RGB888 -> 96x96 works
 *   8. Edge Impulse inference runs
 *   9. FOMO labels / bounding boxes are printed
 *
 * NO WIFI
 * NO CAMERA WEB SERVER
 * NO FLASH LED
 * ============================================================
 */

#include <Arduino.h>
#include "esp_camera.h"
#include "img_converters.h"

#include <esp32_face_rec_inferencing.h>
#include "edge-impulse-sdk/dsp/image/image.hpp"


// ============================================================
// CAMBUTTON / FREENOVE ESP32-S3 CAMERA PINS
// ============================================================
#define PWDN_GPIO_NUM -1
#define RESET_GPIO_NUM -1 
#define XCLK_GPIO_NUM 10 //15
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
// CAMERA IMAGE SIZE
// ============================================================

#define CAMERA_WIDTH   320
#define CAMERA_HEIGHT  240
#define CAMERA_CHANNELS 3


// ============================================================
// EDGE IMPULSE MODEL SIZE
// ============================================================

#define EI_WIDTH  EI_CLASSIFIER_INPUT_WIDTH
#define EI_HEIGHT EI_CLASSIFIER_INPUT_HEIGHT


// ============================================================
// GLOBAL BUFFERS
// ============================================================
//
// Camera JPEG frame lives inside the camera framebuffer.
//
// We allocate TWO permanent PSRAM buffers:
//
// 1. raw_rgb_buffer
//      320x240x3 = 230,400 bytes
//
// 2. model_rgb_buffer
//      96x96x3 = 27,648 bytes
//
// This avoids allocating/freeing these buffers every loop.
// ============================================================

uint8_t *raw_rgb_buffer   = nullptr;
uint8_t *model_rgb_buffer = nullptr;


// Edge Impulse reads from this buffer
static uint8_t *ei_input_buffer = nullptr;


// ============================================================
// CAMERA CONFIG
// ============================================================

camera_config_t camera_config;


// ============================================================
// EDGE IMPULSE DATA CALLBACK
// ============================================================

static int ei_camera_get_data(
    size_t offset,
    size_t length,
    float *out_ptr
)
{
    /*
     * Edge Impulse asks for 'length' pixels beginning
     * at pixel 'offset'.
     *
     * Our buffer contains:
     *
     * R G B | R G B | R G B ...
     */

    size_t pixel_ix = offset * 3;

    for (size_t i = 0; i < length; i++)
    {
        uint8_t r = ei_input_buffer[pixel_ix + 0];
        uint8_t g = ei_input_buffer[pixel_ix + 1];
        uint8_t b = ei_input_buffer[pixel_ix + 2];

        /*
         * Pack RGB888 into a 24-bit value.
         *
         * Edge Impulse image DSP expects this format.
         */
        out_ptr[i] =
            ((uint32_t)r << 16) |
            ((uint32_t)g << 8)  |
            ((uint32_t)b);

        pixel_ix += 3;
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
        "  Free heap : %u bytes\n",
        ESP.getFreeHeap()
    );

    Serial.printf(
        "  Free PSRAM: %u bytes\n",
        ESP.getFreePsram()
    );

    Serial.printf(
        "  Min heap  : %u bytes\n",
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

    // --------------------------------------------------------
    // Check PSRAM
    // --------------------------------------------------------

    if (!psramFound())
    {
        Serial.println(
            "ERROR: PSRAM NOT FOUND!"
        );

        Serial.println(
            "The Edge Impulse image model needs PSRAM."
        );

        return false;
    }

    Serial.println(
        "PSRAM: FOUND"
    );

    Serial.printf(
        "PSRAM size: %u bytes\n",
        ESP.getPsramSize()
    );


    // --------------------------------------------------------
    // Configure camera
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


    // --------------------------------------------------------
    // JPEG from camera
    // --------------------------------------------------------

    camera_config.pixel_format =
        PIXFORMAT_JPEG;


    // Keep resolution LOW while debugging.
    camera_config.frame_size =
        FRAMESIZE_QVGA;


    /*
     * JPEG quality:
     *
     * 0 = highest quality / biggest frame
     * 63 = lowest quality / smallest frame
     */
    camera_config.jpeg_quality =
        15;


    /*
     * Two frame buffers in PSRAM.
     */
    camera_config.fb_count =
        2;


    camera_config.fb_location =
        CAMERA_FB_IN_PSRAM;


    /*
     * Always prefer the newest frame.
     */
    camera_config.grab_mode =
        CAMERA_GRAB_LATEST;


    // --------------------------------------------------------
    // Initialize camera
    // --------------------------------------------------------

    Serial.println(
        "Calling esp_camera_init()..."
    );

    esp_err_t err =
        esp_camera_init(&camera_config);


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
    // Get camera sensor
    // --------------------------------------------------------

    sensor_t *sensor =
        esp_camera_sensor_get();


    if (sensor == nullptr)
    {
        Serial.println(
            "ERROR: esp_camera_sensor_get() returned NULL"
        );

        return false;
    }


    // --------------------------------------------------------
    // Print sensor information
    // --------------------------------------------------------

    Serial.println();
    Serial.println(
        "CAMERA SENSOR INFORMATION:"
    );

    Serial.printf(
        "  PID: 0x%02X\n",
        sensor->id.PID
    );

    Serial.printf(
        "  VER: 0x%02X\n",
        sensor->id.VER
    );

    Serial.printf(
        "  MIDH: 0x%02X\n",
        sensor->id.MIDH
    );

    Serial.printf(
        "  MIDL: 0x%02X\n",
        sensor->id.MIDL
    );


    // --------------------------------------------------------
    // Orientation
    // --------------------------------------------------------

    sensor->set_vflip(
        sensor,
        1
    );

    sensor->set_hmirror(
        sensor,
        0
    );


    Serial.println(
        "Camera sensor ready."
    );


    return true;
}


// ============================================================
// ALLOCATE IMAGE BUFFERS
// ============================================================

bool allocate_buffers()
{
    Serial.println();
    Serial.println(
        "================================"
    );

    Serial.println(
        "ALLOCATING IMAGE BUFFERS"
    );

    Serial.println(
        "================================"
    );


    const size_t raw_size =
        CAMERA_WIDTH *
        CAMERA_HEIGHT *
        CAMERA_CHANNELS;


    const size_t model_size =
        EI_WIDTH *
        EI_HEIGHT *
        3;


    Serial.printf(
        "Raw image buffer : %u bytes\n",
        (unsigned)raw_size
    );

    Serial.printf(
        "Model image buffer: %u bytes\n",
        (unsigned)model_size
    );


    // --------------------------------------------------------
    // Allocate raw camera RGB888 buffer
    // --------------------------------------------------------

    raw_rgb_buffer =
        (uint8_t *)ps_malloc(
            raw_size
        );


    if (raw_rgb_buffer == nullptr)
    {
        Serial.println(
            "ERROR: raw_rgb_buffer allocation FAILED"
        );

        return false;
    }


    // --------------------------------------------------------
    // Allocate Edge Impulse input buffer
    // --------------------------------------------------------

    model_rgb_buffer =
        (uint8_t *)ps_malloc(
            model_size
        );


    if (model_rgb_buffer == nullptr)
    {
        Serial.println(
            "ERROR: model_rgb_buffer allocation FAILED"
        );

        return false;
    }


    Serial.println(
        "Buffers allocated successfully."
    );


    print_memory();


    return true;
}


// ============================================================
// CAPTURE CAMERA FRAME
// ============================================================

bool capture_camera_image()
{
    Serial.println();
    Serial.println(
        "Capturing camera frame..."
    );


    // --------------------------------------------------------
    // Ask camera driver for a frame
    // --------------------------------------------------------

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
    // Convert JPEG -> RGB888
    // --------------------------------------------------------

    Serial.println(
        "Converting JPEG -> RGB888..."
    );


    bool converted =
        fmt2rgb888(
            fb->buf,
            fb->len,
            PIXFORMAT_JPEG,
            raw_rgb_buffer
        );


    /*
     * RETURN CAMERA BUFFER IMMEDIATELY.
     *
     * Very important.
     *
     * If this is forgotten, the camera can eventually
     * run out of frame buffers.
     */
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
    // --------------------------------------------------------

    Serial.printf(
        "Resizing %ux%u -> %ux%u...\n",
        CAMERA_WIDTH,
        CAMERA_HEIGHT,
        EI_WIDTH,
        EI_HEIGHT
    );


    int resize_result =
        ei::image::processing::
        crop_and_interpolate_rgb888(
            model_rgb_buffer,
            CAMERA_WIDTH,
            CAMERA_HEIGHT,
            raw_rgb_buffer,
            EI_WIDTH,
            EI_HEIGHT
        );


    if (resize_result != 0)
    {
        Serial.printf(
            "ERROR: Image resize failed: %d\n",
            resize_result
        );

        return false;
    }


    Serial.println(
        "Resize SUCCESS"
    );


    /*
     * Edge Impulse will read from this buffer.
     */
    ei_input_buffer =
        model_rgb_buffer;


    return true;
}


// ============================================================
// RUN EDGE IMPULSE
// ============================================================

bool run_edge_impulse()
{
    Serial.println();
    Serial.println(
        "================================"
    );

    Serial.println(
        "EDGE IMPULSE INFERENCE"
    );

    Serial.println(
        "================================"
    );


    // --------------------------------------------------------
    // Create Edge Impulse signal
    // --------------------------------------------------------

    ei::signal_t signal;


    signal.total_length =
        EI_WIDTH * EI_HEIGHT;


    signal.get_data =
        &ei_camera_get_data;


    // --------------------------------------------------------
    // Run classifier
    // --------------------------------------------------------

    ei_impulse_result_t result =
        {};


    Serial.println(
        "Calling run_classifier()..."
    );


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


    Serial.printf(
        "run_classifier() returned: %d\n",
        err
    );


    Serial.printf(
        "Inference time: %lu ms\n",
        elapsed
    );


    if (err != EI_IMPULSE_OK)
    {
        Serial.printf(
            "ERROR: Edge Impulse inference failed: %d\n",
            err
        );

        return false;
    }


    // ========================================================
    // FOMO OBJECT DETECTION
    // ========================================================

#if EI_CLASSIFIER_OBJECT_DETECTION == 1

    Serial.println();
    Serial.println(
        "FOMO OBJECT DETECTIONS:"
    );


    bool detected =
        false;


    for (
        uint32_t i = 0;
        i < result.bounding_boxes_count;
        i++
    )
    {
        ei_impulse_result_bounding_box_t bb =
            result.bounding_boxes[i];


        if (bb.value <= 0.0f)
        {
            continue;
        }


        detected =
            true;


        Serial.printf(
            "--------------------------------\n"
        );


        Serial.printf(
            "LABEL      : %s\n",
            bb.label
        );


        Serial.printf(
            "CONFIDENCE : %.2f %%\n",
            bb.value * 100.0f
        );


        Serial.printf(
            "X          : %u\n",
            bb.x
        );


        Serial.printf(
            "Y          : %u\n",
            bb.y
        );


        Serial.printf(
            "WIDTH      : %u\n",
            bb.width
        );


        Serial.printf(
            "HEIGHT     : %u\n",
            bb.height
        );


        // ----------------------------------------------------
        // Check specifically for Alan
        // ----------------------------------------------------

        if (
            strcmp(
                bb.label,
                "Alan"
            ) == 0
        )
        {
            if (
                bb.value >= 0.50f
            )
            {
                Serial.println();
                Serial.println(
                    "********************************"
                );

                Serial.println(
                    "*** ALAN DETECTED! ***"
                );

                Serial.println(
                    "********************************"
                );
            }
        }
    }


    if (!detected)
    {
        Serial.println(
            "No objects detected above threshold."
        );
    }


#endif


    // ========================================================
    // TIMING
    // ========================================================

    Serial.println();
    Serial.println(
        "TIMING:"
    );


    Serial.printf(
        "  DSP            : %d ms\n",
        result.timing.dsp
    );


    Serial.printf(
        "  Classification  : %d ms\n",
        result.timing.classification
    );


    Serial.printf(
        "  Anomaly         : %d ms\n",
        result.timing.anomaly
    );


    // ========================================================
    // MEMORY
    // ========================================================

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
        "EDGE IMPULSE CAMERA TEST"
    );

    Serial.println(
        "========================================"
    );


    // --------------------------------------------------------
    // ESP information
    // --------------------------------------------------------

    Serial.printf(
        "Chip: %s\n",
        ESP.getChipModel()
    );


    Serial.printf(
        "Chip revision: %d\n",
        ESP.getChipRevision()
    );


    Serial.printf(
        "CPU: %u MHz\n",
        ESP.getCpuFreqMHz()
    );


    Serial.printf(
        "PSRAM found: %s\n",
        psramFound()
            ? "YES"
            : "NO"
    );


    Serial.printf(
        "Flash size: %u bytes\n",
        ESP.getFlashChipSize()
    );


    print_memory();


    // --------------------------------------------------------
    // Initialize camera
    // --------------------------------------------------------

    if (!init_camera())
    {
        Serial.println();
        Serial.println(
            "********************************"
        );

        Serial.println(
            "CAMERA INITIALIZATION FAILED"
        );

        Serial.println(
            "********************************"
        );


        while (true)
        {
            delay(1000);
        }
    }


    // --------------------------------------------------------
    // Allocate buffers
    // --------------------------------------------------------

    if (!allocate_buffers())
    {
        Serial.println(
            "FATAL: Buffer allocation failed."
        );


        while (true)
        {
            delay(1000);
        }
    }


    // --------------------------------------------------------
    // Print Edge Impulse model info
    // --------------------------------------------------------

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
        "Project ID: %d\n",
        EI_CLASSIFIER_PROJECT_ID
    );


    Serial.printf(
        "Input width : %d\n",
        EI_CLASSIFIER_INPUT_WIDTH
    );


    Serial.printf(
        "Input height: %d\n",
        EI_CLASSIFIER_INPUT_HEIGHT
    );


    Serial.printf(
        "Label count : %d\n",
        EI_CLASSIFIER_LABEL_COUNT
    );


    Serial.printf(
        "Object detection: %s\n",
        EI_CLASSIFIER_OBJECT_DETECTION
            ? "YES"
            : "NO"
    );


    Serial.printf(
        "Threshold: %.2f\n",
        EI_CLASSIFIER_OBJECT_DETECTION_THRESHOLD
    );


    Serial.println(
        "Labels:"
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
        "Starting camera/inference test..."
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
    // Edge Impulse inference
    // --------------------------------------------------------

    run_edge_impulse();


    Serial.println();
    Serial.println(
        "========================================"
    );

    Serial.println(
        "Waiting for next frame..."
    );

    Serial.println(
        "========================================"
    );


    /*
     * Run about once per second while testing.
     */
    delay(1000);
}