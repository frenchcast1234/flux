#pragma once
#include "screens/Screen.h"
#include <LittleFS.h>

extern int  g_launchApp;
extern char g_openFile[64];

class FluxOS {
public:
    static constexpr int MAX_APPS = 32;
    Screen* apps[MAX_APPS];
    int     numApps = 0;
    int     curApp  = -1;

    void addApp(Screen* s) { if (numApps < MAX_APPS) apps[numApps++] = s; }

    // ── boot ─────────────────────────────────────────────────────────────
    void begin() {
        M5.begin();
        M5.Lcd.setRotation(3);
        M5.Lcd.setBrightness(160);
        LittleFS.begin(true);
        LittleFS.mkdir("/notes");
        LittleFS.mkdir("/drawings");
        LittleFS.mkdir("/audio");
        _drawBoot();
        setApp(0);
    }

    // ── main loop ─────────────────────────────────────────────────────────
    void update() {
        M5.update();

        // long press PWR → power menu
        if (M5.BtnPWR.isPressed() && !_pwrLong &&
            (millis() - _pwrPressAt) > 1500 && _pwrPressAt != 0) {
            _pwrLong = true;
            _showPowerMenu();
            return;
        }
        if (M5.BtnPWR.wasPressed()) { _pwrPressAt = millis(); _pwrLong = false; }
        if (M5.BtnPWR.wasReleased()) {
            if (!_pwrLong && curApp != 0) { setApp(0); }
            _pwrLong = false; _pwrPressAt = 0;
        }

        if (curApp >= 0 && curApp < numApps) {
            Screen* s = apps[curApp];
            if (M5.BtnA.wasPressed()) s->onButtonA();
            if (M5.BtnB.wasPressed()) s->onButtonB();
            s->onLoop();
        }

        // deferred navigation from apps
        if (g_launchApp == -2)      { g_launchApp = -1; setApp(0); }
        else if (g_launchApp >= 0)  { int i = g_launchApp; g_launchApp = -1; setApp(i); }

        // status bar tick
        static uint32_t lastSB = 0;
        if (millis() - lastSB >= 1000) { lastSB = millis(); drawStatusBar(); }

        // toast expiry
        if (_toastEnd && millis() > _toastEnd) {
            _toastEnd = 0; _toastBuf[0] = 0;
            if (curApp >= 0) apps[curApp]->onDraw();
            drawStatusBar();
        }
        if (_toastEnd) _drawToast();
    }

    void toast(const char* msg, uint32_t ms = 2200) {
        strncpy(_toastBuf, msg, sizeof(_toastBuf) - 1);
        _toastEnd = millis() + ms;
        _drawToast();
    }

    void drawStatusBar() {
        static const char* NAMES[] = {
            "FLUX","CLOCK","GYRO","AUDIO","WIFI","SNAKE","PONG",
            "IR","CONFIG","STOPWATCH","LEVEL","SYSMON","LIGHT",
            "REACTION","BLE","DICE","ALARM",
            "TETRIS","BREAKOUT","FLAPPY","CALC","TALLY","METRO","ABOUT",
            "FILES","EDITOR","DRAW","AUDIO R","POMODORO","WEATHER","NTP"
        };
        M5.Lcd.fillRect(0, 0, SCR_W, SB_H, C_PANEL);
        M5.Lcd.drawFastHLine(0, SB_H-1, SCR_W, C_ACCENT);

        // app name + page dot
        M5.Lcd.setTextColor(C_CYAN, C_PANEL);
        M5.Lcd.setTextSize(1);
        M5.Lcd.setCursor(4, 5);
        if (curApp >= 0 && curApp < 31) M5.Lcd.print(NAMES[curApp]);

        // time
        m5::rtc_time_t t; M5.Rtc.getTime(&t);
        char buf[16]; snprintf(buf, sizeof(buf), "%02d:%02d", t.hours, t.minutes);
        M5.Lcd.setTextColor(C_WHITE, C_PANEL);
        M5.Lcd.setCursor(97, 5); M5.Lcd.print(buf);

        // battery
        _drawBatt(195, 3);
    }

private:
    uint32_t _pwrPressAt = 0;
    bool     _pwrLong    = false;
    char     _toastBuf[48] = {};
    uint32_t _toastEnd  = 0;

    // ── app switch ────────────────────────────────────────────────────────
    void setApp(int idx) {
        if (curApp >= 0 && curApp < numApps) apps[curApp]->onExit();
        curApp = idx;
        M5.Lcd.fillRect(0, CNT_Y, SCR_W, CNT_H, C_BG);
        if (curApp >= 0 && curApp < numApps) {
            apps[curApp]->onEnter();
            apps[curApp]->onDraw();
        }
        drawStatusBar();
        _toastEnd = 0;
    }

    // ── boot splash ───────────────────────────────────────────────────────
    void _drawBoot() {
        M5.Lcd.fillScreen(C_BLACK);

        // faint circuit grid
        for (int x = 0; x < SCR_W; x += 20) M5.Lcd.drawFastVLine(x, 0, SCR_H, C_PANEL);
        for (int y = 0; y < SCR_H; y += 20) M5.Lcd.drawFastHLine(0, y, SCR_W, C_PANEL);
        for (int x = 0; x < SCR_W; x += 20)
            for (int y = 0; y < SCR_H; y += 20)
                M5.Lcd.fillRect(x-1, y-1, 3, 3, C_DGRAY);

        // glow layers for "FLUX" (draw at small offsets in darker colours)
        const uint16_t glows[] = { 0x3003u, 0x600Bu, 0xA015u };
        for (uint16_t gc : glows) {
            for (int8_t dx = -2; dx <= 2; dx++) for (int8_t dy = -2; dy <= 2; dy++) {
                M5.Lcd.setTextColor(gc, C_BLACK);
                M5.Lcd.setTextSize(4);
                M5.Lcd.setCursor(65 + dx, 22 + dy);
                M5.Lcd.print("FLUX");
            }
        }

        // main FLUX text — animate letter by letter
        const char* flux = "FLUX";
        M5.Lcd.setTextSize(4);
        for (int i = 0; i < 4; i++) {
            M5.Lcd.setTextColor(C_WHITE, C_BLACK);
            M5.Lcd.setCursor(65 + i * 25, 22);
            M5.Lcd.print(flux[i]);
            M5.Speaker.tone(300 + i * 150, 60);
            delay(110);
        }

        // "OS" badge
        M5.Lcd.fillRoundRect(172, 24, 38, 20, 4, C_ACCENT);
        M5.Lcd.setTextColor(C_WHITE, C_ACCENT);
        M5.Lcd.setTextSize(2);
        M5.Lcd.setCursor(176, 28);
        M5.Lcd.print("OS");

        // horizontal separator
        M5.Lcd.drawFastHLine(40, 68, 200, C_ACCENT);
        M5.Lcd.drawFastHLine(40, 69, 200, C_ACCENT);

        // subtitle lines
        M5.Lcd.setTextColor(C_CYAN, C_BLACK);
        M5.Lcd.setTextSize(1);
        M5.Lcd.setCursor(65, 74);  M5.Lcd.print("v2.0  M5StickC Plus2");
        M5.Lcd.setTextColor(C_DGRAY, C_BLACK);
        M5.Lcd.setCursor(65, 86);  M5.Lcd.print("fluxos.local");

        // corner decorations
        M5.Lcd.drawLine(2, 2, 14, 2, C_ACCENT);   M5.Lcd.drawLine(2, 2, 2, 14, C_ACCENT);
        M5.Lcd.drawLine(SCR_W-3, 2, SCR_W-15, 2, C_ACCENT);
        M5.Lcd.drawLine(SCR_W-3, 2, SCR_W-3, 14, C_ACCENT);
        M5.Lcd.drawLine(2, SCR_H-3, 14, SCR_H-3, C_ACCENT);
        M5.Lcd.drawLine(2, SCR_H-3, 2, SCR_H-15, C_ACCENT);

        // progress bar
        M5.Lcd.drawRect(20, 104, 200, 12, C_DGRAY);
        const int STEPS = 50;
        for (int i = 0; i <= STEPS; i++) {
            int px = i * 198 / STEPS;
            uint16_t col = (i < STEPS*6/10) ? C_GREEN :
                           (i < STEPS*9/10) ? C_YELLOW : C_ACCENT;
            M5.Lcd.fillRect(21, 105, px, 10, col);
            M5.Lcd.setTextColor(C_WHITE, C_BLACK);
            M5.Lcd.setTextSize(1);
            M5.Lcd.setCursor(96, 120);
            char pb[20]; snprintf(pb, sizeof(pb), "Loading %d%%", i*2);
            M5.Lcd.print(pb);
            delay(25);
        }
        M5.Speaker.tone(880, 120);
        delay(300);
    }

    // ── power menu (blocking) ─────────────────────────────────────────────
    void _showPowerMenu() {
        // dim overlay
        M5.Lcd.fillRect(30, 25, 180, 90, C_PANEL);
        M5.Lcd.drawRect(30, 25, 180, 90, C_ACCENT);
        M5.Lcd.drawRect(31, 26, 178, 88, C_ACCENT);

        M5.Lcd.setTextSize(1);
        M5.Lcd.setTextColor(C_ACCENT, C_PANEL);
        M5.Lcd.setCursor(80, 32); M5.Lcd.print("POWER MENU");
        M5.Lcd.drawFastHLine(35, 43, 170, C_DGRAY);

        M5.Lcd.setTextColor(C_WHITE, C_PANEL);
        M5.Lcd.setCursor(50, 50); M5.Lcd.print("[A]  Deep Sleep");
        M5.Lcd.setCursor(50, 64); M5.Lcd.print("[B]  Restart");
        M5.Lcd.setCursor(50, 78); M5.Lcd.print("[PWR] Cancel");
        M5.Lcd.setTextColor(C_DGRAY, C_PANEL);
        M5.Lcd.setCursor(50, 96); M5.Lcd.printf("Batt: %d%%", M5.Power.getBatteryLevel());

        for (;;) {
            M5.update();
            if (M5.BtnA.wasPressed())       { M5.Power.deepSleep(); return; }
            if (M5.BtnB.wasPressed())       { ESP.restart(); return; }
            if (M5.BtnPWR.wasReleased())    {
                _pwrLong = false; _pwrPressAt = 0;
                if (curApp >= 0) apps[curApp]->onDraw();
                drawStatusBar();
                return;
            }
            delay(10);
        }
    }

    // ── toast notification ────────────────────────────────────────────────
    void _drawToast() {
        int y = SCR_H - 16;
        M5.Lcd.fillRoundRect(8, y, SCR_W - 16, 13, 3, C_PANEL);
        M5.Lcd.drawRoundRect(8, y, SCR_W - 16, 13, 3, C_CYAN);
        M5.Lcd.setTextColor(C_WHITE, C_PANEL);
        M5.Lcd.setTextSize(1);
        M5.Lcd.setCursor(14, y + 3);
        M5.Lcd.print(_toastBuf);
    }

    // ── battery indicator ────────────────────────────────────────────────
    void _drawBatt(int x, int y) {
        int pct  = M5.Power.getBatteryLevel();
        bool chg = M5.Power.isCharging();
        uint16_t col = chg ? C_CYAN : (pct > 50 ? C_GREEN : (pct > 20 ? C_YELLOW : C_ACCENT));
        M5.Lcd.drawRect(x, y, 30, 12, C_GRAY);
        M5.Lcd.fillRect(x+30, y+4, 3, 4, C_GRAY);
        int fill = map(constrain(pct,0,100), 0, 100, 0, 28);
        M5.Lcd.fillRect(x+1, y+1, fill, 10, col);
        if (chg) { // lightning bolt
            M5.Lcd.setTextColor(C_WHITE, col > 0 ? col : C_PANEL);
            M5.Lcd.setTextSize(1);
            M5.Lcd.setCursor(x+10, y+2);
            M5.Lcd.print("+");
        }
        M5.Lcd.setTextColor(C_WHITE, C_PANEL);
        M5.Lcd.setTextSize(1);
        M5.Lcd.setCursor(x+33, y+2);
        char buf[6]; snprintf(buf, sizeof(buf), "%d%%", pct);
        M5.Lcd.print(buf);
    }
};
