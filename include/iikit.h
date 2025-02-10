/**
 * @file IIdKit.h
 * @brief Classe para gerenciamento de dispositivos industriais utilizando ESP32.
 *
 * Esta classe oferece suporte para gerenciamento de entradas digitais, saídas analógicas,
 * comunicação via Wi-Fi, OTA (Over-The-Air), display OLED e outras funcionalidades
 * industriais baseadas em ESP32.
 */

#ifndef __ININDKIT_H
#define __ININDKIT_H

#include <Arduino.h>
#include <HardwareSerial.h>
#include <WiFi.h>
#include <EEPROM.h>
#include <ESPmDNS.h>

#include "services/OTA.h"
#include "services/WSerial_c.h"
#include "services/display_c.h"
#include "services/wifimanager_c.h"
#include "services/ads1115_c.h"
#include "services/hart_c.h"

#include "util/asyncDelay.h"
#include "util/dinDebounce.h"

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
 * @class IIKit_c
 * @brief Classe para gerenciamento do kit industrial.
 */
class IIKit_c
{
private:
    char DDNSName[15] = "inindkit"; ///< Nome do dispositivo para DDNS.
    WifiManager_c wm;               ///< Gerenciador de conexões Wi-Fi.
    ADS1115_c ads;                  ///< Conversor ADC.
    Hart_c hart;

    /**
     * @brief Exibe mensagens de erro e reinicia o dispositivo se necessário.
     * @param error Mensagem de erro.
     * @param restart Indica se o dispositivo deve ser reiniciado.
     */
    void errorMsg(String error, bool restart = true);

public:
    DigitalINDebounce  rtn_1;       ///< Botão de retorno 1.
    DigitalINDebounce  rtn_2;       ///< Botão de retorno 2.
    DigitalINDebounce  push_1;      ///< Botão push 1.
    DigitalINDebounce  push_2;      ///< Botão push 2.
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

void IIKit_c::setup()
{
    /********** Inicializando EEPROM ***********/
    EEPROM.begin(1);
    char idKit[2] = "3";
    // EEPROM.write(0, (uint8_t)idKit[0]);
    // EEPROM.commit();
    idKit[0] = (char)EEPROM.read(0);
    strcat(DDNSName, idKit);    
    /****** Inicializando Telnet|Serial***********/
    startWSerial(&WSerial, 4000 + String(idKit[0]).toInt(), 115200);    
    WSerial.println("Booting");
    hart.setup(&WSerial);
    /********** Inicializando Display ***********/
    if (startDisplay(&disp, def_pin_SDA, def_pin_SCL))
    {
        disp.setText(1, "Inicializando...");
        WSerial.println("Display running");
    }
    else
    {
        errorMsg("Display error.", false);
    }

    delay(50);
    /********** Configurando Wi-Fi ***********/
    WiFi.mode(WIFI_STA);
    wm.start(&WSerial);
    wm.setApName(DDNSName);

    disp.setFuncMode(true);
    disp.setText(1, "Mode: Acces Point", true);
    disp.setText(2, "SSID: AutoConnectAP", true);
    disp.setText(3, "PSWD: ", true);

    if (wm.autoConnect("AutoConnectAP"))
    {
        WSerial.print("\nWifi running - IP:");
        WSerial.println(WiFi.localIP());
        disp.setFuncMode(false);
        disp.setText(1, (WiFi.localIP().toString() + " ID:" + String(idKit[0])).c_str());
        disp.setText(2, DDNSName);
        disp.setText(3, "UFU Mode");
        delay(50);
    }
    else
    {
        errorMsg("Wifi error.\nAP MODE...", false);
    }

    /********** Inicializando OTA ***********/
    OTA::start(DDNSName);

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

    rtn_1.setup(def_pin_RTN1, 50);
    rtn_2.setup(def_pin_RTN2, 50);
    push_1.setup(def_pin_PUSH1, 50);
    push_2.setup(def_pin_PUSH2, 50);

    digitalWrite(def_pin_D1, LOW);
    digitalWrite(def_pin_D2, LOW);        
    digitalWrite(def_pin_D3, LOW);
    digitalWrite(def_pin_D4, LOW);
    digitalWrite(def_pin_RELE, LOW);
    analogWrite(def_pin_PWM, 0);
    analogWrite(def_pin_DAC1, 0);
    analogWrite(def_pin_W4a20_1, 0);

    if (!ads.begin())
    {
        errorMsg("ADS error.", true);
    }
}

void IIKit_c::loop(void)
{
    OTA::handle();
    updateWSerial(&WSerial);
    updateDisplay(&disp);

    if (wm.getPortalRunning())
    {
        wm.process();
    }

    rtn_1.update();
    rtn_2.update();
    push_1.update();
    push_2.update();
}

uint16_t IIKit_c::analogReadPot1(void)
{
    return ads.analogRead(1);
}

uint16_t IIKit_c::analogReadPot2(void)
{
    return ads.analogRead(0);
}

uint16_t IIKit_c::analogRead4a20_1(void)
{
    return ads.analogRead(3);
}

uint16_t IIKit_c::analogRead4a20_2(void)
{
    return ads.analogRead(2);
}

void IIKit_c::errorMsg(String error, bool restart)
{
    WSerial.println(error);
    if (restart)
    {
        WSerial.println("Rebooting now...");
        delay(2000);
        ESP.restart();
    }
}

IIKit_c IIKit;

#endif