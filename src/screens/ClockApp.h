#pragma once
#include "Screen.h"

class ClockApp : public Screen {
    M5Canvas spr{&M5.Lcd};
    int      lastSec = -1;

    static const char* DAYS[];
    static const char* MONTHS[];

public:
    void onEnter() override {
        spr.createSprite(SCR_W, CNT_H);
        lastSec = -1;
    }
    void onExit() override { spr.deleteSprite(); }

    void onLoop() override {
        m5::rtc_time_t t; M5.Rtc.getTime(&t);
        if (t.seconds != lastSec) { lastSec = t.seconds; onDraw(); }
    }

    void onDraw() override {
        spr.fillSprite(C_BG);

        m5::rtc_time_t t; M5.Rtc.getTime(&t);
        m5::rtc_date_t d; M5.Rtc.getDate(&d);

        // ── analog clock face ────────────────────────────────────────────
        int cx = 67, cy = 58;
        int R  = 50;

        spr.drawCircle(cx, cy, R, C_DGRAY);
        spr.drawCircle(cx, cy, R - 1, C_PANEL);

        // tick marks
        for (int m = 0; m < 60; m++) {
            float a = (m * 6.0f - 90.0f) * DEG_TO_RAD;
            int r1 = (m % 5 == 0) ? R - 6 : R - 3;
            uint16_t col = (m % 5 == 0) ? C_GRAY : C_DGRAY;
            spr.drawLine(cx + (R-1)*cos(a), cy + (R-1)*sin(a),
                         cx + r1*cos(a),    cy + r1*sin(a),    col);
        }

        // hour hand
        float ha = ((t.hours % 12) * 30.0f + t.minutes * 0.5f - 90.0f) * DEG_TO_RAD;
        spr.drawLine(cx, cy, cx + 28*cos(ha), cy + 28*sin(ha), C_WHITE);
        spr.drawLine(cx + 1, cy, cx + 1 + 28*cos(ha), cy + 28*sin(ha), C_WHITE);

        // minute hand
        float ma = (t.minutes * 6.0f - 90.0f) * DEG_TO_RAD;
        spr.drawLine(cx, cy, cx + 38*cos(ma), cy + 38*sin(ma), C_CYAN);

        // second hand
        float sa = (t.seconds * 6.0f - 90.0f) * DEG_TO_RAD;
        spr.drawLine(cx, cy, cx + 42*cos(sa), cy + 42*sin(sa), C_ACCENT);
        spr.fillCircle(cx, cy, 3, C_ACCENT);

        // ── digital time ─────────────────────────────────────────────────
        char hm[8]; snprintf(hm, sizeof(hm), "%02d:%02d", t.hours, t.minutes);
        char ss[4];  snprintf(ss, sizeof(ss),  ":%02d", t.seconds);

        spr.setTextColor(C_WHITE, C_BG);
        spr.setTextSize(3);
        spr.setCursor(138, 20);
        spr.print(hm);

        spr.setTextColor(C_ACCENT, C_BG);
        spr.setTextSize(2);
        spr.setCursor(213, 26);
        spr.print(ss);

        // ── date ──────────────────────────────────────────────────────────
        char dateBuf[24];
        snprintf(dateBuf, sizeof(dateBuf), "%s %02d %s %d",
                 DAYS[d.weekDay % 7], d.date,
                 MONTHS[(d.month - 1) % 12], 2000 + d.year);
        spr.setTextColor(C_CYAN, C_BG);
        spr.setTextSize(1);
        spr.setCursor(135, 68);
        spr.print(dateBuf);

        // ── seconds progress bar ─────────────────────────────────────────
        int barW = map(t.seconds, 0, 59, 0, 100);
        spr.fillRect(135, 82, 100, 5, C_PANEL);
        spr.fillRect(135, 82, barW, 5, C_ACCENT);

        // ── battery / temp ────────────────────────────────────────────────
        float temp; M5.Imu.getTemp(&temp);
        char tbuf[16]; snprintf(tbuf, sizeof(tbuf), "%.1f C", temp);
        spr.setTextColor(C_DGRAY, C_BG);
        spr.setTextSize(1);
        spr.setCursor(135, 93);
        spr.print(tbuf);

        int batt = M5.Power.getBatteryLevel();
        char bbuf[8]; snprintf(bbuf, sizeof(bbuf), "BAT %d%%", batt);
        spr.setCursor(135, 105);
        spr.print(bbuf);

        spr.pushSprite(0, CNT_Y);
    }
};

const char* ClockApp::DAYS[]   = {"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};
const char* ClockApp::MONTHS[] = {"Jan","Feb","Mar","Apr","May","Jun",
                                    "Jul","Aug","Sep","Oct","Nov","Dec"};
