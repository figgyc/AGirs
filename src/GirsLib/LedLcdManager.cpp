#include "LedLcdManager.h"
#include "Tokenizer.h"
#include <string.h>

LiquidCrystal_I2C *LedLcdManager::lcd;
milliseconds_t LedLcdManager::blinkTime = defaultBlinkTime;
unsigned long LedLcdManager::turnOffTime;
unsigned int LedLcdManager::lcdRows = 0;
unsigned int LedLcdManager::lcdColumns = 0;

pin_t LedLcdManager::physicalLeds[maxLeds];
led_t LedLcdManager::logicalLeds[maxLeds];
boolean LedLcdManager::shouldTimeOut[maxLeds];

void LedLcdManager::setup(int8_t i2cAddress, uint8_t columns, uint8_t rows,
        const pin_t physicalLeds_[], const pin_t logicalLeds_[], const boolean shouldTimeOut_[]) {
    setupLcdI2c(i2cAddress, columns, rows);
    setupPhysicalLeds(physicalLeds_);
    setupLogicLeds(logicalLeds_);
    setupShouldTimeOut(shouldTimeOut_);
    disableTurnOffTime();
}

boolean LedLcdManager::setPhysicalLed(led_t physicalLed, LedState state) {
    pin_t pin = physicalLeds[physicalLed-1];
    if (pin == invalidPin)
        return false;

#ifdef ARDUINO
    digitalWrite(pin, state == off ? LOW : HIGH);
#else
    std::cout << "setPhysicalLed: pin = " << (int) pin << ", " << (state == off ? "off" : "on") << std::endl;
#endif
    if (state == blink)
        updateTurnOffTime();
    return true;
}

LedLcdManager::LedState LedLcdManager::onOffBlinkParse(const char *value) {
        return value == NULL ? invalid
                : strcmp(value, "on") == 0 ? on
                : strcmp(value, "off") == 0 ? off
                : strcmp(value, "blink") == 0 ? blink
                : invalid;
}

boolean LedLcdManager::setLogicLed(led_t logicLed, LedState state) {
    if (logicLed == invalidLed
            || logicLed <  1
            || logicLed > maxLeds
            || state == invalid )
        return false;

    led_t physicalLed = logicalLeds[logicLed - 1];
    if (physicalLed == invalidLed)
        return false;

    return setPhysicalLed(physicalLed, state);
}

boolean LedLcdManager::setupLogicLed(led_t logicLed, led_t physicalLed) {
    if (physicalLed == invalidLed)
        return false;

    logicalLeds[logicLed-1] = physicalLed;
    return true;
}

boolean LedLcdManager::setupLogicLeds(const led_t logicalLeds_[maxLeds]) {
    for (int i = 0; i < maxLeds; i++)
        logicalLeds[i] = logicalLeds_ == NULL ? i+1 : logicalLeds_[i];
    return true;
}

void LedLcdManager::setupPhysicalLeds(const led_t physicalLeds_[maxLeds]) {
    for (int i = 0; i < maxLeds; i++) {
        physicalLeds[i] = physicalLeds_ == NULL ? invalidPin : physicalLeds_[i];
        if (physicalLeds[i] != invalidPin)
            pinMode(physicalLeds[i], OUTPUT);
    }
}

void LedLcdManager::setupShouldTimeOut(const boolean shouldTimeOut_[maxLeds]) {
    for (int i = 0; i < maxLeds; i++)
        shouldTimeOut[i] = shouldTimeOut_ == NULL ? true : shouldTimeOut_[i];
}

void LedLcdManager::setupShouldTimeout(led_t logicLed, boolean state) {
    if (logicLed != invalidLed)
        shouldTimeOut[logicLed-1] = state;
}

void LedLcdManager::setupLcdI2c(int8_t i2cAddress, uint8_t columns, uint8_t rows) {
    lcd = i2cAddress >= 0 ? new LiquidCrystal_I2C((uint8_t)i2cAddress, columns, rows) : NULL;
    if (lcd) {
        lcdRows = rows;
        lcdColumns = columns;
        lcd->init();
    }
}

void LedLcdManager::updateTurnOffTime() {
    turnOffTime = millis() + blinkTime;
}

void LedLcdManager::selfTest(const char *text) {
    lcdPrint(text);
    for (led_t i = 1; i <= maxLeds; i++)
        setLogicLed(i, on);
    delay(defaultBlinkTime);
    allOff(true);
}

void LedLcdManager::selfTest(const __FlashStringHelper *text) {
    lcdPrint(text);
    for (led_t i = 1; i <= maxLeds; i++)
        setLogicLed(i, on);
    delay(defaultBlinkTime);
    allOff(true);
}

void LedLcdManager::checkTurnoff() {
    if (millis() > turnOffTime)
        allOff(false);
}

void LedLcdManager::allOff(boolean force) {
    if (lcd) {
        lcd->noDisplay();
        lcd->noBacklight();
    }
    for (led_t i = 1; i <= maxLeds; i++)
        if (force || shouldTimeOut[i - 1])
            setLogicLed(i, off);

    disableTurnOffTime();
}

void LedLcdManager::disableTurnOffTime() {
    turnOffTime = (unsigned long) -1;
}

void LedLcdManager::lcdPrint(String& string, boolean clear, int x, int y) {
    if (!lcd)
        return;

    int row = (y < 0 && clear) ? 0 : y;
    int column = x < 0 ? 0 : x;
    if (row > (int) lcdRows - 1 || column > (int) lcdColumns - 1) // outside of display
        return;

    if (clear)
        lcd->clear();

    boolean didPrint = false;
    Tokenizer tokenizer(string);
    while (true) {
        String line = tokenizer.getLine();
        if (line.length() == 0)
            break;
        if (line.length() > lcdColumns)
            line = line.substring(0, lcdColumns);
        if (clear || y >= 0) {
            lcd->setCursor(column, row);
            row++;
            column = 0;
        }
        lcd->print(line.c_str());
        didPrint = true;
    }
    if (didPrint) {
        lcd->display();
        lcd->backlight();
    }
    updateTurnOffTime();
}
