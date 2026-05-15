#pragma once
#include "Screen.h"

// Artificial horizon + 3-axis readout
class GyroApp : public Screen {
    M5Canvas spr{&M5.Lcd};
    uint32_t lastDraw = 0;

    // 0x034E ≈ #003470  dark sky blue
    static constexpr uint16_t SKY_COL    = 0x034Eu;
    // 0x4200 ≈ #400000  dark earth brown
    static constexpr uint16_t EARTH_COL  = 0x4220u;
    static constexpr int      HW = 120; // horizon panel width

public:
    void onEnter() override { spr.createSprite(SCR_W, CNT_H); }
    void onExit()  override { spr.deleteSprite(); }

    void onLoop() override {
        if (millis() - lastDraw < 50) return; // ~20 fps
        lastDraw = millis();
        onDraw();
    }

    void onDraw() override {
        float ax, ay, az, gx, gy, gz, temp;
        M5.Imu.getAccelData(&ax, &ay, &az);
        M5.Imu.getGyroData(&gx, &gy, &gz);
        M5.Imu.getTemp(&temp);

        // pitch / roll from accelerometer
        float pitch = atan2f(ax, sqrtf(ay*ay + az*az)) * RAD_TO_DEG;
        float roll  = atan2f(-ay, -az) * RAD_TO_DEG;

        spr.fillSprite(C_BG);

        // ── artificial horizon ───────────────────────────────────────────
        _drawHorizon(pitch, roll);

        // ── roll arc indicator at top of horizon panel ───────────────────
        {
            int cx = HW / 2, cy = 12;
            spr.drawArc(cx, 60, 55, 53, 120, 420, C_DGRAY);
            // roll marker
            float rRad = roll * DEG_TO_RAD;
            int mx = cx + 54 * sinf(rRad);
            int my = 60 - 54 * cosf(rRad);
            spr.fillTriangle(mx, my, mx - 3, my + 5, mx + 3, my + 5, C_YELLOW);
        }

        // ── centre cross-hair ─────────────────────────────────────────────
        {
            int cx = HW/2, cy = CNT_H/2;
            spr.drawFastHLine(cx - 15, cy, 10, C_WHITE);
            spr.drawFastHLine(cx + 5,  cy, 10, C_WHITE);
            spr.drawFastVLine(cx, cy - 5, 4, C_WHITE);
            spr.fillRect(cx - 1, cy - 1, 3, 3, C_WHITE);
        }

        // divider
        spr.drawFastVLine(HW, 0, CNT_H, C_ACCENT);

        // ── data panel ────────────────────────────────────────────────────
        int px = HW + 8;
        spr.setTextSize(1);

        // pitch & roll text
        spr.setTextColor(C_CYAN, C_BG);
        spr.setCursor(px, 4);  spr.printf("P %+6.1f", pitch);
        spr.setCursor(px, 14); spr.printf("R %+6.1f", roll);

        // pitch bar (vertical)
        int pitchPx = constrain((int)map((int)pitch, -90, 90, CNT_H-6, 6), 6, CNT_H-6);
        spr.drawRect(px + 83, 2, 6, CNT_H-4, C_DGRAY);
        spr.fillRect(px + 84, pitchPx - 3, 4, 6, C_CYAN);

        // roll bar (horizontal)
        int rollPx = constrain((int)map((int)roll, -90, 90, 0, 94), 0, 94);
        spr.drawRect(px, 26, 96, 6, C_DGRAY);
        spr.fillRect(px + rollPx - 2, 27, 4, 4, C_YELLOW);

        // gyro rates
        spr.setTextColor(C_GREEN, C_BG);
        spr.setCursor(px, 38);  spr.printf("GX%+6.0f", gx);
        spr.setCursor(px, 48);  spr.printf("GY%+6.0f", gy);
        spr.setCursor(px, 58);  spr.printf("GZ%+6.0f", gz);

        // accel raw
        spr.setTextColor(C_ORANGE, C_BG);
        spr.setCursor(px, 72);  spr.printf("AX%+5.2f", ax);
        spr.setCursor(px, 82);  spr.printf("AY%+5.2f", ay);
        spr.setCursor(px, 92);  spr.printf("AZ%+5.2f", az);

        // temperature
        spr.setTextColor(C_DGRAY, C_BG);
        spr.setCursor(px, 106); spr.printf("T %.1fC", temp);

        spr.pushSprite(0, CNT_Y);
    }

private:
    void _drawHorizon(float pitchDeg, float rollDeg) {
        float rollRad  = rollDeg * DEG_TO_RAD;
        float pitchPx  = pitchDeg * 0.8f; // px per degree
        float hx = HW / 2.0f;
        float hy = CNT_H / 2.0f - pitchPx;

        // fill sky
        spr.fillRect(0, 0, HW, CNT_H, SKY_COL);

        // fill earth with scanline
        float tanR = tanf(rollRad);
        for (int x = 0; x < HW; x++) {
            int yLine = (int)(hy + (x - hx) * tanR);
            if (yLine < CNT_H) {
                int yS = max(0, yLine);
                spr.drawFastVLine(x, yS, CNT_H - yS, EARTH_COL);
            }
        }

        // horizon line
        int y0 = (int)(hy + (0    - hx) * tanR);
        int y1 = (int)(hy + (HW-1 - hx) * tanR);
        spr.drawLine(0, y0, HW-1, y1, C_WHITE);

        // pitch lines (every 10°)
        for (int deg = -30; deg <= 30; deg += 10) {
            if (deg == 0) continue;
            float lineHy = CNT_H/2.0f - (pitchDeg - deg) * 0.8f;
            int lx = HW / 2;
            int barW = (abs(deg) % 20 == 0) ? 24 : 14;
            float sa = sinf(rollRad), ca = cosf(rollRad);
            int ax = lx - (int)(barW * ca / 2), ay = (int)(lineHy - barW * sa / 2);
            int bx = lx + (int)(barW * ca / 2), by = (int)(lineHy + barW * sa / 2);
            spr.drawLine(ax, ay, bx, by, C_DGRAY);
            // label
            spr.setTextColor(C_GRAY, 0x0000);
            spr.setTextSize(1);
            spr.setCursor(max(0, ax - 14), ay - 3);
            spr.printf("%d", deg);
        }
    }
};
