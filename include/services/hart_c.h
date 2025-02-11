/**
 * @file Hart.h
 * @brief Declaração da classe Hart para comunicação full‐duplex via Telnet e Serial2.
 *
 * Esta classe configura a Serial2 e inicia um servidor Telnet utilizando a biblioteca ESP Telnet by
 * Lennart Hennigs, realizando a transferência de dados entre a Serial2 e os clientes Telnet.
 */

 #ifndef HART_H
 #define HART_H
 
 #include <Arduino.h>
 #include <ESPTelnet.h>
 #include <stdarg.h>
 
 /**
  * @brief Classe que implementa uma ponte full‐duplex entre a Serial2 e clientes Telnet.
  *
  * Inicializa a Serial2 com os parâmetros informados e gerencia o servidor Telnet.
  * Os dados recebidos na Serial2 são enviados para os clientes Telnet e vice‐versa.
  */
 class Hart {
 public:
     /**
      * @brief Construtor da classe Hart.
      *
      */
     Hart(){};
 
     /**
      * @brief Inicializa a Serial2 e o servidor Telnet.
      *
      * Este método inicia a Serial2 com a configuração especificada e inicia o servidor Telnet
      * na porta informada (padrão: 23).
      *
      * @param port Porta para o servidor Telnet (padrão: 23).
      * @param baud Taxa de transmissão para a Serial2 (padrão: 115200).
      */
     void setup(uint16_t port = 4100, uint32_t baud = 115200);
 
     /**
      * @brief Processa a transferência de dados entre a Serial2 e os clientes Telnet.
      *
      * Deve ser chamado periodicamente (por exemplo, dentro do loop()) para:
      * - Atualizar as conexões Telnet;
      * - Transferir dados da Serial2 para os clientes Telnet;
      * - Transferir dados dos clientes Telnet para a Serial2.
      */
     void loop();
 
 private:
     uint32_t _baud;   ///< Taxa de transmissão para a Serial2.
     uint16_t _port;
     ESPTelnet _telnet; ///< Instância da biblioteca ESP Telnet by Lennart Hennigs.
 };
 
 #endif // HART_H

void Hart::setup(uint16_t port, uint32_t baud) {
    // Inicializa a Serial2 com a taxa e os pinos configurados.
    Serial2.begin(_baud, SERIAL_8N1, 16, 17);
    // Inicializa o servidor Telnet na porta especificada (padrão: 23).
    _telnet.begin(port);
}

void Hart::loop() {
    // Processa as conexões e a comunicação dos clientes Telnet.
    _telnet.handleClient();
    // --- Dados da Serial2 para os clientes Telnet ---
    while (Serial2.available()) {
        char c = Serial2.read();
        _telnet.write(c);
    }
    // --- Dados dos clientes Telnet para a Serial2 ---
    while (_telnet.available()) {
        char c = _telnet.read();
        Serial2.write(c);
    }
}