#pragma once
#include "Screen.h"

// 2D bubble level + individual axis bars
class LevelApp : public Screen {
    M5Canvas spr{&M5.Lcd};
    uint32_t lastDraw = 0;
    float    offX = 0, offY = 0;  // calibration offsets
    bool     showBars = false;

public:
    void onEnter() override { spr.createSprite(SCR_W, CNT_H); }
    void onExit()  override { spr.deleteSprite(); }

    void onButtonA() override { // calibrate
        float ax, ay, az;
        M5.Imu.getAccelData(&ax, &ay, &az);
        offX = ax; offY = ay;
        onDraw();
    }
    void onButtonB() override { showBars = !showBars; onDraw(); }

    void onLoop() override {
        if (millis() - lastDraw < 50) return;
        lastDraw = millis();
        onDraw();
    }

    void onDraw() override {
        float ax, ay, az;
        M5.Imu.getAccelData(&ax, &ay, &az);
        float rx = ax - offX, ry = ay - offY;

        spr.fillSprite(C_BG);

        if (!showBars) {
            // ── 2D bubble level ─────────────────────────────────────────
            int cx = 80, cy = CNT_H / 2;
            int R  = 50;

            // outer ring
            spr.drawCircle(cx, cy, R, C_DGRAY);
            // tolerance rings
            spr.drawCircle(cx, cy, R*2/3, C_PANEL);
            spr.drawCircle(cx, cy, R/3,   C_DGREEN);
            // crosshair
            spr.drawFastHLine(cx - R - 5, cy, (R + 5)*2, C_PANEL);
            spr.drawFastVLine(cx, cy - R - 5, (R + 5)*2, C_PANEL);

            // bubble position (clamp within outer ring)
            float bx = constrain(-ry * R * 1.2f, -(float)R+8, (float)R-8);
            float by = constrain(-rx * R * 1.2f, -(float)R+8, (float)R-8);

            // distance from center determines colour
            float dist = sqrtf(bx*bx + by*by);
            uint16_t bc = (dist < R/3) ? C_GREEN :
                          (dist < R*2/3) ? C_YELLOW : C_ACCENT;

            spr.fillCircle(cx + (int)bx, cy + (int)by, 10, bc);
            spr.drawCircle(cx + (int)bx, cy + (int)by, 10, C_WHITE);

            // divider
            spr.drawFastVLine(168, 0, CNT_H, C_ACCENT);

            // numerical readout
            float pitch = atan2f(ax, sqrtf(ay*ay + az*az)) * RAD_TO_DEG;
            float roll  = atan2f(-ay, -az) * RAD_TO_DEG;
            spr.setTextSize(1);
            spr.setTextColor(C_CYAN, C_BG);
            spr.setCursor(172, 8);  spr.printf("PITCH");
            spr.setTextSize(2);
            spr.setTextColor(bc, C_BG);
            spr.setCursor(172, 18); spr.printf("%+5.1f", pitch);
            spr.setTextSize(1);
            spr.setTextColor(C_CYAN, C_BG);
            spr.setCursor(172, 42); spr.printf("ROLL");
            spr.setTextSize(2);
            spr.setTextColor(bc, C_BG);
            spr.setCursor(172, 52); spr.printf("%+5.1f", roll);

            bool level = dist < R/3;
            spr.setTextSize(1);
            spr.setTextColor(level ? C_GREEN : C_ACCENT, C_BG);
            spr.setCursor(170, 82);
            spr.print(level ? "LEVEL" : "TILT");
            spr.setTextColor(C_DGRAY, C_BG);
            spr.setCursor(170, 95);
            spr.print("[A] cal");
            spr.setCursor(170, 106);
            spr.print("[B] bars");

        } else {
            // ── bar view ─────────────────────────────────────────────────
            float pitch = atan2f(ax, sqrtf(ay*ay + az*az)) * RAD_TO_DEG;
            float roll  = atan2f(-ay, -az) * RAD_TO_DEG;

            _drawBar(20, 20, "PITCH", pitch, C_CYAN);
            _drawBar(20, 60, "ROLL",  roll,  C_YELLOW);

            spr.setTextColor(C_DGRAY, C_BG);
            spr.setTextSize(1);
            spr.setCursor(4, CNT_H - 10);
            spr.print("[A] calibrate  [B] 2D view");
        }

        spr.pushSprite(0, CNT_Y);
    }

private:
    void _drawBar(int x, int y, const char* label, float val, uint16_t col) {
        spr.setTextSize(1); spr.setTextColor(col, C_BG);
        spr.setCursor(x, y); spr.print(label);
        char vbuf[10]; snprintf(vbuf, sizeof(vbuf), "%+6.1f", val);
        spr.setTextColor(C_WHITE, C_BG);
        spr.setCursor(x + 100, y); spr.print(vbuf);

        int bw = 200, bh = 14;
        spr.drawRect(x, y+10, bw, bh, C_DGRAY);
        int mid = x + bw/2;
        spr.drawFastVLine(mid, y+10, bh, C_DGRAY);

        int fill = (int)(val * (bw/2) / 90.0f);
        fill = constrain(fill, -bw/2+1, bw/2-1);
        if (fill > 0) spr.fillRect(mid, y+11, fill, bh-2, col);
        else          spr.fillRect(mid+fill, y+11, -fill, bh-2, col);
    }
};
