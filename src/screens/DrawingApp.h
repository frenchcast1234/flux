#pragma once
#include "Screen.h"
#include <LittleFS.h>
extern int  g_launchApp;
extern char g_openFile[64];

class DrawingApp : public Screen {
    // Canvas is 240×117 — stored as RGB565, 2 bytes/pixel = 56160 bytes
    static constexpr int CW = SCR_W;
    static constexpr int CH = CNT_H;

    static constexpr uint16_t PALETTE[] = {
        C_WHITE, C_ACCENT, C_CYAN, C_GREEN, C_YELLOW, C_ORANGE, C_PURPLE, C_BLACK
    };
    static constexpr int NCOLORS = 8;

    int      _cx = CW/2, _cy = CH/2; // cursor
    int      _colorIdx = 0;
    bool     _drawing  = false;
    bool     _menuOpen = false;
    int      _menuSel  = 0; // 0=save 1=load 2=clear

    uint32_t _bBtnAt   = 0;
    bool     _bWaiting = false;

    M5Canvas* _spr = nullptr;

    static constexpr float TILT_SCALE = 60.0f;

public:
    void onEnter() override {
        _spr = new M5Canvas(&M5.Lcd);
        _spr->createSprite(CW, CH);
        _spr->fillSprite(C_BLACK);
        _cx = CW/2; _cy = CH/2;
        _colorIdx = 0; _drawing = false; _menuOpen = false;
        if (g_openFile[0]) { _loadFile(); }
        onDraw();
    }

    void onExit() override {
        delete _spr; _spr = nullptr;
        g_openFile[0] = 0;
    }

    void onButtonA() override {
        if (_menuOpen) { _menuSel = (_menuSel+1)%3; _drawMenu(); return; }
        _drawing = !_drawing;
        _drawToolbar();
    }

    void onButtonB() override {
        if (_menuOpen) {
            // confirm menu action
            if      (_menuSel==0) { _saveFile(); _menuOpen=false; onDraw(); }
            else if (_menuSel==1) { _menuOpen=false; /* load: just close, opened from FM */ onDraw(); }
            else                  { _spr->fillSprite(C_BLACK); _menuOpen=false; onDraw(); }
            return;
        }
        // double-tap detection for menu
        uint32_t now = millis();
        if (_bWaiting && (now - _bBtnAt) < 400) {
            _menuOpen = true; _menuSel = 0;
            _bWaiting = false;
            _drawMenu();
            return;
        }
        _bBtnAt = now; _bWaiting = true;
        // single tap = next color
        _colorIdx = (_colorIdx + 1) % NCOLORS;
        _drawToolbar();
    }

    void onLoop() override {
        if (_menuOpen) return;
        if (_bWaiting && (millis()-_bBtnAt) > 400) _bWaiting = false;

        float ax, ay, az;
        M5.Imu.getAccelData(&ax, &ay, &az);
        // ay tilts left/right, ax tilts up/down
        float dx = ay * TILT_SCALE * 0.016f; // ~16ms loop
        float dy = -ax * TILT_SCALE * 0.016f;
        _cx = constrain(_cx + (int)dx, 0, CW-1);
        _cy = constrain(_cy + (int)dy, 0, CH-1);

        if (_drawing) {
            _spr->fillCircle(_cx, _cy, 2, PALETTE[_colorIdx]);
        }
        // push canvas
        _spr->pushSprite(0, CNT_Y);
        // draw cursor
        M5.Lcd.drawCircle(_cx, CNT_Y+_cy, 4, C_WHITE);
        M5.Lcd.drawCircle(_cx, CNT_Y+_cy, 3, PALETTE[_colorIdx]);
        _drawToolbar();
    }

    void onDraw() override {
        if (_spr) _spr->pushSprite(0, CNT_Y);
        _drawToolbar();
    }

private:
    void _drawToolbar() {
        int y = CNT_Y + CH - 14;
        M5.Lcd.fillRect(0, y, SCR_W, 14, C_PANEL);
        M5.Lcd.drawFastHLine(0, y, SCR_W, C_DGRAY);
        // draw mode indicator
        M5.Lcd.setTextSize(1);
        M5.Lcd.setTextColor(_drawing ? C_ACCENT : C_DGRAY, C_PANEL);
        M5.Lcd.setCursor(3, y+4);
        M5.Lcd.print(_drawing ? "DRAW" : "MOVE");
        // color swatches
        for (int i = 0; i < NCOLORS; i++) {
            int x = 40 + i*22;
            M5.Lcd.fillRect(x, y+2, 18, 10, PALETTE[i]);
            if (i == _colorIdx) M5.Lcd.drawRect(x-1, y+1, 20, 12, C_WHITE);
        }
        // hint
        M5.Lcd.setTextColor(C_DGRAY, C_PANEL);
        M5.Lcd.setCursor(SCR_W-50, y+4);
        M5.Lcd.print("B:clr/svd");
    }

    void _drawMenu() {
        int x=40, y=CNT_Y+35, w=160, h=60;
        M5.Lcd.fillRoundRect(x, y, w, h, 5, C_PANEL);
        M5.Lcd.drawRoundRect(x, y, w, h, 5, C_ACCENT);
        M5.Lcd.setTextSize(1);
        const char* items[] = {"Save drawing","Load file","Clear canvas"};
        for (int i=0; i<3; i++) {
            bool sel = (i==_menuSel);
            M5.Lcd.setTextColor(sel?C_WHITE:C_GRAY, C_PANEL);
            if (sel) M5.Lcd.fillRect(x+2, y+4+i*18, w-4, 16, C_PANEL2);
            M5.Lcd.setCursor(x+8, y+8+i*18);
            M5.Lcd.print(items[i]);
        }
    }

    void _saveFile() {
        char path[64];
        int n = 1;
        while (n < 100) {
            snprintf(path, sizeof(path), "/drawings/draw_%03d.drw", n);
            if (!LittleFS.exists(path)) break;
            n++;
        }
        File f = LittleFS.open(path, "w");
        if (!f) return;
        // 8-byte header: "FLUX" + W(u16) + H(u16)
        f.write((uint8_t*)"FLUX", 4);
        uint16_t ww=CW, hh=CH;
        f.write((uint8_t*)&ww, 2); f.write((uint8_t*)&hh, 2);
        // pixel data row by row
        uint16_t row[CW];
        for (int y=0; y<CH; y++) {
            for (int x=0; x<CW; x++) row[x] = _spr->readPixel(x,y);
            f.write((uint8_t*)row, CW*2);
        }
        f.close();
    }

    void _loadFile() {
        File f = LittleFS.open(g_openFile, "r");
        if (!f) return;
        char hdr[4]; f.read((uint8_t*)hdr, 4);
        if (memcmp(hdr,"FLUX",4)!=0) { f.close(); return; }
        uint16_t ww,hh; f.read((uint8_t*)&ww,2); f.read((uint8_t*)&hh,2);
        uint16_t row[CW];
        for (int y=0; y<(int)hh && y<CH; y++) {
            f.read((uint8_t*)row, ww*2);
            for (int x=0; x<(int)ww && x<CW; x++) _spr->drawPixel(x,y,row[x]);
        }
        f.close();
    }
};
constexpr uint16_t DrawingApp::PALETTE[8];
