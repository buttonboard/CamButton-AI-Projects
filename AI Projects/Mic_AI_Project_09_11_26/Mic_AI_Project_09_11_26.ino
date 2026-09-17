#include <Arduino.h>
#include "driver/i2s.h"

// ============================================================
// EDGE IMPULSE AUDIO MODEL
// ============================================================

#include <MicProject_inferencing.h>


// ============================================================
// MICROPHONE PINS
// ============================================================

#define MIC_CLK_PIN   42
#define MIC_DATA_PIN  41

#define I2S_PORT      I2S_NUM_0

#define SAMPLE_RATE   16000

// Small microphone capture buffer
#define BUFFER_SIZE   256


// ============================================================
// EDGE IMPULSE AUDIO BUFFER
// ============================================================
//
// 16,000 samples at 16 kHz = 1 second
//
// int16_t = 2 bytes
//
// 16,000 × 2 = 32,000 bytes
// ============================================================

static int16_t inference_buffer[EI_CLASSIFIER_RAW_SAMPLE_COUNT];


// Temporary I2S buffer
static int16_t audioBuffer[BUFFER_SIZE];


// ============================================================
// VARIABLES
// ============================================================

size_t samplesCollected = 0;


// ============================================================
// EDGE IMPULSE DATA CALLBACK
// ============================================================
//
// Edge Impulse asks:
//
// "Give me samples starting at offset for length samples."
//
// We return the samples from inference_buffer.
// ============================================================

static int get_audio_data(
    size_t offset,
    size_t length,
    float *out_ptr
)
{
    if ((offset + length) > EI_CLASSIFIER_RAW_SAMPLE_COUNT)
    {
        return -1;
    }

    for (size_t i = 0; i < length; i++)
    {
        // Convert int16 audio into float
        //
        // Edge Impulse expects audio values
        // approximately in the range -1.0 to +1.0

        out_ptr[i] =
            (float)inference_buffer[offset + i]
            / 32768.0f;
    }

    return 0;
}


// ============================================================
// PRINT MODEL INFORMATION
// ============================================================

void print_model_info()
{
    Serial.println();
    Serial.println("========================================");
    Serial.println("EDGE IMPULSE AUDIO MODEL");
    Serial.println("========================================");

    Serial.printf(
        "Project ID       : %d\n",
        EI_CLASSIFIER_PROJECT_ID
    );

    Serial.printf(
        "Sample rate      : %d Hz\n",
        EI_CLASSIFIER_FREQUENCY
    );

    Serial.printf(
        "Sample count     : %d\n",
        EI_CLASSIFIER_RAW_SAMPLE_COUNT
    );

    Serial.printf(
        "Expected time    : %.2f seconds\n",
        (float)EI_CLASSIFIER_RAW_SAMPLE_COUNT /
        EI_CLASSIFIER_FREQUENCY
    );

    Serial.printf(
        "Number of labels : %d\n",
        EI_CLASSIFIER_LABEL_COUNT
    );

    Serial.println();
    Serial.println("LABELS:");

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

    Serial.println("========================================");
    Serial.println();
}


// ============================================================
// SETUP MICROPHONE
// ============================================================

bool setup_microphone()
{
    Serial.println();
    Serial.println("========================================");
    Serial.println("SETTING UP PDM MICROPHONE");
    Serial.println("========================================");

    Serial.printf(
        "Clock pin : GPIO %d\n",
        MIC_CLK_PIN
    );

    Serial.printf(
        "Data pin  : GPIO %d\n",
        MIC_DATA_PIN
    );

    Serial.printf(
        "I2S port  : %d\n",
        I2S_PORT
    );

    Serial.printf(
        "Rate      : %d Hz\n",
        SAMPLE_RATE
    );


    // ========================================================
    // I2S CONFIGURATION
    // ========================================================

    i2s_config_t config;

    memset(
        &config,
        0,
        sizeof(config)
    );


    config.mode =
        (i2s_mode_t)(
            I2S_MODE_MASTER |
            I2S_MODE_RX |
            I2S_MODE_PDM
        );


    config.sample_rate =
        SAMPLE_RATE;


    config.bits_per_sample =
        I2S_BITS_PER_SAMPLE_16BIT;


    config.channel_format =
        I2S_CHANNEL_FMT_ONLY_LEFT;


    config.communication_format =
        I2S_COMM_FORMAT_I2S;


    config.intr_alloc_flags = 0;


    config.dma_buf_count = 4;


    config.dma_buf_len = BUFFER_SIZE;


    config.use_apll = false;


    config.tx_desc_auto_clear = false;


    config.fixed_mclk = 0;


    // ========================================================
    // INSTALL I2S DRIVER
    // ========================================================

    Serial.println(
        "Installing PDM I2S driver..."
    );


    esp_err_t result =
        i2s_driver_install(
            I2S_PORT,
            &config,
            0,
            NULL
        );


    Serial.printf(
        "i2s_driver_install() = %d\n",
        result
    );


    if (result != ESP_OK)
    {
        Serial.println(
            "ERROR: I2S driver installation failed!"
        );

        return false;
    }


    Serial.println(
        "I2S driver installed successfully."
    );


    // ========================================================
    // PIN CONFIGURATION
    // ========================================================

    i2s_pin_config_t pin_config;

    memset(
        &pin_config,
        0,
        sizeof(pin_config)
    );


    // PDM does NOT use BCLK in the normal way

    pin_config.bck_io_num =
        I2S_PIN_NO_CHANGE;


    // PDM microphone clock

    pin_config.ws_io_num =
        MIC_CLK_PIN;


    // No microphone data output

    pin_config.data_out_num =
        I2S_PIN_NO_CHANGE;


    // Microphone data

    pin_config.data_in_num =
        MIC_DATA_PIN;


    Serial.println(
        "Configuring microphone pins..."
    );


    result =
        i2s_set_pin(
            I2S_PORT,
            &pin_config
        );


    Serial.printf(
        "i2s_set_pin() = %d\n",
        result
    );


    if (result != ESP_OK)
    {
        Serial.println(
            "ERROR: Microphone pin configuration failed!"
        );

        return false;
    }


    Serial.println(
        "Microphone pins configured successfully."
    );


    // ========================================================
    // START I2S
    // ========================================================

    result =
        i2s_start(I2S_PORT);


    Serial.printf(
        "i2s_start() = %d\n",
        result
    );


    if (result != ESP_OK)
    {
        Serial.println(
            "ERROR: Could not start I2S!"
        );

        return false;
    }


    // Clear old DMA data

    i2s_zero_dma_buffer(
        I2S_PORT
    );


    Serial.println(
        "PDM MICROPHONE READY!"
    );

    return true;
}


// ============================================================
// CAPTURE ONE SECOND OF AUDIO
// ============================================================

bool capture_audio()
{
    Serial.println();
    Serial.println("========================================");
    Serial.println("RECORDING AUDIO");
    Serial.println("========================================");

    Serial.println(
        "Speak now..."
    );


    samplesCollected = 0;


    unsigned long startTime =
        millis();


    // ========================================================
    // Keep reading until we have 16,000 samples
    // ========================================================

    while (
        samplesCollected <
        EI_CLASSIFIER_RAW_SAMPLE_COUNT
    )
    {
        size_t bytesRead = 0;


        // ====================================================
        // Read microphone
        // ====================================================

        esp_err_t result =
            i2s_read(
                I2S_PORT,
                audioBuffer,
                sizeof(audioBuffer),
                &bytesRead,
                pdMS_TO_TICKS(1000)
            );


        if (result != ESP_OK)
        {
            Serial.printf(
                "ERROR: i2s_read() = %d\n",
                result
            );

            return false;
        }


        // Number of int16 samples received

        size_t samplesRead =
            bytesRead /
            sizeof(int16_t);


        if (samplesRead == 0)
        {
            continue;
        }


        // ====================================================
        // Copy into Edge Impulse buffer
        // ====================================================

        for (
            size_t i = 0;
            i < samplesRead;
            i++
        )
        {
            if (
                samplesCollected <
                EI_CLASSIFIER_RAW_SAMPLE_COUNT
            )
            {
                inference_buffer[
                    samplesCollected
                ] =
                    audioBuffer[i];

                samplesCollected++;
            }
        }
    }


    unsigned long elapsed =
        millis() - startTime;


    Serial.println();

    Serial.printf(
        "Captured %d samples\n",
        samplesCollected
    );

    Serial.printf(
        "Capture time: %lu ms\n",
        elapsed
    );


    // ========================================================
    // Calculate basic audio level
    // ========================================================

    int16_t minimum =
        32767;

    int16_t maximum =
        -32768;

    int64_t sum =
        0;


    for (
        size_t i = 0;
        i < samplesCollected;
        i++
    )
    {
        int16_t sample =
            inference_buffer[i];


        if (sample < minimum)
        {
            minimum = sample;
        }


        if (sample > maximum)
        {
            maximum = sample;
        }


        sum += sample;
    }


    float average =
        (float)sum /
        samplesCollected;


    Serial.println();
    Serial.println("AUDIO LEVEL:");

    Serial.printf(
        "Minimum     : %d\n",
        minimum
    );

    Serial.printf(
        "Maximum     : %d\n",
        maximum
    );

    Serial.printf(
        "Peak-to-peak: %d\n",
        maximum - minimum
    );

    Serial.printf(
        "DC average  : %.2f\n",
        average
    );


    return true;
}


// ============================================================
// RUN EDGE IMPULSE
// ============================================================

bool run_audio_classifier()
{
    Serial.println();
    Serial.println("========================================");
    Serial.println("RUNNING EDGE IMPULSE");
    Serial.println("========================================");


    // ========================================================
    // Create signal
    // ========================================================

    signal_t signal;


    signal.total_length =
        EI_CLASSIFIER_RAW_SAMPLE_COUNT;


    signal.get_data =
        &get_audio_data;


    // ========================================================
    // Create result object
    // ========================================================

    ei_impulse_result_t result =
        {};


    Serial.println(
        "Calling run_classifier()..."
    );


    unsigned long startTime =
        millis();


    // ========================================================
    // RUN MODEL
    // ========================================================

    EI_IMPULSE_ERROR res =
        run_classifier(
            &signal,
            &result,
            false
        );


    unsigned long elapsed =
        millis() - startTime;


    Serial.printf(
        "run_classifier() returned: %d\n",
        res
    );


    Serial.printf(
        "Inference time: %lu ms\n",
        elapsed
    );


    // ========================================================
    // Check for error
    // ========================================================

    if (res != EI_IMPULSE_OK)
    {
        Serial.println();
        Serial.println(
            "ERROR: Edge Impulse inference failed!"
        );

        return false;
    }


    // ========================================================
    // PRINT CLASSIFICATION
    // ========================================================

    Serial.println();
    Serial.println("========================================");
    Serial.println("VOICE RECOGNITION RESULTS");
    Serial.println("========================================");


    float bestConfidence =
        0.0f;


    const char *bestLabel =
        "unknown";


    for (
        size_t i = 0;
        i < EI_CLASSIFIER_LABEL_COUNT;
        i++
    )
    {
        const char *label =
            result.classification[i].label;


        float confidence =
            result.classification[i].value;


        Serial.printf(
            "%s: %.2f%%\n",
            label,
            confidence * 100.0f
        );


        // Find highest confidence

        if (
            confidence >
            bestConfidence
        )
        {
            bestConfidence =
                confidence;

            bestLabel =
                label;
        }
    }


    // ========================================================
    // FINAL PREDICTION
    // ========================================================

    Serial.println();
    Serial.println("----------------------------------------");

    Serial.printf(
        "PREDICTION: %s\n",
        bestLabel
    );

    Serial.printf(
        "CONFIDENCE: %.2f%%\n",
        bestConfidence * 100.0f
    );

    Serial.println("----------------------------------------");


    // ========================================================
    // Confidence threshold
    // ========================================================

    if (bestConfidence >= 0.50f)
    {
        Serial.println();
        Serial.println(
            ">>> CONFIDENT VOICE DETECTION <<<"
        );

        Serial.printf(
            ">>> %s <<<\n",
            bestLabel
        );
    }
    else
    {
        Serial.println();
        Serial.println(
            ">>> UNKNOWN / LOW CONFIDENCE <<<"
        );
    }


    // ========================================================
    // TIMING
    // ========================================================

    Serial.println();
    Serial.println("TIMING:");

    Serial.printf(
        "DSP            : %d ms\n",
        result.timing.dsp
    );

    Serial.printf(
        "Classification  : %d ms\n",
        result.timing.classification
    );

    Serial.printf(
        "Anomaly         : %d ms\n",
        result.timing.anomaly
    );


    return true;
}


// ============================================================
// SETUP
// ============================================================

void setup()
{
    Serial.begin(115200);


    delay(1500);


    Serial.println();
    Serial.println();
    Serial.println("========================================");
    Serial.println("ESP32-S3 VOICE RECOGNITION");
    Serial.println("========================================");


    // ========================================================
    // Print model information
    // ========================================================

    print_model_info();


    // ========================================================
    // Setup microphone
    // ========================================================

    if (!setup_microphone())
    {
        Serial.println();
        Serial.println(
            "FATAL: Microphone setup failed."
        );

        while (true)
        {
            delay(1000);
        }
    }


    Serial.println();
    Serial.println(
        "SYSTEM READY!"
    );

    Serial.println(
        "The microphone will record 1 second"
    );

    Serial.println(
        "of audio and classify it."
    );

    Serial.println();
}


// ============================================================
// LOOP
// ============================================================

void loop()
{
    // ========================================================
    // Capture one second
    // ========================================================

    if (!capture_audio())
    {
        Serial.println(
            "Audio capture failed."
        );

        delay(1000);

        return;
    }


    // ========================================================
    // Run Edge Impulse
    // ========================================================

    run_audio_classifier();


    // ========================================================
    // Wait before next recording
    // ========================================================

    Serial.println();
    Serial.println(
        "Waiting 1 second..."
    );

    delay(1000);
}