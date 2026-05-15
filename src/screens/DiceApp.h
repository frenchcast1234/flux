#pragma once
#include "Screen.h"

class DiceApp : public Screen {
    M5Canvas spr{&M5.Lcd};
    int      numDice   = 1;
    int      vals[3]   = {1, 1, 1};
    int      total     = 0;
    bool     rolling   = false;
    uint32_t rollStart = 0;
    static constexpr int ROLL_DUR = 600; // ms of animation

public:
    void onEnter() override { spr.createSprite(SCR_W, CNT_H); onDraw(); }
    void onExit()  override { spr.deleteSprite(); }

    void onButtonA() override { // roll
        rolling   = true;
        rollStart = millis();
        total     = 0;
        for (int d = 0; d < numDice; d++) { vals[d] = random(1, 7); total += vals[d]; }
        onDraw();
    }
    void onButtonB() override { // change number of dice (1/2/3)
        numDice = numDice % 3 + 1;
        onDraw();
    }
    void onLoop() override {
        if (rolling) {
            if (millis() - rollStart < ROLL_DUR) {
                // animate random values
                total = 0;
                for (int d = 0; d < numDice; d++) { vals[d] = random(1, 7); total += vals[d]; }
                onDraw();
                delay(60);
            } else {
                rolling = false;
                // finalize
                total = 0;
                for (int d = 0; d < numDice; d++) total += vals[d];
                onDraw();
            }
        }
    }

    void onDraw() override {
        spr.fillSprite(C_BG);

        int dw = 60; // die width
        int dx_start = (SCR_W - numDice * (dw + 8)) / 2 + 4;

        for (int d = 0; d < numDice; d++) {
            int x = dx_start + d * (dw + 8);
            int y = (CNT_H - dw) / 2 - 10;
            _drawDie(x, y, dw, vals[d], rolling ? C_DGRAY : C_WHITE);
        }

        // total
        if (numDice > 1) {
            spr.setTextColor(C_YELLOW, C_BG);
            spr.setTextSize(2);
            char tb[8]; snprintf(tb, sizeof(tb), "= %d", total);
            int tw = strlen(tb) * 12;
            spr.setCursor((SCR_W - tw) / 2, CNT_H - 26);
            spr.print(tb);
        }

        // hints
        spr.setTextColor(C_DGRAY, C_BG);
        spr.setTextSize(1);
        spr.setCursor(4, CNT_H - 10);
        spr.printf("[A] roll  [B] dice: %d", numDice);

        spr.pushSprite(0, CNT_Y);
    }

private:
    // Draw a die face at (x,y) size s×s with value v
    void _drawDie(int x, int y, int s, int v, uint16_t col) {
        // background
        spr.fillRoundRect(x, y, s, s, 6, C_PANEL);
        spr.drawRoundRect(x, y, s, s, 6, col);

        // dot positions (normalized 0.0-1.0 within face)
        static const float DOT_X[6][6] = {
            {0.5f},                                              // 1
            {0.25f, 0.75f},                                      // 2
            {0.25f, 0.5f,  0.75f},                               // 3
            {0.25f, 0.75f, 0.25f, 0.75f},                        // 4
            {0.25f, 0.75f, 0.5f,  0.25f, 0.75f},                 // 5
            {0.25f, 0.75f, 0.25f, 0.75f, 0.25f, 0.75f},          // 6
        };
        static const float DOT_Y[6][6] = {
            {0.5f},
            {0.25f, 0.75f},
            {0.25f, 0.5f,  0.75f},
            {0.25f, 0.25f, 0.75f, 0.75f},
            {0.25f, 0.25f, 0.5f,  0.75f, 0.75f},
            {0.25f, 0.25f, 0.5f,  0.5f,  0.75f, 0.75f},
        };

        int idx = constrain(v, 1, 6) - 1;
        int dotR = s / 12;
        for (int d = 0; d < v; d++) {
            int dx = x + 4 + (int)(DOT_X[idx][d] * (s - 8));
            int dy = y + 4 + (int)(DOT_Y[idx][d] * (s - 8));
            spr.fillCircle(dx, dy, dotR + 1, col);
        }
    }
};
