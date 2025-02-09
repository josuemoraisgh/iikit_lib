// link: https://dronebotworkshop.com/wifimanager/
/**
 * @file WifiManager_c.h
 * @brief Classe para gerenciamento de conexões Wi-Fi utilizando a biblioteca WiFiManager.
 *
 * Esta classe estende a funcionalidade da biblioteca WiFiManager, permitindo
 * configurações adicionais como gerenciamento de portal, reinicialização e integração
 * com a classe WSerial_c para depuração serial.
 */

#include <Arduino.h>
#include "other/WiFiManager-2.0.17/WiFiManager.h"
#include "services/WSerial_c.h"

/**
 * @class WifiManager_c
 * @brief Classe para gerenciamento de Wi-Fi com suporte a portal de configuração sob demanda.
 */
class WifiManager_c : public WiFiManager {
protected:
    WSerial_c* WSerial; ///< Ponteiro para a classe de depuração serial.
    uint8_t trigger_pin; ///< Pino utilizado para acionar o portal.
    uint8_t timeout; ///< Tempo limite para o portal de configuração (em segundos).
    bool portalRunning = false; ///< Indica se o portal de configuração está ativo.
    const char *apName = "OnDemandAP"; ///< Nome do Access Point (AP).

public:
    /**
     * @brief Construtor da classe.
     *
     * @param timeout Tempo limite para o portal de configuração (padrão: 120 segundos).
     */
    WifiManager_c(const uint8_t &timeout = 120) : WiFiManager() {
        this->timeout = timeout;
    }

    /**
     * @brief Inicializa a classe com um objeto WSerial_c para depuração serial.
     * @param ws Ponteiro para o objeto WSerial_c.
     */
    void start(WSerial_c* ws) {
        WSerial = ws;
    }

    /**
     * @brief Define o nome do Access Point (AP).
     * @param apName Nome do Access Point.
     */
    void setApName(const char *apName);

    /**
     * @brief Alterna o estado do portal de configuração web.
     *
     * @return true se o portal foi iniciado, false se foi parado.
     */
    bool changeWebPortal();

    /**
     * @brief Verifica se o portal de configuração está ativo.
     * @return true se o portal está ativo, false caso contrário.
     */
    bool getPortalRunning() {
        return portalRunning;
    }

    /**
     * @brief Reinicia as configurações de Wi-Fi e reinicializa o dispositivo.
     */
    void resetSettingsRestart();
};

/**
 * @brief Reinicia as configurações de Wi-Fi e reinicializa o dispositivo.
 */
void WifiManager_c::resetSettingsRestart() {
    resetSettings();
    WSerial->println("Rebooting now...");
    delay(2000);
    ESP.restart();
    delay(2000);
}

/**
 * @brief Alterna o estado do portal de configuração web.
 *
 * @return true se o portal foi iniciado, false se foi parado.
 */
bool WifiManager_c::changeWebPortal() {
    if (!portalRunning) {
        WSerial->println("Button Pressed, Starting Portal");
        setConfigPortalBlocking(false);
        startWebPortal();
        portalRunning = true;
        return true;
    } else {
        WSerial->println("Button Pressed, Stopping Portal");
        stopWebPortal();
        portalRunning = false;
        return false;
    }
}

/**
 * @brief Define o nome do Access Point (AP).
 * @param apName Nome do Access Point.
 */
void WifiManager_c::setApName(const char *apName) {
    this->apName = apName;
    ((WiFiManager *)this)->setHostname(apName);
}