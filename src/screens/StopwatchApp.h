#pragma once
#include "Screen.h"

class StopwatchApp : public Screen {
    M5Canvas spr{&M5.Lcd};

    bool     running  = false;
    uint32_t elapsed  = 0;      // ms
    uint32_t startAt  = 0;
    static constexpr int MAX_LAPS = 6;
    uint32_t laps[MAX_LAPS] = {};
    int      lapCount = 0;
    uint32_t lastDraw = 0;

public:
    void onEnter() override { spr.createSprite(SCR_W, CNT_H); }
    void onExit()  override { spr.deleteSprite(); }

    void onButtonA() override { // start / stop
        if (running) {
            elapsed  += millis() - startAt;
            running   = false;
        } else {
            startAt = millis();
            running = true;
        }
        onDraw();
    }

    void onButtonB() override { // lap / reset
        if (running) {
            if (lapCount < MAX_LAPS) laps[lapCount++] = elapsed + (millis() - startAt);
            else {
                memmove(&laps[0], &laps[1], (MAX_LAPS-1)*sizeof(uint32_t));
                laps[MAX_LAPS-1] = elapsed + (millis() - startAt);
            }
        } else {
            elapsed = 0; lapCount = 0; memset(laps, 0, sizeof(laps));
        }
        onDraw();
    }

    void onLoop() override {
        if (running && millis() - lastDraw >= 33) { lastDraw = millis(); onDraw(); }
    }

    void onDraw() override {
        spr.fillSprite(C_BG);

        uint32_t total = running ? elapsed + (millis() - startAt) : elapsed;
        uint32_t cs    = (total / 10)  % 100;
        uint32_t sec   = (total / 1000) % 60;
        uint32_t mn    = (total / 60000) % 60;
        uint32_t hr    = total / 3600000;

        // large timer
        char main[16];
        if (hr > 0) snprintf(main, sizeof(main), "%02lu:%02lu:%02lu", hr, mn, sec);
        else        snprintf(main, sizeof(main), "%02lu:%02lu", mn, sec);

        spr.setTextColor(running ? C_GREEN : C_WHITE, C_BG);
        spr.setTextSize(4);
        int tw = strlen(main) * 24;
        spr.setCursor((SCR_W - tw) / 2, 4);
        spr.print(main);

        // centiseconds
        char cs_buf[4]; snprintf(cs_buf, sizeof(cs_buf), ".%02lu", cs);
        spr.setTextColor(C_ACCENT, C_BG);
        spr.setTextSize(2);
        spr.setCursor((SCR_W + tw) / 2 + 2, 16);
        spr.print(cs_buf);

        // progress ring (seconds around a circle arc)
        int cx = SCR_W/2, cy = 55, R = 22;
        spr.drawCircle(cx, cy, R, C_PANEL);
        if (sec > 0 || cs > 0) {
            float endA = -90.0f + (float)(sec * 60 + cs) * 6.0f / 10.0f;
            spr.drawArc(cx, cy, R, R-3, -90, endA, running ? C_GREEN : C_DGRAY);
        }
        spr.setTextColor(C_DGRAY, C_BG);
        spr.setTextSize(1);
        spr.setCursor(cx - 6, cy - 4);
        spr.printf("%02lu\"", sec);

        // divider
        spr.drawFastHLine(10, 48, SCR_W - 20, C_PANEL);

        // lap list
        spr.setTextSize(1);
        for (int i = 0; i < lapCount; i++) {
            int li = lapCount - 1 - i; // newest first
            if (i >= 5) break;
            uint32_t lt = laps[li];
            uint32_t lcs  = (lt/10)%100, lsec = (lt/1000)%60, lmn = (lt/60000)%60;
            char lb[20]; snprintf(lb, sizeof(lb), "L%d  %02lu:%02lu.%02lu", li+1, lmn, lsec, lcs);
            uint16_t lc = (i == 0) ? C_CYAN : C_DGRAY;
            spr.setTextColor(lc, C_BG);
            spr.setCursor(30, 54 + i * 12);
            spr.print(lb);
        }

        // hints
        spr.setTextColor(C_DGRAY, C_BG);
        spr.setCursor(4, CNT_H - 10);
        spr.print(running ? "[A] stop  [B] lap" : "[A] start  [B] reset");

        spr.pushSprite(0, CNT_Y);
    }
};
