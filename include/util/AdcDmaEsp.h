/**
 * @file AdcDmaEsp.h
 * @author josuemorais
 * @version 1.3
 * @date 2024-07-04
 *
 * @brief Driver para leitura de ADC via I2S com DMA no ESP32, com fallback automático para leitura direta se MAC de simulação Wokwi for detectado.
 *
 * Esta biblioteca permite a leitura de dados analógicos usando o periférico I2S
 * com DMA para aquisição de alta performance. Caso seja detectado o endereço MAC
 * 24:0A:C4:00:01:10 (simulador Wokwi), ativa o modo fallback para leitura direta do ADC.
 */

#ifndef ADCDMAESP_H
#define ADCDMAESP_H

#include <driver/i2s.h>
#include <Arduino.h>
#include <esp_system.h>  // Necessário para esp_read_mac
#include <esp_wifi.h>    // Para ESP_MAC_WIFI_STA

#define CHANNEL_ADC1 ADC1_CHANNEL_0
#define CHANNEL_ADC2 ADC1_CHANNEL_3

// Número de buffers DMA (padrão: 4)
#ifndef DMA_BUFFERS
#define DMA_BUFFERS 4
#endif

// Tamanho de cada buffer (em amostras; padrão: 64)
#ifndef BUFFER_LEN
#define BUFFER_LEN 64
#endif

/**
 * @brief Tipo de callback para tratar os dados ADC.
 *
 * O callback deve receber um ponteiro para os dados de 16 bits lidos e o número de amostras.
 */
typedef void (*CallbackADC)(const int16_t *data, size_t count);

/** Callback que será invocado quando os dados forem adquiridos */
CallbackADC _callbackFunc;

/**
 * @brief Buffer DMA para armazenamento dos dados lidos.
 *
 * Alinhado a 16 bytes para compatibilidade com o DMA.
 */
__attribute__((aligned(16))) int16_t dma_buffer[DMA_BUFFERS * BUFFER_LEN];

/**
 * @brief Buffer alternativo para fallback (leitura direta do ADC).
 */
int16_t fallback_buffer[BUFFER_LEN];

/** Intervalo de plotagem em microsegundos */
uint32_t _callbackPeriod = 0;
/** Última vez que os dados foram plotados (micros) */
uint32_t _last_plot = 0;
/** Canal atual do ADC utilizado */
int _adc_channel = CHANNEL_ADC1;
/** Modo fallback: leitura direta do ADC se I2S/DMA indisponível ou MAC simulada detectada */
bool _adc_fallback_mode = false;

/**
 * @brief Detecta se o endereço MAC corresponde ao ambiente simulado Wokwi.
 *
 * @return true se MAC for igual a 24:0A:C4:00:01:10, false caso contrário.
 */
bool detectWokwiByMac()
{
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);

    // Exibe o MAC detectado para debug
    Serial.printf("MAC detectado: %02X:%02X:%02X:%02X:%02X:%02X\n",
                  mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    // Verifica se o MAC é 24:0A:C4:00:01:10 (Wokwi padrão)
    return (mac[0] == 0x24 && mac[1] == 0x0A && mac[2] == 0xC4 && mac[3] == 0x00 && mac[4] == 0x01 && mac[5] == 0x10) ||
           (mac[0] == 0x00 && mac[1] == 0x00 && mac[2] == 0x00 && mac[3] == 0x00 && mac[4] == 0x00 && mac[5] == 0x00);
}

/**
 * @brief Configura o ADC via DMA utilizando o I2S built-in do ESP32.
 *
 * Tenta configurar o ADC (ADC1) para utilizar um canal específico e o driver I2S no modo ADC built-in.
 * Caso o periférico não seja possível ou se a MAC de simulação for detectada, ativa automaticamente o modo fallback
 * com leitura direta do ADC no loop.
 *
 * @param channel Canal ADC (do tipo adc1_channel_t) a ser utilizado (ex: ADC1_CHANNEL_0, ADC1_CHANNEL_3, etc.).
 * @param samplePeriod Taxa de amostragem em microsegundos do DMA (padrão: 1000 us).
 * @param callbackFunc Função de callback que será invocada com os dados adquiridos do DMA até aquele instante.
 * @param callbackPeriod Intervalo de execução do callback em microsegundos (padrão: 100000 us).
 * @param width_bit Largura dos bits para conversão ADC (padrão ADC_WIDTH_BIT_12).
 */
void adcDmaSetup(
    adc1_channel_t channel,
    uint32_t samplePeriod = 1000UL,
    CallbackADC callbackFunc = nullptr,
    uint32_t callbackPeriod = 100000UL,
    adc_bits_width_t width_bit = ADC_WIDTH_BIT_12)
{
    _callbackFunc = callbackFunc;
    _callbackPeriod = callbackPeriod;
    _adc_channel = channel;
    _adc_fallback_mode = false; // Tenta modo DMA/I2S

    Serial.begin(115200);  // Garante que Serial está iniciado
    delay(100);  // Pequeno delay para estabilizar o sistema antes de ler MAC

    // Detecta ambiente Wokwi pelo MAC address
    if (detectWokwiByMac()) {
        //Serial.println("MAC 24:0A:C4:00:01:10 detectado! Ativando fallback para leitura direta do ADC.");
        _adc_fallback_mode = true;
        return;
    }

    const uint32_t sample_rate = (uint32_t)1000000UL / samplePeriod;
    adc_power_acquire();
    adc1_config_width(width_bit);
    adc1_config_channel_atten(channel, ADC_ATTEN_DB_12);

    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_ADC_BUILT_IN),
        .sample_rate = sample_rate,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_ONLY_RIGHT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = DMA_BUFFERS,
        .dma_buf_len = BUFFER_LEN,
        .use_apll = false,
        .tx_desc_auto_clear = false,
        .fixed_mclk = 0,
        .mclk_multiple = I2S_MCLK_MULTIPLE_256};

    esp_err_t res = i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);

    if (res == ESP_OK) {
        i2s_set_adc_mode(ADC_UNIT_1, channel);
        i2s_adc_enable(I2S_NUM_0);
        _adc_fallback_mode = false;
    } else {
        Serial.println("WARN: I2S DMA não disponível. Usando fallback de leitura direta do ADC.");
        _adc_fallback_mode = true;
    }
}

/**
 * @brief Loop de aquisição e callback.
 *
 * Esta função deve ser chamada periodicamente no loop principal. Ela lê os dados do ADC
 * via I2S utilizando DMA e, se o intervalo de plotagem tiver decorrido, invoca o callback com os dados.
 * Caso o modo fallback esteja ativo (I2S não disponível ou MAC simulada detectada), realiza leituras diretas do ADC para simular um buffer.
 */
void adcDmaLoop()
{
    if (_callbackFunc != nullptr)
    {
        if (micros() - _last_plot >= _callbackPeriod)
        {
            if (!_adc_fallback_mode) {
                size_t bytes_read;
                esp_err_t err = i2s_read(I2S_NUM_0, dma_buffer, sizeof(dma_buffer), &bytes_read, 0);
                if (err == ESP_OK) {
                    _callbackFunc(dma_buffer, (uint16_t)(bytes_read / sizeof(int16_t)));
                }
            } else {
                for (int i = 0; i < BUFFER_LEN; ++i) {
                    fallback_buffer[i] = adc1_get_raw((adc1_channel_t)_adc_channel);
                }
                _callbackFunc(fallback_buffer, BUFFER_LEN);
            }
            _last_plot = micros();
        }
    }
}

#endif // ADCDMAESP_H