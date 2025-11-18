#pragma once
// wserial.h — UDP (AsyncUDP) header-only, com CONNECT/DISCONNECT
// Use: wserial::beginUDP(47268);  wserial::loopUDP();  wserial::sendLineTo("msg\n");
#include <Arduino.h>
#include <WiFi.h>
#include <AsyncUDP.h>

#define BAUD_RATE 115200
#define NEWLINE "\r\n"
// máximo de valores enviados em cada pacote UDP
#ifndef WSR_MAX_POINTS_PER_PACKET
#define WSR_MAX_POINTS_PER_PACKET 128
#endif

// tamanho máximo calculado do pacote
#ifndef WSR_MAX_PACKET_SIZE
#define WSR_MAX_PACKET_SIZE (64 + (WSR_MAX_POINTS_PER_PACKET * 32))
#endif
namespace wserial {
  namespace detail {
    IPAddress lasecPlotIP;
    uint16_t  lasecPlotReceivePort = 0;
    uint16_t listenPort = 0;
    bool isUdpAvailable = false;
    bool isUdpLinked = false;

    AsyncUDP udp;
    std::function<void(std::string)> on_input;

    bool isSerialConnected() {
        // RX0 na ESP32 é GPIO3
        int rx = digitalRead(3);

        // RX flutuante → sem cabo USB / sem monitor serial
        // RX estável em HIGH ou LOW → cabo USB presente
        static int last = rx;
        static uint32_t lastChange = millis();

        if (rx != last) {
            lastChange = millis();
            last = rx;
        }

        // Se o nível estável por 50ms, consideramos conectado
        return (millis() - lastChange) < 50;
    }

    inline void sendLineRaw(const char *txt, size_t len) {
      if (isUdpLinked) {
          udp.writeTo(reinterpret_cast<const uint8_t*>(txt),
                      len, lasecPlotIP, lasecPlotReceivePort);
      } else if (isSerialConnected()) {
        Serial.write(reinterpret_cast<const uint8_t*>(txt),len);
      }
    }
    
    inline void sendLine(const String &s) {
        sendLineRaw(s.c_str(), s.length());
    }

    inline void sendLine(const char *txt) {
        sendLineRaw(txt, strlen(txt));
    }

    template <typename T>
    inline void sendLine(const T &txt) {
      String s(txt);
      sendLine(s);
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
      if (!varName || !y || ylen == 0) return;
      static uint32_t base = 0;
      size_t offset = 0;
      char buf[WSR_MAX_PACKET_SIZE];  // <<< buffer FIXO, sem malloc

      while (offset < ylen) {
          size_t chunk = ylen - offset;
          if (chunk > WSR_MAX_POINTS_PER_PACKET) chunk = WSR_MAX_POINTS_PER_PACKET;
          size_t pos = 0;
          uint32_t ts0 = base + dt_ms * offset;
          // Cabeçalho: >nome:TS0;STEP;
          pos += snprintf(buf + pos, sizeof(buf) - pos,">%s:%u;%u;",varName, ts0, dt_ms);
          // Valores
          for (size_t i = 0; i < chunk; i++) {
              pos += snprintf(buf + pos, sizeof(buf) - pos,"%.2f", (double)y[offset + i]);
              if (i < chunk - 1) buf[pos++] = ';';
          }
          // Unidade opcional
          if (unit) pos += snprintf(buf + pos, sizeof(buf) - pos, "§%s", unit);
          // Fim
          pos += snprintf(buf + pos, sizeof(buf) - pos, "|g\r\n");
          // Envia
          detail::sendLineRaw(buf, pos);
          // Avança para próximo pedaço
          offset += chunk;
      }
      // Atualiza base (primeiro timestamp do próximo lote)
      base += dt_ms * ylen;
  }


  // template<typename T>
  // void plot(const char *varName, uint32_t dt_ms, const T* y, size_t ylen, const char *unit=nullptr)
  // {
  //     if (!varName || !y || ylen == 0) return;

  //     // Tamanho seguro
  //     const size_t MAX_SZ = 64 + ylen * 32;
  //     char *buf = (char*)malloc(MAX_SZ);
  //     if (!buf) return;

  //     static uint32_t base = 0;

  //     size_t pos = 0;

  //     // Prefixo: >NOME:
  //     pos += snprintf(buf + pos, MAX_SZ - pos, ">%s:%u;%u;", varName, base, dt_ms);

  //     // Valores: VAL1;VAL2;VAL3;...
  //     for (size_t i = 0; i < ylen; i++)
  //     {
  //         // escreve o valor
  //         pos += snprintf(buf + pos, MAX_SZ - pos, "%.2f", (double)y[i]);

  //         if (i < ylen - 1)
  //             buf[pos++] = ';';
  //     }

  //     // Unidade opcional
  //     if (unit)
  //         pos += snprintf(buf + pos, MAX_SZ - pos, "§%s", unit);

  //     // Sufixo de flags e newline
  //     pos += snprintf(buf + pos, MAX_SZ - pos, "|g" NEWLINE);

  //     // Envia
  //     detail::sendLineRaw(buf, pos);

  //     // Atualiza base (primeiro timestamp da próxima chamada)
  //     base += dt_ms * ylen;

  //     free(buf);
  // }


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
    detail::sendLineRaw(buf, pos);
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