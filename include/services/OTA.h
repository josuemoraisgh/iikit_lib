/**
 * @file OTA.h
 * @brief Classe para gerenciamento de atualizações Over-The-Air (OTA) utilizando a biblioteca ArduinoOTA.
 *
 * Esta classe facilita a configuração e o gerenciamento de atualizações OTA em dispositivos compatíveis,
 * permitindo atualizações do firmware e gerenciamento de interrupções durante o processo.
 */

#ifndef OTA_H
#define OTA_H

#include <Arduino.h>
#include <ArduinoOTA.h>
#include <utility>
#include <vector>
#include <numeric>

/**
 * @brief Callback para desabilitar interrupções durante o processo OTA.
 */
static std::function<void()> disableInterruptsCallback = nullptr;

/**
 * @class OTA
 * @brief Classe para gerenciamento de atualizações OTA.
 */
class OTA {
public:
    /**
     * @brief Inicializa o processo OTA.
     *
     * @param hostname Nome do dispositivo na rede.
     * @param password Senha para autenticação OTA (opcional).
     * @param port Porta de comunicação OTA (padrão: 3232).
     * @param interval Intervalo em milissegundos para verificação OTA (opcional).
     */
    static void start(const char *hostname, const char *password = "", unsigned int port = 3232, unsigned int interval = 1000) {
        ArduinoOTA.setPort(port);
        ArduinoOTA.setHostname(hostname);
        ArduinoOTA.setPassword(password);

        ArduinoOTA.onStart([]() {
            if (disableInterruptsCallback == nullptr) {
                Serial.println("\nDisabling all pins");
                disableInterruptsAllPins();
            } else {
                Serial.println("\nRunning custom callback to disable interrupts");
                disableInterruptsCallback();
            }

            String type;
            if (ArduinoOTA.getCommand() == U_FLASH)
                type = "sketch";
            else
                type = "filesystem";

            Serial.println("Start updating " + type);
        });

        ArduinoOTA.onEnd([]() {
            Serial.println("\nEnd");
        });

        ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
            Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
        });

        ArduinoOTA.onError([](ota_error_t error) {
            Serial.printf("Error[%u]: ", error);
            if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
            else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
            else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
            else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
            else if (error == OTA_END_ERROR) Serial.println("End Failed");

            Serial.println("Rebooting...");
            delay(100);
            ESP.restart();
        });

        ArduinoOTA.begin();

        Serial.println("Ready");
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());
    }

    /**
     * @brief Define um callback personalizado para desabilitar interrupções.
     * @param _disableInterruptsCallback Função callback a ser usada.
     */
    static void setDisableInterruptsCallback(std::function<void()> _disableInterruptsCallback) {
        disableInterruptsCallback = std::move(_disableInterruptsCallback);
    }

    /**
     * @brief Define um callback padrão para desabilitar interrupções em pinos especificados.
     * @param _pins Vetor com os pinos a serem desabilitados.
     */
    static void setDisableInterruptsCallback(const std::vector<int> &_pins) {
        disableInterruptsCallback = std::bind(disableInterruptsDefaultCallback, _pins);
    }

    /**
     * @brief Executa o loop de verificação OTA.
     */
    static void handle() {
        ArduinoOTA.handle();
    }

private:
    /**
     * @brief Desabilita interrupções em todos os pinos.
     */
    static void disableInterruptsAllPins() {
        std::vector<int> pins(40);
        std::iota(pins.begin(), pins.end(), 0);
        disableInterruptsDefaultCallback(pins);
    }

    /**
     * @brief Callback padrão para desabilitar interrupções em pinos.
     * @param pins Vetor com os pinos a serem desabilitados.
     */
    static void disableInterruptsDefaultCallback(const std::vector<int> &pins) {
        for (int pin: pins) {
            detachInterrupt(pin);
        }
    }
};

#endif // OTA_H