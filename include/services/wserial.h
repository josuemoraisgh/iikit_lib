#pragma once
// wserial.h — UDP (AsyncUDP) header-only, com CONNECT/DISCONNECT
// Use: wserial::beginUDP(47268);  wserial::loopUDP();  wserial::sendLineTo("msg\n");
#include <Arduino.h>
#include <WiFi.h>
#include <AsyncUDP.h>

#define BAUD_RATE 115200
#define NEWLINE "\r\n"

namespace wserial {
  namespace detail {
    IPAddress lasecPlotIP;
    uint16_t  lasecPlotReceivePort = 0;
    uint16_t listenPort = 0;
    bool isUdpAvailable = false;
    bool isUdpLinked = false;

    AsyncUDP udp;
    std::function<void(std::string)> on_input;

    template <typename T>
    void sendLine(const T &txt) {
      if (isUdpLinked) {
        String line = String(txt); // alocação no heap aqui
        udp.writeTo(reinterpret_cast<const uint8_t*>(line.c_str()),
                    line.length(), lasecPlotIP, lasecPlotReceivePort);
      } else {
        Serial.print(txt);
      }
    }

    inline void sendLine(const char *txt, size_t len) {
      if (isUdpLinked) {
          udp.writeTo(reinterpret_cast<const uint8_t*>(txt),
                      len, lasecPlotIP, lasecPlotReceivePort);
      } else {
        // 2) Tenta Serial apenas se tiver espaço imediato
        size_t free = Serial.availableForWrite();
        if (free > len) {
            Serial.write(reinterpret_cast<const uint8_t*>(txt), len);
        }
      }
    }

    bool parseHostPort(const String &s,String &cmd, String &host, uint16_t &port) {
      int c1 = s.indexOf(':');      // primeiro ':'
      int c2 = s.lastIndexOf(':');  // último ':'

      if (c1 <= 0 || c2 <= c1) return false;

      cmd  = s.substring(0, c1);
      host = s.substring(c1 + 1, c2);

      long v = s.substring(c2 + 1).toInt();
      if (v <= 0 || v > 65535) return false;
      port = (uint16_t)v;
      return true;
    }

    void handleOnPacket(AsyncUDPPacket packet) {
      String s((const char*)packet.data(), packet.length());
      s.trim();
      
      String cmd, host;
      uint16_t port;

      if(!parseHostPort(s,cmd,host,port)) { 
        on_input(std::string(s.c_str()));
        return;
      }

      // Seta o lasecPlotIP 
      IPAddress ip;
      if (!ip.fromString(host)) {
        if (WiFi.hostByName(host.c_str(), ip) != 1) {
          Serial.printf("[UDP] DNS fail: %s\n", host.c_str());
          return;
        }
      } 
      if (ip == IPAddress()) { Serial.println("[UDP] Invalid IP"); return; }

      lasecPlotIP = ip;
      lasecPlotReceivePort = port;   // Seta o lasecPlotReceivePort 

      if (cmd == "CONNECT") { // s = "CONNECT:<LASECPLOT_IP>:<LASECPLOT_RECIVE_PORT>"
        isUdpLinked = true;
        const String txt = "CONNECT:" + WiFi.localIP().toString() + ":" + String(lasecPlotReceivePort) + "\n";
        sendLine(txt);
        Serial.printf("[UDP] Linked to %s:%u (OK sent)\n", lasecPlotIP.toString().c_str(), lasecPlotReceivePort);
        return;
      } else {
        if (cmd == "DISCONNECT"){ // Envia DISCONNECT:<LASECPLOT_IP>:<LASECPLOT_RECIVE_PORT> para o alvo atual (se houver)
          if (isUdpLinked) {
            const String txt = "DISCONNECT:" + WiFi.localIP().toString() + ":" + String(lasecPlotReceivePort) + "\n";
            sendLine(txt);
            Serial.printf("[UDP] Linked to %s:%u (BYE sent)\n", lasecPlotIP.toString().c_str(), lasecPlotReceivePort);
            isUdpLinked = false;
            return;
          }
        }
      }
    }
  }
  
  void setup(unsigned long baudrate = BAUD_RATE, uint16_t port=47268) {
    using namespace detail;
    Serial.begin(baudrate);
    while (!Serial)
      delay(1);

    listenPort = port;
    // Tenta listen até conseguir
    if (udp.listen(listenPort)) {
      isUdpAvailable = true;
      udp.onPacket(handleOnPacket);
      Serial.println("[UDP] Listening on " + String(listenPort));
    } else {
      isUdpAvailable = false;
      Serial.println("[UDP] listen() failed");
    }
  }

  void loop() {
    using namespace detail;
    // Se o listen falhou no setup, tente novamente de tempos em tempos
    static uint32_t lastRetry = 0;
    if (!isUdpAvailable && (millis() - lastRetry > 2000)) {
      lastRetry = millis();
      if (udp.listen(listenPort)) {
        isUdpAvailable = true;
        udp.onPacket(handleOnPacket);
        Serial.println("[UDP] Listening on " + String(listenPort) + " (retry ok)");
      }
    }
    if(Serial.available()){
      String linha = Serial.readStringUntil('\n'); // Lê até '\n'
      on_input(linha.c_str());
    }
  }
  void onInputReceived(std::function<void(std::string)> callback) { detail::on_input = callback; }

  // === API pública ===
  template<typename T>
  void plot(const char *varName, uint32_t dt_ms, const T* y, size_t ylen, const char *unit=nullptr)
  {
    // Estima tamanho máximo (muito seguro)
    // varName(30) + ylen * (12 chars?) + unit(10)
    const size_t MAX_SZ = 64 + ylen * 32;
    char *buf = (char*)malloc(MAX_SZ);
    if (!buf) return;

    size_t pos = 0;

    // Prefixo
    pos += snprintf(buf + pos, MAX_SZ - pos, ">%s:", varName);

    static uint32_t base = 0;
    for (size_t i = 0; i < ylen; i++)
    {
        // dt (sempre decimal)
        pos += snprintf(buf + pos, MAX_SZ - pos, "%u:", base);

        // valor (convertido com precisão)
        pos += snprintf(buf + pos, MAX_SZ - pos, "%.2f", (double)y[i]);

        base += dt_ms;

        if (i < ylen - 1)
            buf[pos++] = ';';
    }
    if (unit)
        pos += snprintf(buf + pos, MAX_SZ - pos, "§%s", unit);

    // Sufixo final
    pos += snprintf(buf + pos, MAX_SZ - pos, "|g" NEWLINE);
    detail::sendLine(buf, pos);
    free(buf);
  }

  template <typename T>
  void plot(const char *varName, TickType_t x, T y, const char *unit = nullptr) 
  {
    // Máximo possível e seguro:
    // varName (30) + números (20) + unit (10) + overhead
    char buf[96];  
    size_t pos = 0;

    // Prefixo
    pos += snprintf(buf + pos, sizeof(buf) - pos, ">%s:", varName);

    // timestamp
    pos += snprintf(buf + pos, sizeof(buf) - pos, "%u:", (uint32_t)x);

    // valor (converte qualquer T)
    pos += snprintf(buf + pos, sizeof(buf) - pos, "%.2f", (double)y);

    // unidade, se existir
    if (unit)
        pos += snprintf(buf + pos, sizeof(buf) - pos, "§%s", unit);

    // sufixo
    pos += snprintf(buf + pos, sizeof(buf) - pos, "|g" NEWLINE);
    detail::sendLine(buf, pos);
  }

  template <typename T>
  void plot(const char *varName, T y, const char *unit= nullptr)  {
    plot(varName, (TickType_t) xTaskGetTickCount(), y, unit);
  }

  void log(const char *text, uint32_t ts_ms)  {
    if (ts_ms == 0)
      ts_ms = millis();
    String line = String(ts_ms);
    line += ":";
    line += String(text ? text : "");
    line += NEWLINE;
    detail::sendLine(line);
  }
  
  template <typename T>
  inline void println(const T &data)  {
    detail::sendLine(String(data) + NEWLINE);
  }

  template <typename T>
  inline void print(const T &data)  {
    detail::sendLine(data);
  }
  
  inline void println()  {
    detail::sendLine(NEWLINE);
  }
}