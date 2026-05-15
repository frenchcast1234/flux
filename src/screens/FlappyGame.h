#pragma once
#include "Screen.h"

// Flappy Bird — BtnA or tilt-forward to flap
class FlappyGame : public Screen {
    M5Canvas spr{&M5.Lcd};

    static constexpr int BIRD_X  = 45;
    static constexpr int BIRD_R  = 7;
    static constexpr int PIPE_W  = 22;
    static constexpr int GAP_H   = 42;
    static constexpr int PIPE_SPD = 2;
    static constexpr int PIPE_INTERVAL = 85; // px between pipe starts

    struct Pipe { int x; int gapY; bool passed; };

    float    birdy=0, birdVy=0;
    Pipe     pipes[3];
    int      score=0, hiscore=0;
    bool     over=false, started=false;
    uint32_t lastFrame=0;
    bool     flapFlag=false;

public:
    void onEnter() override { spr.createSprite(SCR_W, CNT_H); _reset(); }
    void onExit()  override { spr.deleteSprite(); }

    void onButtonA() override {
        if (over) { _reset(); return; }
        started = true; flapFlag = true;
    }
    void onButtonB() override { onButtonA(); }

    void onLoop() override {
        if (!started||over) { onDraw(); delay(40); return; }
        if (millis()-lastFrame < 30) return;
        lastFrame = millis();

        // gyro flap: tilt forward (ax < -0.3)
        float ax,ay,az; M5.Imu.getAccelData(&ax,&ay,&az);
        if (ax < -0.35f) flapFlag = true;

        if (flapFlag) { birdVy = -3.2f; flapFlag=false; }

        // gravity
        birdVy += 0.22f;
        birdy  += birdVy;
        birdy   = constrain(birdy, (float)BIRD_R, (float)(CNT_H - BIRD_R - 1));

        // move pipes
        for (auto& p : pipes) {
            p.x -= PIPE_SPD;
            if (p.x + PIPE_W < 0) {
                p.x    = SCR_W + PIPE_INTERVAL;
                p.gapY = random(20, CNT_H - GAP_H - 20);
                p.passed = false;
            }
            // score
            if (!p.passed && p.x + PIPE_W < BIRD_X) { score++; p.passed=true; }

            // collision
            if (BIRD_X + BIRD_R > p.x && BIRD_X - BIRD_R < p.x + PIPE_W) {
                if ((int)birdy - BIRD_R < p.gapY ||
                    (int)birdy + BIRD_R > p.gapY + GAP_H) {
                    over=true;
                    if (score>hiscore) hiscore=score;
                }
            }
        }
        // floor / ceiling collision
        if (birdy >= CNT_H - BIRD_R - 1) { over=true; if(score>hiscore) hiscore=score; }

        onDraw();
    }

    void onDraw() override {
        spr.fillSprite(0x035Fu); // dark sky blue

        // ground
        spr.fillRect(0, CNT_H-8, SCR_W, 8, C_DGREEN);
        spr.fillRect(0, CNT_H-9, SCR_W, 2, C_GREEN);

        // pipes
        for (auto& p : pipes) {
            uint16_t pc = C_DGREEN;
            // top pipe
            spr.fillRect(p.x, 0, PIPE_W, p.gapY, pc);
            spr.drawRect(p.x, 0, PIPE_W, p.gapY, C_GREEN);
            spr.fillRect(p.x-2, p.gapY-8, PIPE_W+4, 8, pc);
            spr.drawRect(p.x-2, p.gapY-8, PIPE_W+4, 8, C_GREEN);
            // bottom pipe
            int bot = p.gapY + GAP_H;
            spr.fillRect(p.x, bot, PIPE_W, CNT_H - bot, pc);
            spr.drawRect(p.x, bot, PIPE_W, CNT_H - bot, C_GREEN);
            spr.fillRect(p.x-2, bot, PIPE_W+4, 8, pc);
            spr.drawRect(p.x-2, bot, PIPE_W+4, 8, C_GREEN);
        }

        // bird body
        uint16_t bcol = over ? C_ACCENT : C_YELLOW;
        spr.fillCircle(BIRD_X, (int)birdy, BIRD_R, bcol);
        // wing flap visual
        spr.fillEllipse(BIRD_X-3, (int)birdy+2, 5, 3, C_ORANGE);
        // eye
        spr.fillCircle(BIRD_X+3, (int)birdy-2, 2, C_WHITE);
        spr.fillCircle(BIRD_X+4, (int)birdy-2, 1, C_BLACK);
        // beak
        spr.fillTriangle(BIRD_X+BIRD_R-1, (int)birdy,
                         BIRD_X+BIRD_R+3, (int)birdy-1,
                         BIRD_X+BIRD_R+3, (int)birdy+2, C_ORANGE);

        // score
        spr.setTextSize(2); spr.setTextColor(C_WHITE, 0x035Fu);
        spr.setCursor(SCR_W/2 - 10, 4);
        spr.printf("%d", score);

        if (!started) {
            spr.setTextSize(1); spr.setTextColor(C_WHITE, 0x035Fu);
            spr.setCursor(55, CNT_H/2-12);
            spr.print("Tilt forward to flap");
            spr.setCursor(68, CNT_H/2);
            spr.print("or press [A]");
        }

        if (over) {
            spr.fillRect(50,32,140,52, C_PANEL); spr.drawRect(50,32,140,52,C_ACCENT);
            spr.setTextColor(C_ACCENT, C_PANEL); spr.setTextSize(2);
            spr.setCursor(56,38); spr.print("GAME OVER");
            spr.setTextSize(1); spr.setTextColor(C_WHITE, C_PANEL);
            spr.setCursor(58,60); spr.printf("Score:%d Best:%d", score, hiscore);
            spr.setTextColor(C_CYAN, C_PANEL);
            spr.setCursor(60,72); spr.print("[A] restart");
        }

        spr.pushSprite(0, CNT_Y);
    }

private:
    void _reset() {
        birdy=CNT_H/2; birdVy=0; score=0; over=false; started=false; flapFlag=false;
        for (int i=0; i<3; i++) {
            pipes[i].x      = SCR_W + 20 + i * PIPE_INTERVAL;
            pipes[i].gapY   = random(20, CNT_H - GAP_H - 20);
            pipes[i].passed = false;
        }
    }
};
