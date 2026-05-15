#pragma once
#include "Screen.h"
#include <WiFi.h>

class WifiScanner : public Screen {
    M5Canvas spr{&M5.Lcd};
    bool     scanning   = false;
    int      numNets    = 0;
    int      scrollOff  = 0;
    uint32_t lastScan   = 0;
    uint32_t scanStart  = 0;
    static constexpr int ROWS = 8;

public:
    void onEnter() override {
        spr.createSprite(SCR_W, CNT_H);
        WiFi.mode(WIFI_STA);
        WiFi.disconnect();
        startScan();
    }
    void onExit() override {
        WiFi.scanDelete();
        spr.deleteSprite();
    }

    void onButtonA() override { // scroll / rescan
        if (!scanning) {
            if (numNets > ROWS && scrollOff < numNets - ROWS)
                scrollOff++;
            else { scrollOff = 0; startScan(); }
            onDraw();
        }
    }
    void onButtonB() override { // rescan
        scrollOff = 0; startScan(); onDraw();
    }

    void onLoop() override {
        if (scanning) {
            int n = WiFi.scanComplete();
            if (n >= 0) {
                numNets  = n;
                scanning = false;
                onDraw();
            } else if (millis() - scanStart > 10000) {
                // timeout
                scanning = false;
                numNets  = 0;
                onDraw();
            }
        }
    }

    void onDraw() override {
        spr.fillSprite(C_BG);

        // ── header ─────────────────────────────────────────────────────
        spr.fillRect(0, 0, SCR_W, 12, C_PANEL);
        spr.setTextSize(1);
        spr.setTextColor(C_CYAN, C_PANEL);
        spr.setCursor(4, 2);
        if (scanning) {
            uint32_t dots = (millis() / 400) % 4;
            char buf[20]; snprintf(buf, sizeof(buf), "SCANNING%.*s", (int)dots, "...");
            spr.print(buf);
        } else {
            char buf[32]; snprintf(buf, sizeof(buf), "%d NETWORKS  [A] scroll  [B] rescan", numNets);
            spr.print(buf);
        }

        if (scanning) {
            // spinner
            int cx = 120, cy = CNT_H/2 + 10;
            for (int i = 0; i < 8; i++) {
                float a   = i * 45.0f * DEG_TO_RAD;
                float age = fmod((millis() / 100.0f - i) / 8.0f, 1.0f);
                uint16_t c = (uint16_t)(age * 255) >> 3;
                c = (c << 11) | (c << 6) | c;
                spr.fillCircle(cx + 18*cosf(a), cy + 18*sinf(a), 2, c);
            }
            spr.pushSprite(0, CNT_Y);
            return;
        }

        // ── network list ───────────────────────────────────────────────
        int yBase = 14;
        int rowH  = (CNT_H - 14) / ROWS; // ~12px per row

        for (int i = 0; i < ROWS && (i + scrollOff) < numNets; i++) {
            int idx = i + scrollOff;
            int y   = yBase + i * rowH;

            // alternating row bg
            spr.fillRect(0, y, SCR_W, rowH - 1, (i % 2) ? C_PANEL : C_BG);

            // signal strength bars (4 bars)
            int rssi   = WiFi.RSSI(idx);
            int bars   = map(constrain(rssi, -90, -30), -90, -30, 1, 4);
            uint16_t sc = rssi > -60 ? C_GREEN : (rssi > -75 ? C_YELLOW : C_ACCENT);
            for (int b = 0; b < 4; b++) {
                int bh = 3 + b * 2;
                uint16_t bc = (b < bars) ? sc : C_DGRAY;
                spr.fillRect(3 + b * 5, y + rowH - 2 - bh, 4, bh, bc);
            }

            // encryption icon
            bool enc = (WiFi.encryptionType(idx) != WIFI_AUTH_OPEN);
            if (enc) {
                spr.drawRect(24, y + 2, 6, 5, C_DGRAY);
                spr.fillRect(24, y + 5, 6, 4, C_DGRAY);
            }

            // SSID (truncate at 22 chars)
            String ssid = WiFi.SSID(idx);
            if (ssid.length() == 0) ssid = "<hidden>";
            if (ssid.length() > 22) ssid = ssid.substring(0, 21) + "~";
            spr.setTextColor(C_WHITE, (i%2) ? C_PANEL : C_BG);
            spr.setCursor(33, y + 2);
            spr.print(ssid);

            // channel + RSSI
            char info[16];
            snprintf(info, sizeof(info), "ch%d %ddBm", WiFi.channel(idx), rssi);
            spr.setTextColor(C_DGRAY, (i%2) ? C_PANEL : C_BG);
            spr.setCursor(170, y + 2);
            spr.print(info);
        }

        // scroll indicator
        if (numNets > ROWS) {
            int trkH = CNT_H - 14;
            int barH = trkH * ROWS / numNets;
            int barY = 14 + trkH * scrollOff / numNets;
            spr.fillRect(SCR_W - 3, 14, 3, trkH, C_PANEL);
            spr.fillRect(SCR_W - 3, barY, 3, barH, C_ACCENT);
        }

        spr.pushSprite(0, CNT_Y);
    }

private:
    void startScan() {
        scanning  = true;
        scanStart = millis();
        WiFi.scanNetworks(true, true); // async, show hidden
    }
};
