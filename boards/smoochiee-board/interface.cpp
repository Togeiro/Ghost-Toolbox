#include "core/powerSave.h"

/***************************************************************************************
** Function name: _setup_gpio()
** Location: main.cpp
** Description:   initial setup for the device
***************************************************************************************/

// Power handler for battery detection
#ifdef XPOWERS_CHIP_BQ25896
#include <Wire.h>
#include <XPowersLib.h>
XPowersPPM PPM;
#endif

void _setup_gpio() {

    if (UP_BTN >= 0) pinMode(UP_BTN, INPUT_PULLUP);
    if (SEL_BTN >= 0) pinMode(SEL_BTN, INPUT_PULLUP);
    if (DW_BTN >= 0) pinMode(DW_BTN, INPUT_PULLUP);
    if (R_BTN >= 0) pinMode(R_BTN, INPUT_PULLUP);
    if (L_BTN >= 0) pinMode(L_BTN, INPUT_PULLUP);
#if defined(ESC_BTN) && ESC_BTN >= 0
    pinMode(ESC_BTN, INPUT_PULLUP);
#endif

    pinMode(CC1101_SS_PIN, OUTPUT);
    pinMode(NRF24_SS_PIN, OUTPUT);

    digitalWrite(CC1101_SS_PIN, HIGH);
    digitalWrite(NRF24_SS_PIN, HIGH);
    // Starts SPI instance for CC1101 and NRF24 with CS pins blocking communication at start

    bruceConfig.rfModule = CC1101_SPI_MODULE;
    bruceConfig.irRx = RXLED;
    Wire.setPins(GROVE_SDA, GROVE_SCL);
    // Wire.begin();
    bool pmu_ret = false;
    Wire.begin(GROVE_SDA, GROVE_SCL);
    pmu_ret = PPM.init(Wire, GROVE_SDA, GROVE_SCL, BQ25896_SLAVE_ADDRESS);
    if (pmu_ret) {
        PPM.setSysPowerDownVoltage(3300);
        PPM.setInputCurrentLimit(3250);
        Serial.printf("getInputCurrentLimit: %d mA\n", PPM.getInputCurrentLimit());
        PPM.disableCurrentLimitPin();
        PPM.setChargeTargetVoltage(4208);
        PPM.setPrechargeCurr(64);
        PPM.setChargerConstantCurr(832);
        PPM.getChargerConstantCurr();
        Serial.printf("getChargerConstantCurr: %d mA\n", PPM.getChargerConstantCurr());
        PPM.enableMeasure(PowersBQ25896::CONTINUOUS);
        PPM.disableOTG();
        PPM.enableCharge();
    }
}
	bool isCharging() {
    //PPM.disableBatterPowerPath();
    return PPM.isCharging();
}


int getBattery() {
    int voltage = PPM.getBattVoltage();
    int percent = (voltage - 3300) * 100 / (float)(4150 - 3350);

    if (percent < 0) return 0;
    if (percent > 100) percent = 100;

    if (PPM.isCharging() && percent >= 97) {
        PPM.disableBatLoad();
        percent = 95; // estimate still charging
    }

    if (PPM.isChargeDone()) { percent = 100; }

    return percent;
}




/*********************************************************************
** Function: setBrightness
** location: settings.cpp
** set brightness value
**********************************************************************/
void _setBrightness(uint8_t brightval) {
    if (brightval == 0) {
        analogWrite(TFT_BL, brightval);
    } else {
        int bl = MINBRIGHT + round(((255 - MINBRIGHT) * brightval / 100));
        analogWrite(TFT_BL, bl);
    }
}

/*********************************************************************
** Function: InputHandler
** Handles the variables PrevPress, NextPress, SelPress, AnyKeyPress and EscPress
**********************************************************************/
void InputHandler(void) {
    static unsigned long tm = 0;
    if (millis() - tm < 200 && !LongPress) return;
    bool _u = (UP_BTN >= 0) ? digitalRead(UP_BTN) : true;
    bool _d = (DW_BTN >= 0) ? digitalRead(DW_BTN) : true;
    bool _l = (L_BTN >= 0) ? digitalRead(L_BTN) : true;
    bool _r = (R_BTN >= 0) ? digitalRead(R_BTN) : true;
    bool _s = (SEL_BTN >= 0) ? digitalRead(SEL_BTN) : true;
#if defined(ESC_BTN) && ESC_BTN >= 0
    bool _esc = digitalRead(ESC_BTN);
#endif

    if (!_s || !_u || !_d || !_r || !_l
#if defined(ESC_BTN) && ESC_BTN >= 0
        || !_esc
#endif
    ) {
        tm = millis();
        if (!wakeUpScreen()) AnyKeyPress = true;
        else return;
    }
    if (!_l) { PrevPress = true; }
    if (!_r) { NextPress = true; }
    if (!_u) {
        UpPress = true;
        PrevPagePress = true;
    }
    if (!_d) {
        DownPress = true;
        NextPagePress = true;
    }
    if (!_s) { SelPress = true; }
#if defined(ESC_BTN) && ESC_BTN >= 0
    if (!_esc) {
        EscPress = true;
        NextPress = false;
        PrevPress = false;
    } else
#endif
    if (!_l && !_r) {
        EscPress = true;
        NextPress = false;
        PrevPress = false;
    }
}

/*********************************************************************
** Function: powerOff
** location: mykeyboard.cpp
** Turns off the device (or try to)
**********************************************************************/
void powerOff() {
    esp_sleep_enable_ext0_wakeup((gpio_num_t)SEL_BTN, BTN_ACT);
    esp_deep_sleep_start();
}

/*********************************************************************
** Function: checkReboot
** location: mykeyboard.cpp
** Btn logic to tornoff the device (name is odd btw)
**********************************************************************/
void checkReboot() {
    int countDown;
    /* Long press power off */
    if (digitalRead(L_BTN) == BTN_ACT && digitalRead(R_BTN) == BTN_ACT) {
        uint32_t time_count = millis();
        while (digitalRead(L_BTN) == BTN_ACT && digitalRead(R_BTN) == BTN_ACT) {
            // Display poweroff bar only if holding button
            if (millis() - time_count > 500) {
                tft.setTextSize(1);
                tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
                countDown = (millis() - time_count) / 1000 + 1;
                if (countDown < 4)
                    tft.drawCentreString("PWR OFF IN " + String(countDown) + "/3", tftWidth / 2, 12, 1);
                else {
                    tft.fillScreen(bruceConfig.bgColor);
                    while (digitalRead(L_BTN) == BTN_ACT || digitalRead(R_BTN) == BTN_ACT);
                    delay(200);
                    powerOff();
                }
                delay(10);
            }
        }

        // Clear text after releasing the button
        delay(30);
        tft.fillRect(60, 12, tftWidth - 60, tft.fontHeight(1), bruceConfig.bgColor);
    }
}
