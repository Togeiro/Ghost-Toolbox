#ifndef Pins_Arduino_h
#define Pins_Arduino_h

#include "soc/soc_caps.h"
#include <stdint.h>

// -----------------------------------------------------------------------------
// Core SPI bus (shared by TFT, touch controller, SD card and external radios)
// -----------------------------------------------------------------------------
#define SPI_SS_PIN 7
#define SPI_MOSI_PIN 35
#define SPI_MISO_PIN 37
#define SPI_SCK_PIN 36
static const uint8_t SS = SPI_SS_PIN;
static const uint8_t MOSI = SPI_MOSI_PIN;
static const uint8_t MISO = SPI_MISO_PIN;
static const uint8_t SCK = SPI_SCK_PIN;

// -----------------------------------------------------------------------------
// I2C bus (PN532 default wiring)
// -----------------------------------------------------------------------------
#define GROVE_SDA 8
#define GROVE_SCL 9
static const uint8_t SDA = GROVE_SDA;
static const uint8_t SCL = GROVE_SCL;

// -----------------------------------------------------------------------------
// TFT_eSPI display configuration (ILI9341 240x320 portrait)
// -----------------------------------------------------------------------------
#define USER_SETUP_LOADED 1
#define ILI9341_DRIVER 1
#define TFT_WIDTH 240
#define TFT_HEIGHT 320
#define ROTATION 0
#define TFT_RGB_ORDER TFT_RGB
#define TFT_BACKLIGHT_ON 1
#define SMOOTH_FONT 1
#define TFT_CS SPI_SS_PIN
#define TFT_DC 15
#define TFT_RST 16
#define TFT_BL 17
#define TFT_MOSI SPI_MOSI_PIN
#define TFT_MISO SPI_MISO_PIN
#define TFT_SCLK SPI_SCK_PIN
#define SPI_FREQUENCY 27000000
#define SPI_READ_FREQUENCY 20000000
#define SPI_TOUCH_FREQUENCY 2500000

// -----------------------------------------------------------------------------
// Touch controller (XPT2046)
// -----------------------------------------------------------------------------
#define HAS_TOUCH 1
#define TOUCH_CS 5
#define TOUCH_IRQ 4

// -----------------------------------------------------------------------------
// SD card slot soldered to the TFT module
// -----------------------------------------------------------------------------
#define SDCARD_CS 34
#define SDCARD_MOSI SPI_MOSI_PIN
#define SDCARD_MISO SPI_MISO_PIN
#define SDCARD_SCK SPI_SCK_PIN

// -----------------------------------------------------------------------------
// Navigation: joystick + external buttons
// -----------------------------------------------------------------------------
#define HAS_5_BUTTONS
#define BTN_ALIAS "\"OK\""
#define BTN_ACT LOW
#define SEL_BTN 42
#define UP_BTN 41
#define DW_BTN 39
#define R_BTN 38
#define L_BTN 40
#define SET_BTN 1
#define ESC_BTN 2

// -----------------------------------------------------------------------------
// RF front-ends (CC1101 + NRF24L01+)
// -----------------------------------------------------------------------------
#define USE_CC1101_VIA_SPI
#define CC1101_SS_PIN 10
#define CC1101_MOSI_PIN SPI_MOSI_PIN
#define CC1101_MISO_PIN SPI_MISO_PIN
#define CC1101_SCK_PIN SPI_SCK_PIN
#define CC1101_GDO0_PIN 33
#define CC1101_GDO2_PIN -1

#define USE_NRF24_VIA_SPI
#define NRF24_SS_PIN 6
#define NRF24_CE_PIN 21
#define NRF24_MOSI_PIN SPI_MOSI_PIN
#define NRF24_MISO_PIN SPI_MISO_PIN
#define NRF24_SCK_PIN SPI_SCK_PIN

// -----------------------------------------------------------------------------
// Infrared defaults (available on the front panel header)
// -----------------------------------------------------------------------------
#define RXLED 18
#define IR_TX_PIN 47
#define LED -1
#define LED_ON HIGH
#define LED_OFF LOW

// -----------------------------------------------------------------------------
// RGB LED (on-board WS2812B)
// -----------------------------------------------------------------------------
#define HAS_RGB_LED 1
#define RGB_LED 48
#define LED_TYPE WS2812B
#define LED_ORDER GRB
#define LED_TYPE_IS_RGBW 0
#define LED_COUNT 1
#define LED_COLOR_STEP 15

// -----------------------------------------------------------------------------
// Misc peripherals
// -----------------------------------------------------------------------------
#define USB_as_HID 1
#define MINBRIGHT (uint8_t)1

// PN532 helpers
#define PN532_SDA GROVE_SDA
#define PN532_SCL GROVE_SCL
#define PN532_IRQ -1
#define PN532_RF_REST -1

#endif /* Pins_Arduino_h */
