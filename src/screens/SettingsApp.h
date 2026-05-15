#pragma once
#include "Screen.h"

class SettingsApp : public Screen {
    M5Canvas spr{&M5.Lcd};

    int   menuIdx = 0;
    bool  editing = false;
    int   bright  = 128;
    int   setH = 12, setM = 0;  // time being edited
    int   editField = 0; // 0=hour 1=min

    static constexpr int ITEMS = 4;
    const char* LABELS[ITEMS] = {"Brightness", "Set Time", "Battery", "About"};

public:
    void onEnter() override { spr.createSprite(SCR_W, CNT_H); editing = false; onDraw(); }
    void onExit()  override { spr.deleteSprite(); }

    void onButtonA() override { // navigate / increment
        if (editing) {
            if (menuIdx == 0) { bright = min(255, bright + 16); M5.Lcd.setBrightness(bright); }
            else if (menuIdx == 1) {
                if (editField == 0) setH = (setH + 1) % 24;
                else                setM = (setM + 1) % 60;
            }
        } else {
            menuIdx = (menuIdx + 1) % ITEMS;
        }
        onDraw();
    }

    void onButtonB() override { // select / decrement / confirm
        if (editing) {
            if (menuIdx == 0) { bright = max(16, bright - 16); M5.Lcd.setBrightness(bright); }
            else if (menuIdx == 1) {
                if (editField == 0) { editField = 1; }
                else {
                    // save time
                    m5::rtc_time_t t; M5.Rtc.getTime(&t);
                    t.hours   = setH; t.minutes = setM; t.seconds = 0;
                    M5.Rtc.setTime(&t);
                    editing = false; editField = 0;
                }
            } else {
                editing = false;
            }
        } else {
            if (menuIdx == 0 || menuIdx == 1) {
                editing = true; editField = 0;
                if (menuIdx == 1) {
                    m5::rtc_time_t t; M5.Rtc.getTime(&t);
                    setH = t.hours; setM = t.minutes;
                }
            }
        }
        onDraw();
    }

    void onDraw() override {
        spr.fillSprite(C_BG);

        // title
        spr.fillRect(0, 0, SCR_W, 14, C_PANEL);
        spr.setTextColor(C_ACCENT, C_PANEL);
        spr.setTextSize(1);
        spr.setCursor(4, 3);
        spr.print("SETTINGS  [A] nav  [B] select");

        // menu items
        int rowH = 22;
        for (int i = 0; i < ITEMS; i++) {
            int y   = 16 + i * rowH;
            bool sel = (i == menuIdx);
            uint16_t bg = sel ? C_PANEL2 : C_BG;
            spr.fillRect(0, y, SCR_W, rowH - 1, bg);
            if (sel) {
                spr.fillRect(0, y, 3, rowH - 1, C_ACCENT);
            }
            spr.setTextColor(sel ? C_WHITE : C_GRAY, bg);
            spr.setTextSize(1);
            spr.setCursor(8, y + 7);
            spr.print(LABELS[i]);

            // value on right
            if (sel) _drawValue(i, y + 7);
        }

        // editing overlay
        if (editing) {
            spr.fillRect(0, 16 + menuIdx * rowH, SCR_W, rowH - 1, C_PANEL2);
            spr.fillRect(0, 16 + menuIdx * rowH, 3, rowH - 1, C_CYAN);
            spr.setTextColor(C_CYAN, C_PANEL2);
            spr.setTextSize(1);
            spr.setCursor(8, 16 + menuIdx * rowH + 7);
            spr.print(LABELS[menuIdx]);

            _drawEditValue(menuIdx, 16 + menuIdx * rowH + 7);

            spr.setTextColor(C_DGRAY, C_PANEL2);
            spr.setCursor(4, 16 + menuIdx * rowH + rowH + 2);
            if (menuIdx == 0)     spr.print("[A] brighter  [B] darker");
            else if (menuIdx == 1 && editField == 0) spr.print("[A] hour++  [B] next field");
            else if (menuIdx == 1 && editField == 1) spr.print("[A] min++   [B] save");
        }

        // battery / device info (always shown at bottom)
        _drawBottomInfo();

        spr.pushSprite(0, CNT_Y);
    }

private:
    void _drawValue(int item, int y) {
        char buf[32];
        switch (item) {
        case 0: snprintf(buf, sizeof(buf), "%d%%", bright * 100 / 255); break;
        case 1: {
            m5::rtc_time_t t; M5.Rtc.getTime(&t);
            snprintf(buf, sizeof(buf), "%02d:%02d", t.hours, t.minutes);
            break;
        }
        case 2: snprintf(buf, sizeof(buf), "%d%%", M5.Power.getBatteryLevel()); break;
        case 3: strncpy(buf, "FluxOS v1.0", sizeof(buf)); break;
        }
        spr.setTextColor(C_CYAN, (item == menuIdx) ? C_PANEL2 : C_BG);
        spr.setCursor(165, y);
        spr.print(buf);
    }

    void _drawEditValue(int item, int y) {
        char buf[16];
        if (item == 0) {
            snprintf(buf, sizeof(buf), "[%d%%]", bright * 100 / 255);
            // brightness bar
            spr.drawRect(100, y - 2, 100, 10, C_DGRAY);
            spr.fillRect(101, y - 1, bright * 98 / 255, 8, C_CYAN);
        } else if (item == 1) {
            if (editField == 0) snprintf(buf, sizeof(buf), "[%02d]:%02d", setH, setM);
            else                snprintf(buf, sizeof(buf), "%02d:[%02d]", setH, setM);
        } else {
            buf[0] = 0;
        }
        spr.setTextColor(C_YELLOW, C_PANEL2);
        spr.setCursor(120, y);
        spr.print(buf);
    }

    void _drawBottomInfo() {
        int y = CNT_H - 20;
        spr.fillRect(0, y, SCR_W, 20, C_PANEL);
        spr.drawFastHLine(0, y, SCR_W, C_ACCENT);

        int batt = M5.Power.getBatteryLevel();
        bool chg = M5.Power.isCharging();
        float temp = 0; M5.Imu.getTemp(&temp);
        m5::rtc_date_t d; M5.Rtc.getDate(&d);

        char buf[64];
        snprintf(buf, sizeof(buf), "BAT %d%%%s  T:%.1fC  %04d-%02d-%02d",
                 batt, chg ? "(CHG)" : "", temp, 2000+d.year, d.month, d.date);
        spr.setTextSize(1);
        spr.setTextColor(C_DGRAY, C_PANEL);
        spr.setCursor(4, y + 6);
        spr.print(buf);
    }
};
