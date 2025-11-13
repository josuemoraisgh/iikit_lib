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
    uint32_t base_ms = 0;

    AsyncUDP udp;
    std::function<void(std::string)> on_input;
    template <typename T>
    void sendLine(const T &txt) {
      if(isUdpLinked) {
        String line = String(txt);
        udp.writeTo(reinterpret_cast<const uint8_t*>(line.c_str()), line.length(), lasecPlotIP, lasecPlotReceivePort);
      }
      else Serial.print(txt);
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
    template <typename T>
  void plot(const char *varName, TickType_t x, T y, const char *unit= nullptr)  {
    // >var:timestamp_ms:valor[§unit]|g\n
    String str(">");
    str += varName;
    str += ":";
    uint32_t ts_ms = (uint32_t)(x);
    if (ts_ms < 100000)
      ts_ms = millis();
    str += String(ts_ms);
    str += ":";
    str += String(y);
    if (unit && unit[0])
    {
      str += "§";
      str += unit;
    }
    str += "|g" NEWLINE;

    detail::sendLine(str);
  }

  template <typename T>
  void plot(const char *varName, T y, const char *unit= nullptr)  {
    plot(varName, (TickType_t) xTaskGetTickCount(), y, unit);
  }

  template<typename T>
  void plot(const char *varName, uint32_t dt_ms, const T* y, size_t ylen, const char *unit)  {
    String str(">");
    str += varName;
    str += ":";

    for (size_t i = 0; i < ylen; i++)
    {
      str += String((uint32_t) detail::base_ms);  // mantém como decimal sem espaços
      str += ":";
      str += String((double)y[i], 6);      // 6 casas decimais
      detail::base_ms += dt_ms; 
      if (i < ylen - 1) str += ";";
    }

    if (unit != nullptr) {
      str += "§";
      str += unit;
    }

    str += "|g" NEWLINE;
    detail::sendLine(str);
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