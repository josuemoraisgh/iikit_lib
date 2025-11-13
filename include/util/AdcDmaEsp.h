#pragma once

#include <Arduino.h>
#include "driver/i2s.h"
#include "driver/adc.h"

/**
 * AdcDmaEsp
 *
 * - Usa I2S + ADC interno do ESP32 em modo DMA contínuo
 * - DMA preenche internamente os buffers do driver
 * - A classe "colhe" esses dados em um buffer circular grande na RAM
 * - No loop() você chama:
 *
 *     adc.poll();   // coleta o que o DMA trouxe
 *     size_t n = adc.read(newSamples, maxSamples); // lê só o que chegou desde a última vez
 *
 * - Sem tasks, sem malloc, sem travar o watchdog.
 */

class AdcDmaEsp {
public:
    // tamanho do buffer temporário (por leitura de DMA)
    static constexpr size_t DMA_TMP_LEN = 256;
    // tamanho total do buffer grande
    static constexpr size_t BIGBUF_LEN  = 8192;

    AdcDmaEsp()
        : _port(I2S_NUM_0),
          _adc_channel(ADC1_CHANNEL_0),
          _sample_rate(1000),
          _started(false),
          _write_pos(0),
          _read_pos(0)
    {}

    bool begin(adc1_channel_t channel,
               int sample_rate_hz,
               i2s_port_t port = I2S_NUM_0)
    {
        _adc_channel = channel;
        _sample_rate = sample_rate_hz;
        _port        = port;

        // Configura ADC1
        adc1_config_width(ADC_WIDTH_BIT_12);
        if (adc1_config_channel_atten(_adc_channel, ADC_ATTEN_DB_12) != ESP_OK) {
            return false;
        }

        // Configura I2S em modo ADC embutido
        i2s_config_t i2s_config = {};
        i2s_config.mode = (i2s_mode_t)(
            I2S_MODE_MASTER |
            I2S_MODE_RX |
            I2S_MODE_ADC_BUILT_IN
        );
        i2s_config.sample_rate = _sample_rate;
        i2s_config.bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT;
        i2s_config.channel_format = I2S_CHANNEL_FMT_ONLY_RIGHT;
        i2s_config.communication_format = I2S_COMM_FORMAT_STAND_I2S;
        i2s_config.dma_buf_count = 4;
        i2s_config.dma_buf_len   = DMA_TMP_LEN;
        i2s_config.use_apll      = false;

        if (i2s_driver_install(_port, &i2s_config, 0, nullptr) != ESP_OK) {
            return false;
        }

        if (i2s_set_adc_mode(ADC_UNIT_1, _adc_channel) != ESP_OK) {
            i2s_driver_uninstall(_port);
            return false;
        }

        if (i2s_adc_enable(_port) != ESP_OK) {
            i2s_driver_uninstall(_port);
            return false;
        }

        _write_pos = 0;
        _read_pos  = 0;
        _started   = true;
        return true;
    }

    void end() {
        if (!_started) return;
        i2s_adc_disable(_port);
        i2s_driver_uninstall(_port);
        _started = false;
    }

    /**
     * poll()
     *
     * Chame MUITAS vezes no loop().
     * Ele coleta tudo o que o DMA já escreveu nos buffers internos
     * e copia para o buffer circular grande.
     */
    void poll() {
        if (!_started) return;

        // buffer temporário na pilha
        int16_t tmp[DMA_TMP_LEN];

        // Limita quantas leituras fazemos por chamada para não travar o loop
        for (int iter = 0; iter < 4; ++iter) {
            size_t bytes_read = 0;

            esp_err_t err = i2s_read(
                _port,
                (void*)tmp,
                sizeof(tmp),
                &bytes_read,
                0  // timeout 0 => não bloqueia
            );

            if (err != ESP_OK || bytes_read == 0) {
                // nada mais para ler agora
                break;
            }

            size_t samples = bytes_read / sizeof(int16_t);
            _pushToBigBuffer(tmp, samples);
        }
    }

    /**
     * read()
     *
     * Copia para 'dest' até 'maxSamples' novas amostras que chegaram desde
     * a última chamada de read().
     *
     * Retorna a quantidade de amostras copiadas.
     */
    size_t read(int16_t* dest, size_t maxSamples) {
        if (!_started) return 0;

        uint32_t available = _write_pos - _read_pos;
        if (available == 0) return 0;

        if (available > BIGBUF_LEN) {
            // consumidor ficou para trás; descarta mais antigas
            _read_pos = _write_pos - BIGBUF_LEN;
            available = BIGBUF_LEN;
        }

        size_t toRead = (available < maxSamples) ? available : maxSamples;

        for (size_t i = 0; i < toRead; ++i) {
            uint32_t idx = (_read_pos + i) % BIGBUF_LEN;
            dest[i] = _bigbuf[idx];
        }

        _read_pos += toRead;
        return toRead;
    }

    /**
     * Quantidade de amostras disponíveis para read()
     */
    size_t available() const {
        return _write_pos - _read_pos;
    }

private:
    i2s_port_t      _port;
    adc1_channel_t  _adc_channel;
    int             _sample_rate;
    bool            _started;

    // buffer grande circular
    int16_t         _bigbuf[BIGBUF_LEN];
    volatile uint32_t _write_pos;  // posição lógica de escrita
    volatile uint32_t _read_pos;   // posição lógica de leitura

    void _pushToBigBuffer(const int16_t* data, size_t count) {
        for (size_t i = 0; i < count; ++i) {
            uint32_t idx = _write_pos % BIGBUF_LEN;
            _bigbuf[idx] = data[i];
            _write_pos++;
        }

        // Se ultrapassou muito, adiante o read_pos para manter só as mais recentes
        uint32_t diff = _write_pos - _read_pos;
        if (diff > BIGBUF_LEN) {
            _read_pos = _write_pos - BIGBUF_LEN;
        }
    }
};