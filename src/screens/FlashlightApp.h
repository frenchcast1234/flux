#pragma once
#include "Screen.h"

class FlashlightApp : public Screen {
    static constexpr uint16_t COLORS[] = {
        0xFFFFu, // white
        0xF800u, // red
        0x07E0u, // green
        0x001Fu, // blue
        0xFFE0u, // yellow
        0xF81Fu, // magenta
        0x07FFu, // cyan
    };
    static constexpr int NCOLORS = 7;
    int      colorIdx = 0;
    int      bright   = 255;
    int      prevBright = 160;

public:
    void onEnter() override {
        prevBright = 160;
        bright     = 255;
        M5.Lcd.setBrightness(bright);
        onDraw();
    }
    void onExit() override {
        M5.Lcd.setBrightness(160);
        M5.Lcd.fillRect(0, CNT_Y, SCR_W, CNT_H, C_BG);
    }

    void onButtonA() override { // cycle color
        colorIdx = (colorIdx + 1) % NCOLORS;
        onDraw();
    }
    void onButtonB() override { // toggle brightness
        bright = (bright == 255) ? 120 : 255;
        M5.Lcd.setBrightness(bright);
        onDraw();
    }

    void onDraw() override {
        uint16_t col = COLORS[colorIdx];
        M5.Lcd.fillRect(0, CNT_Y, SCR_W, CNT_H, col);

        // minimal label in complementary color
        uint16_t tc = (col == 0xFFFF || col == 0xFFE0) ? C_BLACK : C_WHITE;
        const char* NAMES[] = {"WHITE","RED","GREEN","BLUE","YELLOW","MAGENTA","CYAN"};
        M5.Lcd.setTextColor(tc, col);
        M5.Lcd.setTextSize(1);
        M5.Lcd.setCursor(4, CNT_Y + 4);
        M5.Lcd.printf("[A] color: %s  [B] bright: %d%%",
                      NAMES[colorIdx], bright * 100 / 255);
    }
};
constexpr uint16_t FlashlightApp::COLORS[FlashlightApp::NCOLORS];
