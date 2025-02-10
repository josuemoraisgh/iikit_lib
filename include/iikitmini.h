/**
 * @file IIKitmini.h
 * @brief Classe para gerenciamento de dispositivos industriais utilizando ESP32.
 *
 * Esta classe oferece suporte para gerenciamento de entradas digitais, saídas analógicas,
 * comunicação via Wi-Fi, OTA (Over-The-Air), display OLED e outras funcionalidades
 * industriais baseadas em ESP32.
 */

#ifndef __IIDKITMINI_H
#define __IIDKITMINI_H

#include <Arduino.h>
#include "services/display_c.h"
#include "services/ads1115_c.h"
#include "services/hart_c.h"

/********** GPIO DEFINITIONS ***********/
#define def_pin_ADC1 39    ///< GPIO para entrada ADC1.
#define def_pin_RTN2 35    ///< GPIO para botão retentivo 2.
#define def_pin_PUSH1 34   ///< GPIO para botão push 1.
#define def_pin_PWM 33     ///< GPIO para saída PWM.
#define def_pin_PUSH2 32    ///< GPIO para botão push 2.
#define def_pin_RELE 27    ///< GPIO para relé.
#define def_pin_W4a20_1 26 ///< GPIO para saída 4-20mA 1.
#define def_pin_DAC1 25    ///< GPIO para saída DAC1.
#define def_pin_D1 23      ///< GPIO para I/O digital 1.
#define def_pin_SCL 22     ///< GPIO para SCL do display OLED.
#define def_pin_SDA 21     ///< GPIO para SDA do display OLED.
#define def_pin_D2 19      ///< GPIO para I/O digital 2.
#define def_pin_D3 18      ///< GPIO para I/O digital 3.
///< GPIO15 - ESP_PROG_TDO:6
///< GPIO14 - ESP_PROG_TMS:2
///< GPIO13 - ESP_PROG_TCK:4
///< GPIO12 - ESP_PROG_TDI:8
#define def_pin_D4 4      ///< GPIO para I/O digital 4.
///< GPIO3  - ESP_COM_TX:3
#define def_pin_RTN1 2    ///< GPIO para botão retentivo 1.
///< GPIO1  - ESP_COM_RX:5
///< GPIO0  - ESP_COM_BOOT:6
///< ESPEN  - ESP_COM_EN:1

/**
 * @class IIKitmini_c
 * @brief Classe para gerenciamento do kit industrial sem wifi.
 */
class IIKitmini_c
{
private:
    ADS1115_c ads;                  ///< Conversor ADC.
    Hart_c hart;

public:
    Display_c disp;    ///< Display OLED.
    WSerial_c WSerial; ///< Conexão Telnet e Serial.

    /**
     * @brief Inicializa o kit industrial.
     */
    void setup();

    /**
     * @brief Executa o loop principal do kit industrial.
     */
    void loop(void);

    /**
     * @brief Lê o valor do potenciômetro 1.
     * @return Valor analógico do potenciômetro 1.
     */
    uint16_t analogReadPot1(void);

    /**
     * @brief Lê o valor do potenciômetro 2.
     * @return Valor analógico do potenciômetro 2.
     */
    uint16_t analogReadPot2(void);

    /**
     * @brief Lê o valor do canal 4-20mA 1.
     * @return Valor analógico do canal 4-20mA 1.
     */
    uint16_t analogRead4a20_1(void);

    /**
     * @brief Lê o valor do canal 4-20mA 2.
     * @return Valor analógico do canal 4-20mA 2.
     */
    uint16_t analogRead4a20_2(void);
};

void IIKitmini_c::setup()
{
    /****** Inicializando Telnet|Serial***********/
    startWSerial(&WSerial, 4000, 115200UL);  
    WSerial.println("Booting");
    hart.setup(&WSerial);
    /********** Inicializando Display ***********/
    if (startDisplay(&disp, def_pin_SDA, def_pin_SCL))
    {
        disp.setText(1, "Inicializando...");
        WSerial.println("Display running");
    }
    delay(50);
    /********** Configurando Wi-Fi ***********/
    disp.setFuncMode(false);
    disp.setText(1, "Mode: sem WIFI", false);
    /********** Configurando GPIOs ***********/
    pinMode(def_pin_RTN1, INPUT_PULLDOWN);
    pinMode(def_pin_RTN2, INPUT_PULLDOWN);
    pinMode(def_pin_PUSH1, INPUT_PULLDOWN);
    pinMode(def_pin_PUSH2, INPUT_PULLDOWN);
    pinMode(def_pin_D1, OUTPUT);
    pinMode(def_pin_D2, OUTPUT);
    pinMode(def_pin_D3, OUTPUT);
    pinMode(def_pin_D4, OUTPUT);
    pinMode(def_pin_PWM, OUTPUT);
    pinMode(def_pin_DAC1, ANALOG);
    pinMode(def_pin_ADC1, ANALOG);
    pinMode(def_pin_RELE, OUTPUT);
    pinMode(def_pin_W4a20_1, OUTPUT);
    digitalWrite(def_pin_D1, LOW);
    digitalWrite(def_pin_D2, LOW);        
    digitalWrite(def_pin_D3, LOW);
    digitalWrite(def_pin_D4, LOW);
    digitalWrite(def_pin_RELE, LOW);
    analogWrite(def_pin_PWM, 0);
    analogWrite(def_pin_DAC1, 0);
    analogWrite(def_pin_W4a20_1, 0);
    ads.begin();
}

void IIKitmini_c::loop(void)
{
    updateWSerial(&WSerial);
    updateDisplay(&disp);
}

uint16_t IIKitmini_c::analogReadPot1(void)
{
    return ads.analogRead(1);
}

uint16_t IIKitmini_c::analogReadPot2(void)
{
    return ads.analogRead(0);
}

uint16_t IIKitmini_c::analogRead4a20_1(void)
{
    return ads.analogRead(3);
}

uint16_t IIKitmini_c::analogRead4a20_2(void)
{
    return ads.analogRead(2);
}

IIKitmini_c IIKit;

#endif