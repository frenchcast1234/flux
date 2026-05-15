#pragma once
#include "Screen.h"

class SnakeGame : public Screen {
    M5Canvas spr{&M5.Lcd};

    static constexpr int CW   = 8;      // cell width px
    static constexpr int CH   = 8;      // cell height px
    static constexpr int GW   = 30;     // grid cols
    static constexpr int GH   = 14;     // grid rows (112px)
    static constexpr int SCORE_H = 5;   // px reserved below grid

    struct Pt { int8_t x, y; };

    Pt      body[GW * GH];
    int     bodyLen = 0;
    Pt      food    = {5, 5};
    int8_t  dir     = 0; // 0=R 1=D 2=L 3=U
    int8_t  nextDir = 0;
    int     score   = 0;
    int     hiscore = 0;
    bool    over    = false;
    uint32_t lastStep = 0;
    uint32_t speed    = 250; // ms per step

public:
    void onEnter() override { spr.createSprite(SCR_W, CNT_H); _reset(); }
    void onExit()  override { spr.deleteSprite(); }

    void onButtonA() override { // turn left
        if (over) { _reset(); onDraw(); return; }
        nextDir = (nextDir + 3) % 4; // CCW
    }
    void onButtonB() override { // turn right
        if (over) { _reset(); onDraw(); return; }
        nextDir = (nextDir + 1) % 4; // CW
    }

    void onLoop() override {
        if (over) return;
        if (millis() - lastStep < speed) return;
        lastStep = millis();
        _step();
        onDraw();
    }

    void onDraw() override {
        spr.fillSprite(C_BG);

        // grid border
        spr.drawRect(0, 0, GW*CW, GH*CH, C_PANEL);

        // food (blinking)
        uint16_t fc = (millis() / 300 % 2) ? C_ACCENT : C_ORANGE;
        spr.fillRect(food.x * CW + 1, food.y * CH + 1, CW-2, CH-2, fc);

        // snake body
        for (int i = 0; i < bodyLen; i++) {
            uint16_t col;
            if (i == 0)                   col = C_WHITE;
            else if (i < 3)              col = C_CYAN;
            else if (i < bodyLen / 2)    col = C_GREEN;
            else                          col = C_DGREEN;
            spr.fillRect(body[i].x * CW + 1, body[i].y * CH + 1, CW-2, CH-2, col);
        }

        // head eyes
        if (bodyLen > 0) {
            int hx = body[0].x * CW, hy = body[0].y * CH;
            switch (dir) {
            case 0: spr.fillRect(hx+5,hy+1,2,2,C_BG); spr.fillRect(hx+5,hy+5,2,2,C_BG); break;
            case 1: spr.fillRect(hx+1,hy+5,2,2,C_BG); spr.fillRect(hx+5,hy+5,2,2,C_BG); break;
            case 2: spr.fillRect(hx+1,hy+1,2,2,C_BG); spr.fillRect(hx+1,hy+5,2,2,C_BG); break;
            case 3: spr.fillRect(hx+1,hy+1,2,2,C_BG); spr.fillRect(hx+5,hy+1,2,2,C_BG); break;
            }
        }

        // score bar
        spr.setTextSize(1);
        spr.setTextColor(C_CYAN, C_BG);
        spr.setCursor(GW*CW + 6, 2);
        spr.printf("SCORE\n%d", score);
        spr.setTextColor(C_DGRAY, C_BG);
        spr.setCursor(GW*CW + 6, 22);
        spr.printf("BEST\n%d", hiscore);
        spr.setCursor(GW*CW + 6, 42);
        spr.printf("LEN\n%d", bodyLen);
        spr.setCursor(GW*CW + 6, 62);
        spr.setTextColor(C_DGRAY, C_BG);
        spr.print("[A] <-\n[B] ->");

        // speed bar
        int spd = map(speed, 80, 250, 100, 0);
        spr.drawRect(GW*CW + 4, 95, 28, 6, C_DGRAY);
        spr.fillRect(GW*CW + 4, 95, (int)(spd * 28 / 100), 6, C_ACCENT);
        spr.setCursor(GW*CW + 4, 104);
        spr.setTextColor(C_DGRAY, C_BG);
        spr.print("SPD");

        if (over) {
            // overlay
            spr.fillRect(30, 30, 180, 54, C_PANEL);
            spr.drawRect(30, 30, 180, 54, C_ACCENT);
            spr.setTextColor(C_ACCENT, C_PANEL);
            spr.setTextSize(2);
            spr.setCursor(55, 36);
            spr.print("GAME OVER");
            spr.setTextSize(1);
            spr.setTextColor(C_WHITE, C_PANEL);
            spr.setCursor(60, 58);
            spr.printf("Score: %d  Best: %d", score, hiscore);
            spr.setTextColor(C_CYAN, C_PANEL);
            spr.setCursor(70, 72);
            spr.print("A or B to restart");
        }

        spr.pushSprite(0, CNT_Y);
    }

private:
    void _reset() {
        bodyLen  = 4;
        dir      = 0;
        nextDir  = 0;
        score    = 0;
        speed    = 250;
        over     = false;
        lastStep = millis();
        // start at center heading right
        for (int i = 0; i < bodyLen; i++)
            body[i] = {(int8_t)(GW/2 - i), (int8_t)(GH/2)};
        _placeFood();
    }

    void _placeFood() {
        do {
            food.x = random(0, GW);
            food.y = random(0, GH);
        } while (_onSnake(food));
    }

    bool _onSnake(Pt p) {
        for (int i = 0; i < bodyLen; i++)
            if (body[i].x == p.x && body[i].y == p.y) return true;
        return false;
    }

    void _step() {
        // apply turn (no reverse)
        if (abs(nextDir - dir) != 2) dir = nextDir;

        const int8_t DX[] = {1, 0, -1, 0};
        const int8_t DY[] = {0, 1, 0, -1};

        Pt newHead = {(int8_t)(body[0].x + DX[dir]),
                      (int8_t)(body[0].y + DY[dir])};

        // wall collision
        if (newHead.x < 0 || newHead.x >= GW || newHead.y < 0 || newHead.y >= GH) {
            _gameOver(); return;
        }
        // self collision (skip tail which is about to move)
        for (int i = 0; i < bodyLen - 1; i++) {
            if (body[i].x == newHead.x && body[i].y == newHead.y) {
                _gameOver(); return;
            }
        }

        // ate food?
        bool ate = (newHead.x == food.x && newHead.y == food.y);
        if (!ate) bodyLen--; else {
            score += 10;
            speed  = max(80u, speed - 5u);
            _placeFood();
        }

        // shift body
        memmove(&body[1], &body[0], bodyLen * sizeof(Pt));
        body[0] = newHead;
        if (ate) bodyLen++;
    }

    void _gameOver() {
        over = true;
        if (score > hiscore) hiscore = score;
    }
};
