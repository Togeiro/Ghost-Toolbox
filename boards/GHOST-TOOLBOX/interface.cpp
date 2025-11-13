#include "core/powerSave.h"
#include "core/utils.h"
#include <Wire.h>
#include <esp_sleep.h>
#include <globals.h>
#include <interface.h>

namespace {
constexpr uint16_t kTouchCalData[5] = {270, 3620, 320, 3640, 1};
constexpr int8_t kButtonPins[] = {
    UP_BTN, DW_BTN, L_BTN, R_BTN, SEL_BTN,
#if defined(SET_BTN)
    SET_BTN,
#else
    -1,
#endif
#if defined(ESC_BTN)
    ESC_BTN
#else
    -1
#endif
};
} // namespace

/***************************************************************************************
** Function name: _setup_gpio()
** Location: main.cpp
** Description:   initial setup for the device
***************************************************************************************/
void _setup_gpio() {
    for (int8_t pin : kButtonPins) {
        if (pin >= 0) { pinMode(pin, INPUT_PULLUP); }
    }

    if (TFT_CS >= 0) {
        pinMode(TFT_CS, OUTPUT);
        digitalWrite(TFT_CS, HIGH);
    }
    if (TOUCH_CS >= 0) {
        pinMode(TOUCH_CS, OUTPUT);
        digitalWrite(TOUCH_CS, HIGH);
    }
    if (SDCARD_CS >= 0) {
        pinMode(SDCARD_CS, OUTPUT);
        digitalWrite(SDCARD_CS, HIGH);
    }
    if (CC1101_SS_PIN >= 0) {
        pinMode(CC1101_SS_PIN, OUTPUT);
        digitalWrite(CC1101_SS_PIN, HIGH);
    }
    if (NRF24_SS_PIN >= 0) {
        pinMode(NRF24_SS_PIN, OUTPUT);
        digitalWrite(NRF24_SS_PIN, HIGH);
    }
    if (NRF24_CE_PIN >= 0) {
        pinMode(NRF24_CE_PIN, OUTPUT);
        digitalWrite(NRF24_CE_PIN, LOW);
    }
    if (TFT_BL >= 0) {
        pinMode(TFT_BL, OUTPUT);
        analogWrite(TFT_BL, 255);
    }

    Wire.begin(GROVE_SDA, GROVE_SCL);

    bruceConfig.rfModule = CC1101_SPI_MODULE;
    bruceConfig.rfidModule = PN532_I2C_MODULE;
    bruceConfig.irRx = RXLED;
    bruceConfig.irTx = IR_TX_PIN;

#if HAS_TOUCH
    tft.setTouch(const_cast<uint16_t *>(kTouchCalData));
#endif
}

/***************************************************************************************
** Function name: _post_setup_gpio()
** Location: main.cpp
** Description:   second stage gpio setup to make a few functions work
***************************************************************************************/
void _post_setup_gpio() {}

/***************************************************************************************
** Function name: getBattery()
** location: display.cpp
** Description:   Delivers the battery value from 1-100
***************************************************************************************/
int getBattery() { return 0; }

/***************************************************************************************
** Function name: isCharging()
** Description:   Default implementation that returns false
***************************************************************************************/
bool isCharging() { return false; }

/*********************************************************************
** Function: setBrightness
** location: settings.cpp
** set brightness value
**********************************************************************/
void _setBrightness(uint8_t brightval) {
    if (TFT_BL < 0) return;
    if (brightval == 0) {
        analogWrite(TFT_BL, 0);
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
    checkPowerSaveTime();

#if HAS_TOUCH
    if (millis() - tm > 150 || LongPress) {
        uint16_t x = 0, y = 0;
        digitalWrite(TFT_CS, HIGH);
        digitalWrite(TOUCH_CS, LOW);
        bool touched = tft.getTouch(&x, &y);
        digitalWrite(TOUCH_CS, HIGH);
        if (touched) {
            tm = millis();
            if (!wakeUpScreen()) AnyKeyPress = true;
            else return;

            if (bruceConfig.rotation == 0) {
                uint16_t tmp = x;
                x = tftWidth - y;
                y = (tftHeight + 20) - tmp;
            } else if (bruceConfig.rotation == 1) {
                y = (tftHeight + 20) - y;
            } else if (bruceConfig.rotation == 2) {
                uint16_t tmp = x;
                x = y;
                y = (tftHeight + 20) - tmp;
            } else if (bruceConfig.rotation == 3) {
                uint16_t tmp = x;
                x = tftWidth - tmp;
            }

            touchPoint.x = x;
            touchPoint.y = y;
            touchPoint.pressed = true;
            touchHeatMap(touchPoint);
        }
    }
#endif

    if (millis() - tm < 200 && !LongPress) return;

    bool up = (UP_BTN >= 0) ? digitalRead(UP_BTN) == BTN_ACT : false;
    bool down = (DW_BTN >= 0) ? digitalRead(DW_BTN) == BTN_ACT : false;
    bool left = (L_BTN >= 0) ? digitalRead(L_BTN) == BTN_ACT : false;
    bool right = (R_BTN >= 0) ? digitalRead(R_BTN) == BTN_ACT : false;
    bool mid = (SEL_BTN >= 0) ? digitalRead(SEL_BTN) == BTN_ACT : false;
#if defined(SET_BTN) && SET_BTN >= 0
    bool setBtn = digitalRead(SET_BTN) == BTN_ACT;
#else
    bool setBtn = false;
#endif
#if defined(ESC_BTN) && ESC_BTN >= 0
    bool escBtn = digitalRead(ESC_BTN) == BTN_ACT;
#else
    bool escBtn = false;
#endif

    if (up || down || left || right || mid || setBtn || escBtn) {
        tm = millis();
        if (!wakeUpScreen()) AnyKeyPress = true;
        else return;
    }
    if (left) { PrevPress = true; }
    if (right) { NextPress = true; }
    if (up) {
        UpPress = true;
        PrevPagePress = true;
    }
    if (down) {
        DownPress = true;
        NextPagePress = true;
    }
    if (mid || setBtn) { SelPress = true; }
    if (escBtn || (left && right)) {
        EscPress = true;
        NextPress = false;
        PrevPress = false;
    }
}

/*********************************************************************
** Function: keyboard
** location: mykeyboard.cpp
** Starts keyboard to type data
**********************************************************************/
String keyboard(String mytext, int maxSize, String msg) { return mytext; }

/*********************************************************************
** Function: powerOff
** location: mykeyboard.cpp
** Turns off the device (or try to)
**********************************************************************/
void powerOff() {
#if defined(ESC_BTN) && ESC_BTN >= 0
    esp_sleep_enable_ext0_wakeup((gpio_num_t)ESC_BTN, BTN_ACT);
#else
    esp_sleep_enable_ext0_wakeup((gpio_num_t)SEL_BTN, BTN_ACT);
#endif
    esp_deep_sleep_start();
}

/*********************************************************************
** Function: checkReboot
** location: mykeyboard.cpp
** Btn logic to turn off the device (name is odd btw)
**********************************************************************/
void checkReboot() {
#if defined(ESC_BTN) && ESC_BTN >= 0
    if (digitalRead(ESC_BTN) == BTN_ACT && digitalRead(SEL_BTN) == BTN_ACT) {
#else
    if (digitalRead(L_BTN) == BTN_ACT && digitalRead(R_BTN) == BTN_ACT) {
#endif
        uint32_t time_count = millis();
        while (
#if defined(ESC_BTN) && ESC_BTN >= 0
            digitalRead(ESC_BTN) == BTN_ACT && digitalRead(SEL_BTN) == BTN_ACT
#else
            digitalRead(L_BTN) == BTN_ACT && digitalRead(R_BTN) == BTN_ACT
#endif
        ) {
            if (millis() - time_count > 500) {
                tft.setTextSize(1);
                tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
                int countDown = (millis() - time_count) / 1000 + 1;
                if (countDown < 4) {
                    tft.drawCentreString("PWR OFF IN " + String(countDown) + "/3", tftWidth / 2, 12, 1);
                } else {
                    tft.fillScreen(bruceConfig.bgColor);
                    while (
#if defined(ESC_BTN) && ESC_BTN >= 0
                        digitalRead(ESC_BTN) == BTN_ACT || digitalRead(SEL_BTN) == BTN_ACT
#else
                        digitalRead(L_BTN) == BTN_ACT || digitalRead(R_BTN) == BTN_ACT
#endif
                    )
                        ;
                    delay(200);
                    powerOff();
                }
                delay(10);
            }
        }
        delay(30);
        tft.fillRect(60, 12, tftWidth - 60, tft.fontHeight(1), bruceConfig.bgColor);
    }
}
