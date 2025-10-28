/**
 * @file adc_dma_config.h
 * @brief Configurações para o ADC e DMA no ESP32.
 *
 * Este arquivo contém uma classe para inicializar e configurar o ADC e DMA
 * no modo contínuo.
 */

#ifndef ADC_DMA_ESP_H
#define ADC_DMA_ESP_H

#include <Arduino.h>
#include <esp_adc/adc_continuous.h> // Biblioteca do ESP-IDF para operações de ADC contínuo.

/**
 * @class AdcDmaConfig
 * @brief Classe para gerenciar a configuração do ADC e DMA.
 */
#define NUM_SAMPLES 1024 ///< Define o tamanho fixo do buffer de amostras do ADC.

class AdcDmaESP {
private:
    adc_continuous_handle_t adc_handle; ///< Handle para gerenciar o ADC contínuo.
    uint32_t bytes_read; ///< Armazena o número de bytes lidos pelo DMA.
    uint16_t adc_buffer[NUM_SAMPLES]; ///< Buffer fixo para armazenar os valores do ADC.

public:
    /**
     * @brief Construtor da classe AdcDmaConfig.
     *
     * Inicializa o handle como NULL e configura os valores padrão para as variáveis.
     */
    AdcDmaConfig() : adc_handle(nullptr), bytes_read(0) {}

    /**
     * @brief Configura o ADC e o DMA para operação contínua.
     *
     * @param sample_rate Taxa de amostragem do ADC em Hertz (Hz).
     * @param adc_channel Canal do ADC utilizado para amostragem.
     * @param bit_width Resolução do ADC em bits.
     *
     * Inicializa o ADC contínuo com o DMA, configurando as amostras,
     * a taxa de amostragem e os parâmetros necessários para operação eficiente.
     */
    void setup(uint32_t sample_rate, adc_channel_t adc_channel, adc_bitwidth_t bit_width) {
        // Configuração dos parâmetros gerais do ADC contínuo, incluindo buffer e quadro de conversão.
        adc_continuous_handle_cfg_t adc_config = {
            .max_store_buf_size = NUM_SAMPLES * sizeof(uint16_t), // Define o tamanho máximo de armazenamento do buffer.
            .conv_frame_size = NUM_SAMPLES // Define o tamanho do quadro de conversão.
        };
        ESP_ERROR_CHECK(adc_continuous_new_handle(&adc_config, &adc_handle)); // Cria e inicializa o handle do ADC contínuo.

        // Configuração do padrão de canal para o ADC.
        adc_digi_pattern_config_t channel_pattern = {
            .atten = ADC_ATTEN_DB_12, // Define atenuação de 12 dB, permitindo leituras de 0 a 3,3V.
            .channel = adc_channel,  // Define o canal do ADC baseado no parâmetro.
            .unit = ADC_UNIT_1,      // Define que será utilizado o ADC1.
            .bit_width = bit_width   // Configura a resolução com base no parâmetro.
        };

        // Configuração adicional do ADC contínuo, incluindo taxa de amostragem e formato de saída.
        adc_continuous_config_t channel_config = {
            .sample_freq_hz = sample_rate, // Define a taxa de amostragem em Hertz com base no parâmetro.
            .conv_mode = ADC_CONV_SINGLE_UNIT_1, // Configura o modo de conversão para usar apenas o ADC1.
            .format = ADC_DIGI_OUTPUT_FORMAT_TYPE1 // Especifica o formato de saída dos dados do ADC.
        };
        channel_config.pattern_num = 1; // Especifica que apenas um padrão de canal será utilizado.
        channel_config.adc_pattern = &channel_pattern; // Associa o padrão configurado ao ADC.

        ESP_ERROR_CHECK(adc_continuous_config(adc_handle, &channel_config)); // Aplica as configurações ao ADC contínuo.
        ESP_ERROR_CHECK(adc_continuous_start(adc_handle)); // Inicia a operação contínua do ADC.
    }

    /**
     * @brief Verifica se há dados prontos no ADC.
     *
     * Esta função tenta ler os valores do ADC para o buffer interno.
     * Caso haja dados suficientes, os bytes lidos serão armazenados em `bytes_read`.
     *
     * @return ESP_OK se a leitura for bem-sucedida, ou um código de erro caso contrário.
     */
    esp_err_t hasData() {
        return adc_continuous_read(adc_handle, (uint8_t *)adc_buffer, NUM_SAMPLES * sizeof(uint16_t), &bytes_read, 0);
    }

    /**
     * @brief Retorna um ponteiro para o buffer de amostras do ADC.
     *
     * @return Ponteiro para o buffer de amostras.
     */
    uint16_t* getBuffer() {
        return adc_buffer;
    }

    /**
     * @brief Retorna o número de amostras lidas no buffer.
     *
     * @return Quantidade de amostras lidas.
     */
    size_t getSampleCount() {
        return bytes_read / sizeof(uint16_t);
    }

    /**
     * @brief Retorna o handle do ADC.
     *
     * @return Handle do ADC contínuo.
     */
    adc_continuous_handle_t getHandle() const {
        return adc_handle;
    }
};

#endif // ADC_DMA_CONFIG_H