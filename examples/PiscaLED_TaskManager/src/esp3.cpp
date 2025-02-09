//https://docs.espressif.com/projects/arduino-esp32/en/latest/api/timer.html
#include <iikitmini.h>  // Biblioteca base do framework Arduino, necessária para funções básicas como Serial e delays.
#define NUMTASKS 5
#include "util/jtask.h"
#include "util/dinDebounce.h"

//Funçao de alterar o estado de um led
void blinkLEDFunc(uint8_t pin) {
    digitalWrite(pin, !digitalRead(pin));
}

//Função que le os valores dos POT e das Entradas 4 a 20 mA e plota no display
void managerInputFunc(void) {
    const uint16_t vlPOT1 = IIKit.analogReadPot1();
    const uint16_t vlPOT2 = IIKit.analogReadPot2();
    const uint16_t vlR4a20_1 = IIKit.analogRead4a20_1();
    const uint16_t vlR4a20_2 = IIKit.analogRead4a20_2();

    IIKit.disp.setText(2, ("P1:" + String(vlPOT1) + "  P2:" + String(vlPOT2)).c_str());
    IIKit.disp.setText(3, ("T1:" + String(vlR4a20_1) + "  T2:" + String(vlR4a20_2)).c_str());

    IIKit.WSerial.plot("vlPOT1", vlPOT1);
    IIKit.WSerial.plot("vlPOT2", vlPOT2);
    IIKit.WSerial.plot("vlR4a20_1", vlR4a20_1);
    IIKit.WSerial.plot("vlR4a20_2", vlR4a20_2);
}

//DigitalDebounce RTN1(def_pin_RTN1, 50, [](bool state){digitalWrite(def_pin_D1, state);});
//DigitalDebounce RTN2(def_pin_RTN2, 50, [](bool state){digitalWrite(def_pin_D2, state);});
DigitalINDebounce PUSH1(def_pin_PUSH1, 50, [](bool state){digitalWrite(def_pin_D3, state);});
DigitalINDebounce PUSH2(def_pin_PUSH2, 50, [](bool state){digitalWrite(def_pin_D4, state);});

// Configuração inicial do programa
void setup() {
    // Faz as configuções do hardware ESP + Perifericos
    IIKit.setup();
    jtaskSetup();    // Configura o timer para 1000 Hz (1 ms)
    jtaskAttachFunc(managerInputFunc, 50000UL); //anexa um função e sua base de tempo para ser executada
    jtaskAttachFunc([](){blinkLEDFunc(def_pin_D1);}, 500000UL);  //anexa um função e sua base de tempo para ser executada
    jtaskAttachFunc([](){blinkLEDFunc(def_pin_D2);}, 1000000UL);  //anexa um função e sua base de tempo para ser executada
    //jtaskAttachFunc([](){blinkLEDFunc(def_pin_D3);}, 1500000UL);  //anexa um função e sua base de tempo para ser executada
}

// Loop principal
void loop() {
  // Monitora os perifericos
  IIKit.loop();
  jtaskLoop();
  //RTN1.update();  // Atualiza a leitura com debounce
  //RTN2.update();  // Atualiza a leitura com debounce 
  PUSH1.update();  // Atualiza a leitura com debounce
  PUSH2.update();  // Atualiza a leitura com debounce   
}