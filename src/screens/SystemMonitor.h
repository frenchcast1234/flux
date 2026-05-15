#pragma once
#include "Screen.h"
#include <WiFi.h>

class SystemMonitor : public Screen {
    M5Canvas spr{&M5.Lcd};
    uint32_t lastDraw = 0;

public:
    void onEnter() override { spr.createSprite(SCR_W, CNT_H); }
    void onExit()  override { spr.deleteSprite(); }
    void onLoop()  override {
        if (millis() - lastDraw >= 1000) { lastDraw = millis(); onDraw(); }
    }

    void onDraw() override {
        spr.fillSprite(C_BG);

        // ── title bar ─────────────────────────────────────────────────────
        spr.fillRect(0, 0, SCR_W, 11, C_PANEL);
        spr.setTextColor(C_ACCENT, C_PANEL); spr.setTextSize(1);
        spr.setCursor(4, 2); spr.print("SYSTEM MONITOR");

        uint32_t up = millis() / 1000;
        char ubuf[20]; snprintf(ubuf, sizeof(ubuf), "UP %luh%02lum%02lus",
                                up/3600, (up/60)%60, up%60);
        spr.setTextColor(C_DGRAY, C_PANEL);
        spr.setCursor(140, 2); spr.print(ubuf);

        int y = 14;
        // ── CPU ──────────────────────────────────────────────────────────
        _row(y,  "CPU",  String(getCpuFrequencyMhz()) + " MHz", C_CYAN);
        // ── RAM ──────────────────────────────────────────────────────────
        uint32_t freeH  = ESP.getFreeHeap();
        uint32_t totalH = ESP.getHeapSize();
        int ramPct = 100 - freeH * 100 / totalH;
        _barRow(y+12, "RAM", ramPct, C_GREEN,
                String(freeH/1024) + "kB free / " + String(totalH/1024) + "kB");
        // ── Flash ─────────────────────────────────────────────────────────
        uint32_t skSz  = ESP.getSketchSize();
        uint32_t skFr  = ESP.getFreeSketchSpace();
        int flPct = skSz * 100 / (skSz + skFr);
        _barRow(y+28, "FLASH", flPct, C_YELLOW,
                String(skSz/1024) + "kB / " + String((skSz+skFr)/1024) + "kB");
        // ── Battery ───────────────────────────────────────────────────────
        int batt = M5.Power.getBatteryLevel();
        bool chg = M5.Power.isCharging();
        _barRow(y+44, "BATT", batt, batt > 50 ? C_GREEN : (batt > 20 ? C_YELLOW : C_ACCENT),
                String(batt) + "%" + (chg ? " CHG" : ""));
        // ── Temperature ───────────────────────────────────────────────────
        float temp = 0; M5.Imu.getTemp(&temp);
        _row(y+60, "TEMP", String(temp, 1) + " C", C_ORANGE);
        // ── WiFi ─────────────────────────────────────────────────────────
        bool wcon = WiFi.isConnected();
        if (wcon) {
            _row(y+72, "WiFi", WiFi.SSID() + "  " + WiFi.localIP().toString(), C_CYAN);
            int rssi = WiFi.RSSI();
            _barRow(y+84, "RSSI", map(constrain(rssi,-90,-30),-90,-30,0,100),
                    C_GREEN, String(rssi) + " dBm");
        } else {
            _row(y+72, "WiFi", "not connected", C_DGRAY);
        }
        // ── ESP info ──────────────────────────────────────────────────────
        spr.setTextColor(C_DGRAY, C_BG);
        spr.setTextSize(1);
        spr.setCursor(4, CNT_H - 10);
        spr.printf("ESP32 %s  SDK %s", ESP.getChipModel(), ESP.getSdkVersion());

        spr.pushSprite(0, CNT_Y);
    }

private:
    void _row(int y, const char* label, String val, uint16_t col) {
        spr.setTextSize(1);
        spr.setTextColor(col, C_BG);
        spr.setCursor(4, y); spr.print(label);
        spr.setTextColor(C_WHITE, C_BG);
        spr.setCursor(60, y); spr.print(val);
    }

    void _barRow(int y, const char* label, int pct, uint16_t col, String info) {
        spr.setTextSize(1);
        spr.setTextColor(col, C_BG);
        spr.setCursor(4, y); spr.print(label);
        int bx = 60, bw = 110, bh = 8;
        spr.drawRect(bx, y, bw, bh, C_DGRAY);
        spr.fillRect(bx+1, y+1, pct*(bw-2)/100, bh-2, col);
        spr.setTextColor(C_WHITE, C_BG);
        spr.setCursor(bx + bw + 4, y); spr.print(info);
    }
};
