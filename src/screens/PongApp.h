#pragma once
#include "Screen.h"

class PongApp : public Screen {
    M5Canvas spr{&M5.Lcd};

    static constexpr int PW = 4, PH = 26;  // paddle size
    static constexpr int BW = 5, BH = 5;   // ball size
    static constexpr int PMOVE = 4;        // paddle speed px/frame
    static constexpr int SCORE_WIN = 7;

    float bx, by, bdx, bdy;
    int   ly, ry; // paddle Y centres
    int   lScore = 0, rScore = 0;
    bool  over   = false;
    bool  paused = true;
    uint32_t lastFrame = 0;

    int   playH;    // play area height
    int   playY;    // play area start y (in sprite)

public:
    void onEnter() override {
        spr.createSprite(SCR_W, CNT_H);
        playY = 16; playH = CNT_H - playY;
        _reset();
    }
    void onExit() override { spr.deleteSprite(); }

    void onButtonA() override { // paddle up
        if (over) { _newGame(); return; }
        paused = false;
        ly = max(playY + PH/2, ly - PMOVE*2);
        onDraw();
    }
    void onButtonB() override { // paddle down
        if (over) { _newGame(); return; }
        paused = false;
        ly = min(playY + playH - PH/2, ly + PMOVE*2);
        onDraw();
    }

    void onLoop() override {
        if (paused || over) return;
        uint32_t now = millis();
        if (now - lastFrame < 33) return; // ~30fps
        lastFrame = now;
        _update();
        onDraw();
    }

    void onDraw() override {
        spr.fillSprite(C_BG);

        // header score
        spr.fillRect(0, 0, SCR_W, playY, C_PANEL);
        spr.setTextSize(2);
        spr.setTextColor(C_WHITE, C_PANEL);
        spr.setCursor(85, 0);  spr.print(lScore);
        spr.setCursor(115, 0); spr.print(":");
        spr.setCursor(130, 0); spr.print(rScore);

        // net
        for (int y = playY; y < CNT_H; y += 8)
            spr.fillRect(SCR_W/2 - 1, y, 2, 4, C_DGRAY);

        // paddles
        spr.fillRoundRect(4,         ly - PH/2, PW, PH, 2, C_CYAN);
        spr.fillRoundRect(SCR_W-4-PW, ry - PH/2, PW, PH, 2, C_ACCENT);

        // ball
        uint16_t bc = (millis()/100 % 2) ? C_WHITE : C_YELLOW;
        spr.fillRect((int)bx, (int)by, BW, BH, bc);

        // trails
        spr.fillRect((int)(bx - bdx), (int)(by - bdy), BW, BH, C_DGRAY);
        spr.fillRect((int)(bx - 2*bdx), (int)(by - 2*bdy), BW, BH, C_PANEL);

        if (paused) {
            spr.setTextSize(1);
            spr.setTextColor(C_CYAN, C_BG);
            spr.setCursor(85, CNT_H/2 - 4);
            spr.print("Press A/B");
        }

        if (over) {
            spr.fillRect(40, 30, 160, 50, C_PANEL);
            spr.drawRect(40, 30, 160, 50, C_ACCENT);
            spr.setTextColor(C_ACCENT, C_PANEL);
            spr.setTextSize(2);
            spr.setCursor(55, 36);
            spr.print(lScore >= SCORE_WIN ? "YOU WIN!" : "YOU LOSE");
            spr.setTextSize(1);
            spr.setTextColor(C_CYAN, C_PANEL);
            spr.setCursor(75, 65);
            spr.print("A or B to replay");
        }

        spr.pushSprite(0, CNT_Y);
    }

private:
    void _reset() {
        bx = SCR_W/2.0f - BW/2; by = playY + playH/2.0f;
        float ang = (random(60) - 30) * DEG_TO_RAD;
        float spd = 2.5f;
        bdx = spd * cosf(ang) * (random(2) ? 1 : -1);
        bdy = spd * sinf(ang);
        ly  = playY + playH/2;
        ry  = playY + playH/2;
        paused = true;
    }

    void _newGame() {
        lScore = 0; rScore = 0; over = false; _reset();
    }

    void _update() {
        bx += bdx; by += bdy;

        // wall bounce
        if (by <= playY)          { by = playY;           bdy = fabsf(bdy); }
        if (by + BH >= CNT_H)     { by = CNT_H - BH;      bdy = -fabsf(bdy); }

        // AI right paddle
        int rTarget = (int)(by + BH/2);
        if (ry < rTarget - 2) ry = min(ry + PMOVE, rTarget);
        else if (ry > rTarget + 2) ry = max(ry - PMOVE, rTarget);
        ry = constrain(ry, playY + PH/2, playY + playH - PH/2);

        // left paddle collision
        if (bx <= 4 + PW && bx >= 4 &&
            by + BH >= ly - PH/2 && by <= ly + PH/2) {
            float rel = (by + BH/2 - ly) / (PH/2.0f);
            float ang = rel * 45.0f * DEG_TO_RAD;
            float spd = min(fabsf(bdx) + 0.15f, 6.0f);
            bdx =  fabsf(spd * cosf(ang));
            bdy =  spd * sinf(ang);
        }

        // right paddle collision
        if (bx + BW >= SCR_W - 4 - PW && bx + BW <= SCR_W - 4 &&
            by + BH >= ry - PH/2 && by <= ry + PH/2) {
            float rel = (by + BH/2 - ry) / (PH/2.0f);
            float ang = rel * 45.0f * DEG_TO_RAD;
            float spd = min(fabsf(bdx) + 0.15f, 6.0f);
            bdx = -fabsf(spd * cosf(ang));
            bdy =  spd * sinf(ang);
        }

        // scoring
        if (bx < 0)        { rScore++; _checkWin(); _reset(); }
        if (bx > SCR_W)    { lScore++; _checkWin(); _reset(); }
    }

    void _checkWin() {
        if (lScore >= SCORE_WIN || rScore >= SCORE_WIN) over = true;
    }
};
