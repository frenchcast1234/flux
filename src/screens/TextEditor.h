#pragma once
#include "Screen.h"
#include <LittleFS.h>
extern int  g_launchApp;
extern char g_openFile[64];

class TextEditor : public Screen {
    static constexpr int MAX_LEN = 480; // ~4 lines × 40 chars × 3 pages
    static constexpr int LINE_H  = 10;
    static constexpr int LINES   = 10; // visible lines in content area

    // Character wheel: space, a-z, A-Z, 0-9, symbols, DEL, SAVE, NEW
    static const char WHEEL[];
    static constexpr int WHEEL_LEN = 70;

    char   _buf[MAX_LEN+1] = {};
    int    _len    = 0;      // current text length
    int    _wIdx   = 0;      // wheel index
    int    _scroll = 0;      // scroll offset in lines
    bool   _saved  = false;
    bool   _viewMode = false; // true when opening existing file

    // hold-repeat
    uint32_t _holdStart = 0;
    bool     _aHeld = false;

public:
    void onEnter() override {
        _len=0; _wIdx=0; _scroll=0; _saved=false; _aHeld=false;
        memset(_buf, 0, sizeof(_buf));
        if (g_openFile[0]) {
            _loadFile();
            _viewMode = true;
        } else {
            _viewMode = false;
        }
        onDraw();
    }

    void onExit() override { g_openFile[0] = 0; }

    void onButtonA() override {
        if (_viewMode) { _scroll = max(0, _scroll-1); onDraw(); return; }
        _wIdx = (_wIdx + 1) % WHEEL_LEN;
        _holdStart = millis(); _aHeld = true;
        _redrawWheel();
    }

    void onButtonB() override {
        if (_viewMode) { _scroll++; onDraw(); return; }
        char c = WHEEL[_wIdx];
        if (c == '\x01') { // DEL
            if (_len > 0) { _buf[--_len] = 0; }
        } else if (c == '\x02') { // SAVE
            _saveFile();
        } else if (c == '\x03') { // NEW — clear
            _len = 0; memset(_buf, 0, sizeof(_buf)); _scroll = 0;
        } else {
            if (_len < MAX_LEN) { _buf[_len++] = c; }
        }
        _wIdx = 0;
        onDraw();
    }

    void onLoop() override {
        if (_aHeld && millis() - _holdStart > 400) {
            if (_viewMode) return;
            _wIdx = (_wIdx + 1) % WHEEL_LEN;
            _holdStart = millis() - 280; // fast repeat
            _redrawWheel();
        }
    }

    void onDraw() override {
        M5.Lcd.fillRect(0, CNT_Y, SCR_W, CNT_H, C_BG);
        _drawHeader();
        _drawText();
        if (!_viewMode) _redrawWheel();
    }

private:
    void _drawHeader() {
        M5.Lcd.setTextSize(1);
        M5.Lcd.setTextColor(C_ACCENT, C_BG);
        M5.Lcd.setCursor(2, CNT_Y+2);
        if (_viewMode) {
            const char* n = strrchr(g_openFile, '/');
            M5.Lcd.print(n ? n+1 : g_openFile);
            M5.Lcd.setTextColor(C_DGRAY, C_BG);
            M5.Lcd.setCursor(150, CNT_Y+2);
            char buf[12]; snprintf(buf,sizeof(buf),"%dB", _len);
            M5.Lcd.print(buf);
        } else {
            M5.Lcd.print(_saved ? "SAVED " : "EDITOR");
            M5.Lcd.setTextColor(C_DGRAY, C_BG);
            M5.Lcd.setCursor(60, CNT_Y+2);
            char buf[12]; snprintf(buf,sizeof(buf),"%d/%d", _len, MAX_LEN);
            M5.Lcd.print(buf);
        }
        M5.Lcd.drawFastHLine(0, CNT_Y+11, SCR_W, C_PANEL);
    }

    void _drawText() {
        int textAreaH = _viewMode ? CNT_H-12 : CNT_H-32;
        int maxLines  = textAreaH / LINE_H;

        // word-wrap into display lines
        char lines[24][42];
        int  numLines = 0;
        int  i = 0;
        while (i <= _len && numLines < 24) {
            int col = 0;
            memset(lines[numLines], 0, 42);
            while (i <= _len && col < 40) {
                if (i == _len || _buf[i] == '\n') { i++; break; }
                lines[numLines][col++] = _buf[i++];
            }
            numLines++;
        }
        if (numLines == 0) numLines = 1;
        if (_scroll > numLines - 1) _scroll = max(0, numLines-1);

        for (int l = 0; l < maxLines && (l+_scroll) < numLines; l++) {
            int y = CNT_Y + 13 + l * LINE_H;
            M5.Lcd.setTextColor(C_WHITE, C_BG);
            M5.Lcd.setTextSize(1);
            M5.Lcd.setCursor(2, y);
            M5.Lcd.print(lines[l+_scroll]);
        }
        // cursor blink position
        if (!_viewMode) {
            int cursorLine = numLines - 1 - _scroll;
            if (cursorLine >= 0 && cursorLine < maxLines) {
                int cx = 2 + strlen(lines[numLines-1])*6;
                int cy = CNT_Y + 13 + cursorLine * LINE_H;
                if ((millis()/500)%2 == 0)
                    M5.Lcd.fillRect(cx, cy, 5, 8, C_ACCENT);
            }
        }
    }

    void _redrawWheel() {
        int y = CNT_Y + CNT_H - 18;
        M5.Lcd.fillRect(0, y, SCR_W, 18, C_PANEL);
        M5.Lcd.drawFastHLine(0, y, SCR_W, C_ACCENT);
        // show 7 chars centered on _wIdx
        int mid = 3;
        for (int i = 0; i < 7; i++) {
            int wi = (_wIdx - mid + i + WHEEL_LEN*10) % WHEEL_LEN;
            char c = WHEEL[wi];
            int x = 4 + i * 33;
            bool isSel = (i == mid);
            if (isSel) M5.Lcd.fillRect(x-1, y+2, 28, 14, C_PANEL2);
            M5.Lcd.setTextSize(isSel ? 1 : 1);
            if      (c == '\x01') { M5.Lcd.setTextColor(isSel?C_ACCENT:C_DGRAY, isSel?C_PANEL2:C_PANEL); M5.Lcd.setCursor(x+1,y+5); M5.Lcd.print("DEL"); }
            else if (c == '\x02') { M5.Lcd.setTextColor(isSel?C_GREEN:C_DGRAY,  isSel?C_PANEL2:C_PANEL); M5.Lcd.setCursor(x+1,y+5); M5.Lcd.print("SAV"); }
            else if (c == '\x03') { M5.Lcd.setTextColor(isSel?C_YELLOW:C_DGRAY, isSel?C_PANEL2:C_PANEL); M5.Lcd.setCursor(x+1,y+5); M5.Lcd.print("CLR"); }
            else if (c == '\n')   { M5.Lcd.setTextColor(isSel?C_CYAN:C_DGRAY,   isSel?C_PANEL2:C_PANEL); M5.Lcd.setCursor(x+1,y+5); M5.Lcd.print("RET"); }
            else                  { M5.Lcd.setTextColor(isSel?C_WHITE:C_GRAY,   isSel?C_PANEL2:C_PANEL); M5.Lcd.setCursor(x+8,y+5); M5.Lcd.printf("%c",c); }
        }
    }

    void _saveFile() {
        // find next available note number
        char path[64];
        int n = 1;
        while (n < 100) {
            snprintf(path, sizeof(path), "/notes/note_%03d.txt", n);
            if (!LittleFS.exists(path)) break;
            n++;
        }
        File f = LittleFS.open(path, "w");
        if (f) { f.write((uint8_t*)_buf, _len); f.close(); _saved = true; }
    }

    void _loadFile() {
        File f = LittleFS.open(g_openFile, "r");
        if (!f) return;
        _len = f.read((uint8_t*)_buf, MAX_LEN);
        _buf[_len] = 0;
        f.close();
    }
};

const char TextEditor::WHEEL[] = {
    ' ', 'a','b','c','d','e','f','g','h','i','j','k','l','m',
    'n','o','p','q','r','s','t','u','v','w','x','y','z',
    'A','B','C','D','E','F','G','H','I','J','K','L','M',
    'N','O','P','Q','R','S','T','U','V','W','X','Y','Z',
    '0','1','2','3','4','5','6','7','8','9',
    '.', ',', '!', '?', ':', ';', '-', '\n',
    '\x01', '\x02', '\x03' // DEL, SAVE, CLR
};
