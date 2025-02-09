#include "iikit.h"

void IIKit_c::setup()
{
    /********** Inicializando EEPROM ***********/
    EEPROM.begin(1);
    char idKit[2] = "3";
    // EEPROM.write(0, (uint8_t)idKit[0]);
    // EEPROM.commit();
    idKit[0] = (char)EEPROM.read(0);
    strcat(DDNSName, idKit);    
    /****** Inicializando Telnet|Serial***********/
    startWSerial(&WSerial, 4000 + String(idKit[0]).toInt(), 115200);    
    WSerial.println("Booting");
    hart.setup(&WSerial);
    /********** Inicializando Display ***********/
    if (startDisplay(&disp, def_pin_SDA, def_pin_SCL))
    {
        disp.setText(1, "Inicializando...");
        WSerial.println("Display running");
    }
    else
    {
        errorMsg("Display error.", false);
    }

    delay(50);
    /********** Configurando Wi-Fi ***********/
    WiFi.mode(WIFI_STA);
    wm.start(&WSerial);
    wm.setApName(DDNSName);

    disp.setFuncMode(true);
    disp.setText(1, "Mode: Acces Point", true);
    disp.setText(2, "SSID: AutoConnectAP", true);
    disp.setText(3, "PSWD: ", true);

    if (wm.autoConnect("AutoConnectAP"))
    {
        WSerial.print("\nWifi running - IP:");
        WSerial.println(WiFi.localIP());
        disp.setFuncMode(false);
        disp.setText(1, (WiFi.localIP().toString() + " ID:" + String(idKit[0])).c_str());
        disp.setText(2, DDNSName);
        disp.setText(3, "UFU Mode");
        delay(50);
    }
    else
    {
        errorMsg("Wifi error.\nAP MODE...", false);
    }

    /********** Inicializando OTA ***********/
    OTA::start(DDNSName);

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

    rtn_1.setup(def_pin_RTN1, 50);
    rtn_2.setup(def_pin_RTN2, 50);
    push_1.setup(def_pin_PUSH1, 50);
    push_2.setup(def_pin_PUSH2, 50);

    digitalWrite(def_pin_D1, LOW);
    digitalWrite(def_pin_D2, LOW);        
    digitalWrite(def_pin_D3, LOW);
    digitalWrite(def_pin_D4, LOW);
    digitalWrite(def_pin_RELE, LOW);
    analogWrite(def_pin_PWM, 0);
    analogWrite(def_pin_DAC1, 0);
    analogWrite(def_pin_W4a20_1, 0);

    if (!ads.begin())
    {
        errorMsg("ADS error.", true);
    }
}

void IIKit_c::loop(void)
{
    OTA::handle();
    updateWSerial(&WSerial);
    updateDisplay(&disp);

    if (wm.getPortalRunning())
    {
        wm.process();
    }

    rtn_1.update();
    rtn_2.update();
    push_1.update();
    push_2.update();
}

uint16_t IIKit_c::analogReadPot1(void)
{
    return ads.analogRead(1);
}

uint16_t IIKit_c::analogReadPot2(void)
{
    return ads.analogRead(0);
}

uint16_t IIKit_c::analogRead4a20_1(void)
{
    return ads.analogRead(3);
}

uint16_t IIKit_c::analogRead4a20_2(void)
{
    return ads.analogRead(2);
}

void IIKit_c::errorMsg(String error, bool restart)
{
    WSerial.println(error);
    if (restart)
    {
        WSerial.println("Rebooting now...");
        delay(2000);
        ESP.restart();
    }
}