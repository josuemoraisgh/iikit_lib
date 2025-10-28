#ifndef __WSERIAL_H
#define __WSERIAL_H
#include <Arduino.h>
#include <AsyncTelnet.h>

// === UDP support ===
#include <WiFi.h>
#include <AsyncUDP.h>
#include <WiFiUdp.h>

#define BAUD_RATE 115200
#define NEWLINE "\r\n"

// Protocolo UDP (lado dispositivo):
// 1) LasecPlot envia para CMD_UDP_PORT: "CONNECT:<IP_LOCAL_DO_VSCODE>:<UDP_PORT_DO_VSCODE>"
// 2) Dispositivo responde para (<IP_LOCAL_DO_VSCODE>, <UDP_PORT_DO_VSCODE>): "OK:<IP_DO_DISPOSITIVO>:<CMD_UDP_PORT>\n"
// 3) Telemetria: envia linhas no formato ">var:timestamp_ms:valor|g\n" para (<IP_LOCAL_DO_VSCODE>, <UDP_PORT_DO_VSCODE>)
//    - Para LOG: envia ">:timestamp_ms:Mensagem de log\n"

class WSerial_c
{
protected:
  // Telnet/Serial (legado)
  uint16_t server_port = 0;
  AsyncTelnet *_telnet = nullptr;
  bool isClientConnected = false;

  // UDP
  AsyncUDP _udpCmd;     // escuta CONNECT no CMD_UDP_PORT
  WiFiUDP  _udpData;    // envia dados/OK para o cliente
  bool _udpAvailable = false;   // true se conseguiu iniciar o listener de comando
  bool _udpLinked    = false;   // true após receber CONNECT válido
  uint16_t _cmdUdpPort = 47268; // porta de comando (este dispositivo escuta aqui)

  IPAddress _remoteIP;          // IP remoto (VSCODE/LasecPlot)
  uint16_t  _remoteDataPort=0;  // porta de dados remota (UDP_PORT do VSCode)

  std::function<void(std::string)> on_input;

  void start(uint16_t port, unsigned long baudrate = BAUD_RATE, uint16_t cmdUdpPort = 47268);
  void updateUDP();
  void update();  

  // auxiliares UDP
  void _handleConnectPacket(const char* msg, const IPAddress& senderIP, uint16_t senderPort);
  void _udpSendLine(const String& line);

  // util
  static bool _parseHostPort(const String& s, String& host, uint16_t& port);

public:
  uint16_t serverPort() { return (server_port); }
  void stop();
  friend inline void startWSerial(WSerial_c *ws,uint16_t port, unsigned long baudrate = BAUD_RATE, uint16_t cmdUdpPort = 47268);
  friend inline void updateWSerial(WSerial_c *ws);   

  template <typename T>
  void print(const T &data);

  template <typename T>
  void println(const T &data);
  void println();

  template <typename T>
  void plot(const char *varName, TickType_t x, T y, const char *unit = NULL);
  template <typename T>
  void plot(const char *varName, T y, const char *unit = NULL);

  // Log no mesmo canal (UDP se linkado, senão Serial/Telnet)
  void log(const char* text, uint32_t ts_ms = 0);

  void onInputReceived(std::function<void(std::string)> callback);

  // Estado UDP
  bool udpAvailable() const { return _udpAvailable; }
  bool udpLinked()    const { return _udpLinked; }
  uint16_t cmdUdpPort() const { return _cmdUdpPort; }
};

void WSerial_c::stop()
{
  if (_telnet) {
    _telnet->stop();
  }
  if (_udpAvailable) {
    _udpCmd.close();
  }
  _udpData.stop();
}

inline void startWSerial(WSerial_c *ws,uint16_t port, unsigned long baudrate, uint16_t cmdUdpPort){ws->start(port, baudrate, cmdUdpPort);}

void WSerial_c::start(uint16_t port, unsigned long baudrate, uint16_t cmdUdpPort)
{
  // Reinicia Telnet se já havia
  if (_telnet) {
    _telnet->stop();
    delete (_telnet);
    _telnet = nullptr;
  }
  isClientConnected = false;
  server_port = port;
  Serial.begin(baudrate);

  // Inicia Telnet (para retrocompatibilidade/depuração)
  _telnet = new AsyncTelnet(server_port);
  Serial.println();
  _telnet->onConnect([=](void *, AsyncClient *client)
                     {
                       Serial.println("\nClient connected");
                       isClientConnected = true; });

  _telnet->onDisconnect([=](AsyncClient *client)
                        {
                          Serial.println("\nClient disconnected");
                          isClientConnected = false; });

  _telnet->onIncomingData([=](const std::string &data)
                          { print(data.c_str()); });
  _telnet->begin(false, false);
  println();

  // === UDP CMD listener ===
  _cmdUdpPort = cmdUdpPort;
  _udpAvailable = false;
  _udpLinked = false;
  _remoteDataPort = 0;

  if (WiFi.isConnected()) {
    if (_udpCmd.listen(_cmdUdpPort)) {
      _udpAvailable = true;
      Serial.printf("[UDP] Listening CMD on %u\n", _cmdUdpPort);
      _udpCmd.onPacket([this](AsyncUDPPacket packet){
        // buffer é binário, converte para texto
        String s; s.reserve(packet.length()+1);
        for (size_t i=0;i<packet.length();++i) s += char(packet.data()[i]);
        s.trim();
        _handleConnectPacket(s.c_str(), packet.remoteIP(), packet.remotePort());
      });
    } else {
      Serial.printf("[UDP] Failed to listen on %u\n", _cmdUdpPort);
    }
  } else {
    Serial.println("[UDP] WiFi not connected. Fallback to Serial/Telnet.");
  }
}

inline void updateWSerial(WSerial_c *ws) {ws->update();}

void WSerial_c::update() {
  // mantém entrada Serial se não há telnet
  if(!isClientConnected) {
    if(Serial.available()) {
      if (on_input) on_input(std::string((Serial.readStringUntil('\n')).c_str()));
    }
  }
  // processa UDP (nada a fazer aqui porque AsyncUDP é orientado a callback)
  updateUDP();
}

void WSerial_c::updateUDP() {
  // placeholder para futuros timeouts/keepalive
}

// Trata CONNECT:<IP_LOCAL>:<UDP_PORT>
void WSerial_c::_handleConnectPacket(const char* msg, const IPAddress& senderIP, uint16_t senderPort) {
  if (!msg) return;
  if (strncmp(msg, "CONNECT:", 8) != 0) return;

  // Extrai <IP_LOCAL>:<UDP_PORT> após "CONNECT:"
  const char* p = msg + 8;
  String spec = String(p);
  spec.trim();

  String clientHost;
  uint16_t clientPort = 0;
  if (!_parseHostPort(spec, clientHost, clientPort)) {
    Serial.printf("[UDP] Invalid CONNECT payload: %s\n", msg);
    return;
  }

  // Determina IP do dispositivo (para responder OK)
  IPAddress myIP = WiFi.localIP();
  char myIPStr[16]; snprintf(myIPStr, sizeof(myIPStr), "%u.%u.%u.%u", myIP[0], myIP[1], myIP[2], myIP[3]);

  // Envia OK:<IP_DO_DISPOSITIVO>:<CMD_UDP_PORT> para (clientHost, clientPort)
  String ok = "OK:"; ok += myIPStr; ok += ":"; ok += String(_cmdUdpPort); ok += "\n";

  // Resolve clientHost (IPv4)
  IPAddress clientIP;
  if (!clientIP.fromString(clientHost)) {
    // tenta DNS
    WiFi.hostByName(clientHost.c_str(), clientIP);
  }
  if (!clientIP) {
    Serial.printf("[UDP] Could not resolve client host: %s\n", clientHost.c_str());
    return;
  }

  // Salva destino de dados
  _remoteIP = clientIP;
  _remoteDataPort = clientPort;
  _udpLinked = true;

  // Envia OK e registra
  _udpData.begin(0); // porta local arbitrária para envio
  _udpData.beginPacket(_remoteIP, _remoteDataPort);
  _udpData.write((const uint8_t*)ok.c_str(), ok.length());
  _udpData.endPacket();
  Serial.printf("[UDP] Linked to %s:%u  (OK sent: %s)\n", _remoteIP.toString().c_str(), _remoteDataPort, ok.c_str());
}

void WSerial_c::_udpSendLine(const String& line) {
  if (!_udpLinked) return;
  if (!_remoteIP || _remoteDataPort == 0) return;
  _udpData.beginPacket(_remoteIP, _remoteDataPort);
  _udpData.write((const uint8_t*)line.c_str(), line.length());
  _udpData.endPacket();
}

bool WSerial_c::_parseHostPort(const String& s, String& host, uint16_t& port) {
  // suporta IPv4/DNS na forma "host:port"
  int colon = s.lastIndexOf(':');
  if (colon <= 0) return false;
  host = s.substring(0, colon);
  String p = s.substring(colon+1);
  p.trim();
  long v = p.toInt();
  if (v <= 0 || v > 65535) return false;
  port = (uint16_t)v;
  return true;
}

// === API pública ===

template <typename T>
void WSerial_c::plot(const char *varName, T y, const char *unit)
{
  plot(varName,(TickType_t) xTaskGetTickCount(), y, unit);
}

template <typename T>
void WSerial_c::plot(const char *varName, TickType_t x, T y, const char *unit)
{
  // Monta payload Teleplot/LasecPlot
  // Formato para variável: ">var:timestamp_ms:valor[§unit]|g\n"
  String str(">");
  str.concat(varName);
  str.concat(":");
  // xTaskGetTickCount está em "ticks". Para ms, converta conforme config.
  // Aqui vamos supor que 'x' já seja timestamp em ms. Caso contrário, use (millis()):
  uint32_t ts_ms = (uint32_t)(x);
  if (ts_ms < 100000) ts_ms = millis(); // fallback para ms reais
  str.concat(ts_ms);
  str.concat(":");
  str.concat(y);
  if (unit != NULL && unit[0] != 0) {
    str.concat("§");
    str.concat(unit);
  }
  str.concat("|g");
  str.concat(NEWLINE);

  if (_udpAvailable && _udpLinked) {
    _udpSendLine(str);
  } else {
    // Enquanto não linkar UDP, manda por Serial/Telnet
    println(str);
  }
}

template <typename T>
void WSerial_c::print(const T &data)
{
  if (_udpAvailable && _udpLinked) {
    String s = String(data);
    if (!s.endsWith(NEWLINE)) s += NEWLINE;
    _udpSendLine(s);
    return;
  }
  if (isClientConnected && _telnet)
    _telnet->write(String(data).c_str());
  else
    Serial.print(data);
}

template <typename T>
void WSerial_c::println(const T &data)
{
  String str(data);
  str.concat(NEWLINE);
  print(str);
}

void WSerial_c::println()
{
  if (_udpAvailable && _udpLinked) {
    _udpSendLine(String(NEWLINE));
    return;
  }
  if (isClientConnected && _telnet)
    _telnet->write(NEWLINE);
  else
    Serial.println();
}

void WSerial_c::log(const char* text, uint32_t ts_ms)
{
  if (ts_ms == 0) ts_ms = millis();
  // Formato de LOG: ">:timestamp_ms:Mensagem\n"
  String line = ">:";
  line += String(ts_ms);
  line += ":";
  line += String(text);
  line += "\n";
  if (_udpAvailable && _udpLinked) _udpSendLine(line);
  else println(line);
}

void WSerial_c::onInputReceived(std::function<void(std::string)> callback)
{
  if (_telnet) _telnet->onIncomingData(callback);
  on_input = callback;
}

#endif
