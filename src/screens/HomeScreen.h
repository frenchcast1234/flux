#pragma once
#include "Screen.h"
extern int g_launchApp;

// 3-page launcher, 8 icons per page (4×2)
class HomeScreen : public Screen {
    static constexpr int PER_PAGE = 8;
    static constexpr int TOTAL    = 23; // total non-home apps

    int sel  = 0;
    int page = 0;

    struct AppInfo { const char* label; uint16_t color; uint8_t icon; };
    static constexpr AppInfo APPS[TOTAL] = {
        // page 1 — essentials
        {"CLOCK",    C_CYAN,    0}, {"GYRO",     C_PURPLE,  1},
        {"AUDIO",    C_GREEN,   2}, {"WIFI",     0x035Fu,   3},
        {"SNAKE",    C_DGREEN,  4}, {"PONG",     C_ORANGE,  5},
        {"IR",       C_ACCENT,  6}, {"CONFIG",   C_GRAY,    7},
        // page 2 — tools & sensors
        {"STOPWATCH",C_CYAN,    8}, {"LEVEL",    C_GREEN,   9},
        {"SYSMON",   C_PURPLE,  10},{"LIGHT",    C_YELLOW,  11},
        {"REACTION", C_ACCENT,  12},{"BLE",      0x035Fu,   13},
        {"DICE",     C_ORANGE,  14},{"ALARM",    C_ACCENT,  0},
        // page 3 — games & more
        {"TETRIS",   C_CYAN,    4}, {"BREAKOUT", C_ORANGE,  5},
        {"FLAPPY",   C_YELLOW,  1}, {"CALC",     C_GREEN,   7},
        {"TALLY",    C_PURPLE,  6}, {"METRO",    C_CYAN,    8},
        {"ABOUT",    C_GRAY,    10},
    };

public:
    void onEnter() override { sel=0; page=0; }

    void onButtonA() override {
        sel = (sel + 1) % TOTAL;
        page = sel / PER_PAGE;
        onDraw();
    }
    void onButtonB() override { g_launchApp = sel + 1; }

    void onDraw() override {
        M5.Lcd.fillRect(0, CNT_Y, SCR_W, CNT_H, C_BG);
        int base = page * PER_PAGE;
        int cnt  = min(PER_PAGE, TOTAL - base);
        for (int i=0; i<cnt; i++) _drawCell(i, base+i, (base+i)==sel);
        // page dots
        for (int p=0; p<3; p++) {
            spr_fillCircle(108+p*12, CNT_Y+CNT_H-5, 3, (p==page)?C_ACCENT:C_PANEL);
        }
    }

private:
    // Draw directly on Lcd (no sprite) to avoid allocating a full-screen canvas
    void spr_fillCircle(int x, int y, int r, uint16_t c) {
        M5.Lcd.fillCircle(x, y, r, c);
    }

    void _drawCell(int gridIdx, int appIdx, bool selected) {
        int col = gridIdx%4, row = gridIdx/4;
        int cx  = col*60, cy = CNT_Y + row*56;
        uint16_t bg = selected ? C_PANEL2 : C_PANEL;
        M5.Lcd.fillRoundRect(cx+2,cy+2,56,52,5,bg);
        if (selected) {
            M5.Lcd.drawRoundRect(cx+2,cy+2,56,52,5,APPS[appIdx].color);
            M5.Lcd.drawRoundRect(cx+3,cy+3,54,50,5,APPS[appIdx].color);
        }
        _drawIcon(APPS[appIdx].icon, cx+30, cy+21, APPS[appIdx].color);
        int llen = strlen(APPS[appIdx].label);
        M5.Lcd.setTextSize(1);
        M5.Lcd.setTextColor(selected?C_WHITE:C_GRAY, bg);
        M5.Lcd.setCursor(cx+30-llen*3, cy+42);
        M5.Lcd.print(APPS[appIdx].label);
    }

    void _drawIcon(uint8_t type, int cx, int cy, uint16_t col) {
        switch (type%15) {
        case 0:  // Clock
            M5.Lcd.drawCircle(cx,cy,11,col);
            M5.Lcd.drawLine(cx,cy,cx,cy-7,col);
            M5.Lcd.drawLine(cx,cy,cx+5,cy+3,col);
            M5.Lcd.fillCircle(cx,cy,2,col); break;
        case 1:  // Gyro / rotation symbol
            M5.Lcd.drawCircle(cx,cy,10,col);
            M5.Lcd.drawLine(cx-10,cy,cx+10,cy,col);
            M5.Lcd.fillCircle(cx,cy,3,col); break;
        case 2:  // Bars
            for(int b=0;b<5;b++){int h=3+b*3;M5.Lcd.fillRect(cx-10+b*5,cy+9-h,3,h,col);}
            break;
        case 3:  // WiFi
            for(int r=0;r<3;r++) M5.Lcd.drawArc(cx,cy+6,5+r*4,4+r*4,225,315,col);
            M5.Lcd.fillCircle(cx,cy+7,2,col); break;
        case 4:  // Snake / tetris S
            M5.Lcd.fillRect(cx-8,cy-2,10,4,col); M5.Lcd.fillRect(cx+2,cy-8,4,10,col);
            M5.Lcd.fillRect(cx-4,cy-8,8,4,col);  M5.Lcd.fillCircle(cx-6,cy+2,2,C_ACCENT);
            break;
        case 5:  // Pong / Breakout paddle
            M5.Lcd.fillRect(cx-12,cy-6,3,12,col); M5.Lcd.fillRect(cx+9,cy-6,3,12,col);
            M5.Lcd.fillCircle(cx,cy,3,col); break;
        case 6:  // IR
            for(int r=0;r<3;r++) M5.Lcd.drawArc(cx,cy,5+r*4,4+r*4,315,45,col);
            M5.Lcd.fillRect(cx-2,cy-2,4,4,col); break;
        case 7:  // Gear / Calc
            M5.Lcd.drawCircle(cx,cy,8,col); M5.Lcd.fillCircle(cx,cy,4,col);
            for(int t=0;t<6;t++){float a=t*60.0f*DEG_TO_RAD;
                M5.Lcd.fillRect(cx+9*cosf(a)-2,cy+9*sinf(a)-2,4,4,col);}
            break;
        case 8:  // Stopwatch / Metronome circle
            M5.Lcd.drawCircle(cx,cy,10,col);
            M5.Lcd.drawLine(cx,cy,cx,cy-8,col);
            M5.Lcd.drawLine(cx-4,cy-12,cx+4,cy-12,col); break;
        case 9:  // Level bubble
            M5.Lcd.drawCircle(cx,cy,10,col);
            M5.Lcd.drawCircle(cx-2,cy-2,4,col);
            M5.Lcd.fillCircle(cx-2,cy-2,2,col); break;
        case 10: // Computer monitor
            M5.Lcd.drawRect(cx-10,cy-8,20,14,col);
            M5.Lcd.drawFastHLine(cx-4,cy+6,8,col);
            M5.Lcd.drawFastHLine(cx-6,cy+9,12,col); break;
        case 11: // Flashlight / sun
            M5.Lcd.fillCircle(cx,cy,5,col);
            for(int t=0;t<8;t++){float a=t*45*DEG_TO_RAD;
                M5.Lcd.drawLine(cx+(int)(6*cosf(a)),cy+(int)(6*sinf(a)),
                                cx+(int)(10*cosf(a)),cy+(int)(10*sinf(a)),col);}
            break;
        case 12: // Lightning bolt / reaction
            M5.Lcd.drawLine(cx+2,cy-10,cx-4,cy+1,col);
            M5.Lcd.drawLine(cx-4,cy+1,cx+3,cy+1,col);
            M5.Lcd.drawLine(cx+3,cy+1,cx-3,cy+10,col); break;
        case 13: // Bluetooth-ish
            M5.Lcd.drawLine(cx,cy-10,cx,cy+10,col);
            M5.Lcd.drawLine(cx,cy-10,cx+7,cy-4,col);
            M5.Lcd.drawLine(cx+7,cy-4,cx,cy+2,col);
            M5.Lcd.drawLine(cx,cy+2,cx+7,cy+8,col);
            M5.Lcd.drawLine(cx+7,cy+8,cx,cy+10,col); break;
        case 14: // Dice face
            M5.Lcd.drawRoundRect(cx-8,cy-8,16,16,3,col);
            M5.Lcd.fillCircle(cx-3,cy-3,2,col); M5.Lcd.fillCircle(cx+3,cy-3,2,col);
            M5.Lcd.fillCircle(cx,cy,2,col);
            M5.Lcd.fillCircle(cx-3,cy+3,2,col); M5.Lcd.fillCircle(cx+3,cy+3,2,col);
            break;
        }
    }
};
constexpr HomeScreen::AppInfo HomeScreen::APPS[23];
