#pragma once
#include <Arduino.h>
#include <driver/i2s.h>
#include <driver/adc.h>

/*
 * Simple, safe and optimized ADC DMA engine for ESP32
 * - No malloc
 * - No tasks
 * - No locks
 * - Ring buffer for stable acquisition
 * - Callback always receives small fixed-size blocks
 * - Works in normal loop() without freezing
 */

class SimpleADC_DMA {

public:
    using CallbackFunc = void(*)(int16_t*, size_t);

private:
    // -------------------------
    // DMA parameters
    // -------------------------
    static constexpr size_t BUFFER_LEN  = 256;      // bloco enviado ao callback
    static constexpr size_t DMA_BUFFERS = 4;        // buffers do driver I2S
    static constexpr size_t DMA_SIZE    = BUFFER_LEN * DMA_BUFFERS;

    int _sample_rate     = 0;
    int _adc_channel     = -1;
    bool _adc_fallback_mode = false;

    uint32_t _callbackPeriod = 1000; // micros entre callbacks
    uint32_t _last_plot = 0;

    CallbackFunc _callbackFunc = nullptr;

    // DMA read buffer
    int16_t dma_buffer[DMA_SIZE];

    // fallback ADC buffer (modo manual)
    int16_t fallback_buffer[BUFFER_LEN];

    // -------------------------
    // Ring buffer estático
    // -------------------------
    static constexpr size_t RING_SAMPLES = DMA_SIZE * 2;
    int16_t ring_buffer[RING_SAMPLES];
    volatile size_t ring_head = 0;

public:

    // ---------------------------------------------------------
    // Inicia o ADC via I2S (modo DMA)
    // ---------------------------------------------------------
    bool begin(adc1_channel_t channel,
               int sample_rate_hz,
               uint32_t callback_period_us,
               i2s_port_t port = I2S_NUM_0)
    {
        _adc_channel = channel;
        _sample_rate = sample_rate_hz;
        _callbackPeriod = callback_period_us;

        // Configuração do ADC
        adc1_config_width(ADC_WIDTH_BIT_12);
        adc1_config_channel_atten(channel, ADC_ATTEN_DB_12);

        // -------------------------
        // Configuração do I2S
        // -------------------------
        i2s_config_t i2s_config = {};
        i2s_config.mode = (i2s_mode_t)(I2S_MODE_MASTER |
                                       I2S_MODE_RX |
                                       I2S_MODE_ADC_BUILT_IN);
        i2s_config.sample_rate = _sample_rate;
        i2s_config.bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT;
        i2s_config.channel_format = I2S_CHANNEL_FMT_ONLY_RIGHT;
        i2s_config.communication_format = I2S_COMM_FORMAT_STAND_I2S;
        i2s_config.dma_buf_count = DMA_BUFFERS;
        i2s_config.dma_buf_len   = BUFFER_LEN;
        i2s_config.use_apll = false;

        if (i2s_driver_install(port, &i2s_config, 0, nullptr) != ESP_OK)
            return false;

        if (i2s_set_adc_mode(ADC_UNIT_1, channel) != ESP_OK)
            return false;

        if (i2s_adc_enable(port) != ESP_OK)
            return false;

        _adc_fallback_mode = false;
        _last_plot = micros();

        return true;
    }

    // ---------------------------------------------------------
    // Define o callback
    // ---------------------------------------------------------
    void onData(CallbackFunc cb) {
        _callbackFunc = cb;
    }

    // ---------------------------------------------------------
    // Rotina a ser chamada no loop()
    // ---------------------------------------------------------
    void adcDmaLoop() {

        // ----- Parte 1: adquirir dados -----
        if (!_adc_fallback_mode) {

            size_t bytes_read = 0;
            esp_err_t err = i2s_read(I2S_NUM_0,
                                     dma_buffer,
                                     sizeof(dma_buffer),
                                     &bytes_read,
                                     10 /* timeout */);

            if (err == ESP_OK && bytes_read > 0) {

                size_t count = bytes_read / sizeof(int16_t);

                // Copia para ring buffer
                for (size_t i = 0; i < count; i++) {
                    ring_buffer[ring_head] = dma_buffer[i];
                    ring_head = (ring_head + 1) % RING_SAMPLES;
                }
            }
        }
        else {
            // fallback ADC modo manual
            for (size_t i = 0; i < BUFFER_LEN; i++) {
                int16_t v = adc1_get_raw((adc1_channel_t)_adc_channel);
                fallback_buffer[i] = v;
                ring_buffer[ring_head] = v;
                ring_head = (ring_head + 1) % RING_SAMPLES;
            }
        }

        // ----- Parte 2: callback periódico -----
        uint32_t now = micros();
        if (_callbackFunc && (now - _last_plot >= _callbackPeriod)) {

            static int16_t small_block[BUFFER_LEN];

            // copia bloco pequeno recente do ring
            for (size_t i = 0; i < BUFFER_LEN; i++) {
                size_t idx = (ring_head + RING_SAMPLES - BUFFER_LEN + i) % RING_SAMPLES;
                small_block[i] = ring_buffer[idx];
            }

            _callbackFunc(small_block, BUFFER_LEN);
            _last_plot = now;
        }

        // Alivia watchdog
        vTaskDelay(1);
    }
};