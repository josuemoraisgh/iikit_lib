//https://docs.espressif.com/projects/arduino-esp32/en/latest/api/timer.html
#include "IIKit.h"   // Biblioteca base do framework Arduino, necessária para funções básicas como Serial e delays.

#define SAMPLE_RATE 1000    // Define a taxa de amostragem, neste caso, 1000 amostras por segundo.
hw_timer_t *timer = nullptr; // Configuração global do timer

AsyncDelay_c blinkLED(500); // time mili second
void blinkLEDFunc(uint8_t pin) {
  if (blinkLED.isExpired()) {
    digitalWrite(pin, !digitalRead(pin));
  }
}

AsyncDelay_c delayPOT(50); // time mili second
void managerInputFunc(void) {
  if (delayPOT.isExpired()) {
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
}

void IRAM_ATTR timerCallback() { // Função chamada pelo timer
    blinkLEDFunc(def_pin_D1);
    managerInputFunc();
}

// Configuração inicial do programa
void setup() {
    IIKit.setup();
    pinMode(def_pin_D1, OUTPUT);
    // Configuração do timer de hardware
    timer = timerBegin(SAMPLE_RATE); // Inicializa o timer com a frequência de amostragem.
    timerAttachInterrupt(timer, &timerCallback); // Associa a função timerCallback como a ISR do timer.
    timerAlarm(timer,  (80000000 / 80) / SAMPLE_RATE, true, 0);//  // Set alarm to call onTimer (value in microseconds).
}

// Loop principal
void loop() {
  IIKit.loop();
  // blinkLEDFunc(def_pin_D1);
  // managerInputFunc();    
}