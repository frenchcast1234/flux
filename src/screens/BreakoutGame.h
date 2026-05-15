#pragma once
#include "Screen.h"

class BreakoutGame : public Screen {
    M5Canvas spr{&M5.Lcd};

    static constexpr int BCOLS = 8, BROWS = 5;
    static constexpr int BW = 28, BH = 10, BGAP = 2; // brick w/h/gap
    static constexpr int PAD_W = 40, PAD_H = 5;
    static constexpr int PAD_Y = CNT_H - 8;
    static constexpr int BALL_R = 4;

    uint8_t bricks[BROWS][BCOLS]; // 0=gone, 1-5=alive (row color)
    float   bx, by, bdx, bdy;
    int     padX;
    int     lives=3, score=0, hiscore=0;
    bool    over=false, won=false, paused=true;
    uint32_t lastFrame=0;

    static constexpr uint16_t ROW_COLORS[5] = {
        C_ACCENT, C_ORANGE, C_YELLOW, C_GREEN, C_CYAN
    };
    static constexpr int ROW_PTS[5] = {50, 40, 30, 20, 10};

public:
    void onEnter() override { spr.createSprite(SCR_W, CNT_H); _reset(); }
    void onExit()  override { spr.deleteSprite(); }

    void onButtonA() override {
        if (over||won) { _reset(); return; }
        padX = max(0, padX - 10);
        paused = false;
        onDraw();
    }
    void onButtonB() override {
        if (over||won) { _reset(); return; }
        padX = min(SCR_W - PAD_W, padX + 10);
        paused = false;
        onDraw();
    }

    void onLoop() override {
        if (paused||over||won) {
            // auto-move paddle via tilt in pause too
            float ax,ay,az; M5.Imu.getAccelData(&ax,&ay,&az);
            padX = constrain(padX + (int)(ay*8), 0, SCR_W - PAD_W);
            onDraw(); delay(30); return;
        }
        if (millis()-lastFrame < 25) return;
        lastFrame = millis();

        // tilt paddle
        float ax,ay,az; M5.Imu.getAccelData(&ax,&ay,&az);
        padX = constrain(padX + (int)(ay*6), 0, SCR_W - PAD_W);

        bx += bdx; by += bdy;

        // wall bounce
        if (bx - BALL_R < 0)          { bx = BALL_R;           bdx = fabsf(bdx); }
        if (bx + BALL_R >= SCR_W)      { bx = SCR_W-BALL_R-1;  bdx = -fabsf(bdx); }
        if (by - BALL_R < 0)           { by = BALL_R;           bdy = fabsf(bdy); }

        // paddle bounce
        if (by + BALL_R >= PAD_Y && by + BALL_R <= PAD_Y + PAD_H &&
            bx >= padX && bx <= padX + PAD_W) {
            float rel = (bx - padX - PAD_W/2.0f) / (PAD_W/2.0f);
            float angle = rel * 55.0f * DEG_TO_RAD;
            float spd = min(sqrtf(bdx*bdx+bdy*bdy)+0.05f, 5.5f);
            bdx =  spd * sinf(angle);
            bdy = -fabsf(spd * cosf(angle));
        }

        // brick collision
        for (int r=0; r<BROWS; r++) for (int c=0; c<BCOLS; c++) {
            if (!bricks[r][c]) continue;
            int brickX = 2 + c*(BW+BGAP);
            int brickY = 2 + r*(BH+BGAP);
            if (bx+BALL_R > brickX && bx-BALL_R < brickX+BW &&
                by+BALL_R > brickY && by-BALL_R < brickY+BH) {
                bricks[r][c] = 0;
                score += ROW_PTS[r];
                if (score > hiscore) hiscore = score;
                // decide reflection
                float overlapX = min(bx+BALL_R-brickX, brickX+BW-(bx-BALL_R));
                float overlapY = min(by+BALL_R-brickY, brickY+BH-(by-BALL_R));
                if (overlapX < overlapY) bdx = -bdx; else bdy = -bdy;
                goto checkWin;
            }
        }
        checkWin:
        // check win
        {bool anyLeft=false;
        for (int r=0;r<BROWS;r++) for(int c=0;c<BCOLS;c++) if(bricks[r][c]) anyLeft=true;
        if (!anyLeft) { won=true; onDraw(); return; }}

        // ball lost
        if (by - BALL_R > CNT_H) {
            lives--;
            if (lives <= 0) { over=true; onDraw(); return; }
            _resetBall();
        }
        onDraw();
    }

    void onDraw() override {
        spr.fillSprite(C_BG);

        // bricks
        for (int r=0; r<BROWS; r++) for (int c=0; c<BCOLS; c++) {
            if (!bricks[r][c]) continue;
            int x = 2 + c*(BW+BGAP), y = 2 + r*(BH+BGAP);
            spr.fillRect(x, y, BW, BH, ROW_COLORS[r]);
            spr.drawRect(x, y, BW, BH, C_BG);
        }

        // paddle
        spr.fillRoundRect(padX, PAD_Y, PAD_W, PAD_H, 2, C_WHITE);

        // ball
        spr.fillCircle((int)bx, (int)by, BALL_R, C_YELLOW);

        // lives hearts
        for (int i=0; i<lives; i++) {
            int hx = 4 + i*14;
            spr.fillCircle(hx,   CNT_H-3, 3, C_ACCENT);
            spr.fillCircle(hx+6, CNT_H-3, 3, C_ACCENT);
            spr.fillTriangle(hx-3,CNT_H-2, hx+9,CNT_H-2, hx+3,CNT_H+4, C_ACCENT);
        }

        // score
        spr.setTextSize(1); spr.setTextColor(C_WHITE, C_BG);
        spr.setCursor(SCR_W-60, CNT_H-10);
        spr.printf("SC:%d", score);

        if (paused) {
            spr.setTextColor(C_CYAN, C_BG); spr.setTextSize(1);
            spr.setCursor(70, CNT_H/2 - 4);
            spr.print("Tilt to move, A/B start");
        }

        if (over) {
            spr.fillRect(30,42,180,46, C_PANEL); spr.drawRect(30,42,180,46,C_ACCENT);
            spr.setTextColor(C_ACCENT, C_PANEL); spr.setTextSize(2);
            spr.setCursor(55,48); spr.print("GAME OVER");
            spr.setTextSize(1); spr.setTextColor(C_WHITE, C_PANEL);
            spr.setCursor(52,70); spr.printf("Score:%d  Best:%d", score, hiscore);
        }
        if (won) {
            spr.fillRect(30,42,180,46, C_PANEL); spr.drawRect(30,42,180,46,C_GREEN);
            spr.setTextColor(C_GREEN, C_PANEL); spr.setTextSize(2);
            spr.setCursor(58,48); spr.print("YOU WIN!");
            spr.setTextSize(1); spr.setTextColor(C_WHITE, C_PANEL);
            spr.setCursor(60,70); spr.printf("Score:%d", score);
        }

        spr.pushSprite(0, CNT_Y);
    }

private:
    void _reset() {
        for (int r=0;r<BROWS;r++) for(int c=0;c<BCOLS;c++) bricks[r][c]=1;
        score=0; lives=3; over=false; won=false; paused=true;
        padX = SCR_W/2 - PAD_W/2;
        _resetBall();
    }
    void _resetBall() {
        bx=SCR_W/2; by=CNT_H/2;
        float a=(random(60)-30)*DEG_TO_RAD;
        bdx=2.5f*sinf(a); bdy=-fabsf(2.5f*cosf(a));
        paused=true;
    }
};
constexpr uint16_t BreakoutGame::ROW_COLORS[5];
constexpr int      BreakoutGame::ROW_PTS[5];
