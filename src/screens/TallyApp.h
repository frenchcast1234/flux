#pragma once
#include "Screen.h"

class TallyApp : public Screen {
    M5Canvas spr{&M5.Lcd};
    int32_t  count    = 0;
    int32_t  session  = 0;   // increments since last reset
    bool     flashAnim = false;
    uint32_t flashAt   = 0;

public:
    void onEnter() override { spr.createSprite(SCR_W, CNT_H); onDraw(); }
    void onExit()  override { spr.deleteSprite(); }

    void onButtonA() override { // +1
        count++;  session++;
        flashAnim = true; flashAt = millis();
        M5.Speaker.tone(600 + (count % 20)*30, 40);
        onDraw();
    }
    void onButtonB() override { // -1 or reset (double tap in 300ms)
        static uint32_t lastB = 0;
        if (millis() - lastB < 300) { count=0; session=0; M5.Speaker.tone(220,80); }
        else count = max(0, count - 1);
        lastB = millis();
        onDraw();
    }

    void onLoop() override {
        if (flashAnim && millis()-flashAt > 120) { flashAnim=false; onDraw(); }
    }

    void onDraw() override {
        uint16_t bg = flashAnim ? C_PANEL2 : C_BG;
        spr.fillSprite(bg);

        // giant count
        char num[12]; snprintf(num, sizeof(num), "%ld", count);
        int numLen = strlen(num);
        int sz = (numLen <= 3) ? 5 : (numLen <= 5 ? 4 : 3);
        int charW = 6 * sz;
        int tw = numLen * charW;
        spr.setTextSize(sz);
        spr.setTextColor(flashAnim ? C_CYAN : C_WHITE, bg);
        spr.setCursor((SCR_W - tw) / 2, CNT_H/2 - sz*4 - 10);
        spr.print(num);

        // plus animation ring
        if (flashAnim) {
            spr.drawCircle(SCR_W/2, CNT_H/2, 50, C_CYAN);
            spr.drawCircle(SCR_W/2, CNT_H/2, 52, C_PANEL2);
        }

        // session
        spr.setTextColor(C_DGRAY, bg); spr.setTextSize(1);
        spr.setCursor(SCR_W/2 - 30, CNT_H/2 + sz*4 + 2);
        spr.printf("session: +%ld", session);

        // tick marks (last 10)
        int ticks = min((int)session, 10);
        for (int i=0; i<ticks; i++) {
            int tx = 20 + i * 20, ty = CNT_H - 20;
            if (i == 4 || i == 9) {
                // 5th tick diagonal
                spr.drawLine(tx-8, ty-12, tx+2, ty+2, C_ACCENT);
            } else {
                spr.drawFastVLine(tx, ty-12, 14, C_ACCENT);
            }
        }

        spr.setTextColor(C_DGRAY, bg); spr.setTextSize(1);
        spr.setCursor(4, CNT_H-10);
        spr.print("[A] count  [B] -1  [B]+[B] reset");

        spr.pushSprite(0, CNT_Y);
    }
};
