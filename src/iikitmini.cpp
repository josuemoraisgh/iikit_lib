#include "iikitmini.h"

void IIKitmini_c::setup()
{
    /****** Inicializando Telnet|Serial***********/
    startWSerial(&WSerial, 4000, 115200UL);  
    WSerial.println("Booting");
    hart.setup(&WSerial);
    /********** Inicializando Display ***********/
    if (startDisplay(&disp, def_pin_SDA, def_pin_SCL))
    {
        disp.setText(1, "Inicializando...");
        WSerial.println("Display running");
    }
    delay(50);
    /********** Configurando Wi-Fi ***********/
    disp.setFuncMode(false);
    disp.setText(1, "Mode: sem WIFI", false);
    /********** Configurando GPIOs ***********/
    pinMode(def_pin_RTN1, INPUT_PULLDOWN);
    pinMode(def_pin_RTN2, INPUT_PULLDOWN);
    pinMode(def_pin_PUSH1, INPUT_PULLDOWN);
    pinMode(def_pin_PUSH2, INPUT_PULLDOWN);
    pinMode(def_pin_D1, OUTPUT);
    pinMode(def_pin_D2, OUTPUT);
    pinMode(def_pin_D3, OUTPUT);
    pinMode(def_pin_D4, OUTPUT);
    pinMode(def_pin_PWM, OUTPUT);
    pinMode(def_pin_DAC1, ANALOG);
    pinMode(def_pin_ADC1, ANALOG);
    pinMode(def_pin_RELE, OUTPUT);
    pinMode(def_pin_W4a20_1, OUTPUT);
    digitalWrite(def_pin_D1, LOW);
    digitalWrite(def_pin_D2, LOW);        
    digitalWrite(def_pin_D3, LOW);
    digitalWrite(def_pin_D4, LOW);
    digitalWrite(def_pin_RELE, LOW);
    analogWrite(def_pin_PWM, 0);
    analogWrite(def_pin_DAC1, 0);
    analogWrite(def_pin_W4a20_1, 0);
    ads.begin();
}

void IIKitmini_c::loop(void)
{
    updateWSerial(&WSerial);
    updateDisplay(&disp);
}

uint16_t IIKitmini_c::analogReadPot1(void)
{
    return ads.analogRead(1);
}

uint16_t IIKitmini_c::analogReadPot2(void)
{
    return ads.analogRead(0);
}

uint16_t IIKitmini_c::analogRead4a20_1(void)
{
    return ads.analogRead(3);
}

uint16_t IIKitmini_c::analogRead4a20_2(void)
{
    return ads.analogRead(2);
}