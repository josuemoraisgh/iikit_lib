#pragma once
// wserial.h — UDP (AsyncUDP) header-only, com CONNECT/DISCONNECT
// Use: wserial::beginUDP(47268);  wserial::loopUDP();  wserial::sendLineTo("msg\n");
#include <Arduino.h>
#include <WiFi.h>
#include <AsyncUDP.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#define BAUD_RATE 115200
#define NEWLINE "\r\n"

// #ifndef WSR_MAX_PACKET_SIZE
// #define WSR_MAX_PACKET_SIZE 1024
// #endif

// #ifndef WSR_MAX_POINTS_PER_PACKET
// #define WSR_MAX_POINTS_PER_PACKET 4096
// #endif


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

    inline void sendLineRaw(const char *txt, size_t len) {
      if (isUdpLinked) {
          udp.writeTo(reinterpret_cast<const uint8_t*>(txt),
                      len, lasecPlotIP, lasecPlotReceivePort);
      } else {
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
  // void plot(const char* varName, uint32_t dt_ms, const T* y, size_t ylen, const char* unit=nullptr)
  // {
  //     if (!varName || !y || ylen == 0) return;

  //     static uint32_t base = 0;
  //     size_t offset = 0;
  //     alignas(4) unsigned char buf[WSR_MAX_PACKET_SIZE];

  //     while (offset < ylen) {
  //         uint32_t ts0 = base + dt_ms * (uint32_t)offset;
  //         size_t pos = 0;

  //         // Cabeçalho ASCII: >nome:TS0;STEP;
  //         pos += (size_t)snprintf((char*)buf + pos, sizeof(buf) - pos, ">%s:%u;%u;", varName, ts0, dt_ms);

  //         // Espaço reservado p/ sufixo e terminadores
  //         const size_t unit_len = unit ? strlen(unit) : 0;              // bytes do nome da unidade
  //         const size_t tail_len = (unit ? (2 + unit_len) : 0) + 3;      // "§" (2 bytes UTF-8) + unit + "|g\r\n"


  //         // Precisamos de 8 bytes para min/max (float32) + 2 bytes por amostra
  //         size_t room = (pos < sizeof(buf) && sizeof(buf) > pos + tail_len) ? (sizeof(buf) - pos - tail_len) : 0;
  //         if (room < 8) {                 // sem espaço nem para min/max -> envia só cabeçalho e finaliza
  //             buf[pos++] = '|'; buf[pos++] = 'g'; buf[pos++] = '\r'; buf[pos++] = '\n';
  //             detail::sendLineRaw(reinterpret_cast<const char*>(buf), pos);
  //             break;
  //         }

  //         size_t chunk = ylen - offset;
  //         size_t max_by_buf = (room - 8) / 2;                    // 2 bytes por ponto após min/max
  //         if (chunk > WSR_MAX_POINTS_PER_PACKET) chunk = WSR_MAX_POINTS_PER_PACKET;
  //         if (chunk > max_by_buf)               chunk = max_by_buf;
  //         if (chunk == 0) break;

  //         // Calcula min/max do pedaço
  //         float mn = (float)y[offset];
  //         float mx = mn;
  //         for (size_t i = 1; i < chunk; ++i) {
  //             float v = (float)y[offset + i];
  //             if (v < mn) mn = v;
  //             if (v > mx) mx = v;
  //         }
  //         if (!(mx > mn)) mx = mn + 1e-12f; // evita divisão por zero

  //         // Anexa min e max como IEEE-754 float32 (binário)
  //         memcpy(buf + pos, &mn, 4); pos += 4;
  //         memcpy(buf + pos, &mx, 4); pos += 4;

  //         // Quantiza e escreve os 16 bits (binário, little-endian do host)
  //         const float scale = 65535.0f / (mx - mn);
  //         for (size_t i = 0; i < chunk; ++i) {
  //             float v = (float)y[offset + i];
  //             uint32_t q = (uint32_t)lrintf((v - mn) * scale);
  //             if (q > 65535u) q = 65535u;
  //             uint16_t u16 = (uint16_t)q;
  //             memcpy(buf + pos, &u16, 2);
  //             pos += 2;
  //         }

  //         // Unidade opcional (ASCII)
  //         if (unit) {
  //             // UTF-8 de '§' = 0xC2, 0xA7
  //             buf[pos++] = 0xC2;
  //             buf[pos++] = 0xA7;
  //             memcpy(buf + pos, unit, unit_len);
  //             pos += unit_len;
  //         }

  //         // Final
  //         buf[pos++] = '|'; buf[pos++] = 'g'; buf[pos++] = '\r'; buf[pos++] = '\n';

  //         // Envia
  //         detail::sendLineRaw(reinterpret_cast<const char*>(buf), pos);
  //         offset += chunk;
  //     }

  //     base += dt_ms * (uint32_t)ylen;
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