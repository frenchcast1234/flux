#pragma once
#include "Screen.h"
#include <cmath>

class AudioMeter : public Screen {
    M5Canvas    spr{&M5.Lcd};
    static constexpr int BUF = 256;
    static constexpr int WAVE_W = 240;

    int16_t  micBuf[BUF]  = {};
    uint8_t  waveBuf[WAVE_W] = {}; // compressed waveform history
    int      wavePos = 0;
    float    peak    = 0;
    float    rmsSmooth = 0;
    uint32_t lastRead  = 0;

public:
    void onEnter() override {
        spr.createSprite(SCR_W, CNT_H);
        M5.Mic.begin();
        peak = 0; rmsSmooth = 0; wavePos = 0;
    }
    void onExit() override {
        M5.Mic.end();
        spr.deleteSprite();
    }

    void onLoop() override {
        if (millis() - lastRead < 30) return;
        lastRead = millis();

        // record samples
        if (M5.Mic.record(micBuf, BUF, 8000)) {
            // compute RMS
            int64_t sum = 0;
            for (int i = 0; i < BUF; i++) sum += (int64_t)micBuf[i] * micBuf[i];
            float rms = sqrtf((float)sum / BUF);

            // smooth
            rmsSmooth = rmsSmooth * 0.7f + rms * 0.3f;
            if (rmsSmooth > peak) peak = rmsSmooth;
            else                   peak *= 0.995f; // decay peak

            // waveform: store mid-sample compressed to 0-63
            int midSample = abs(micBuf[BUF / 2]);
            waveBuf[wavePos] = (uint8_t)constrain(midSample / 200, 0, 55);
            wavePos = (wavePos + 1) % WAVE_W;

            onDraw();
        }
    }

    void onDraw() override {
        spr.fillSprite(C_BG);

        // ── waveform (top half) ───────────────────────────────────────────
        int waveH = 55;
        int waveY = 2;
        spr.drawRect(0, waveY, SCR_W, waveH, C_PANEL);

        for (int x = 0; x < WAVE_W; x++) {
            int idx  = (wavePos + x) % WAVE_W;
            int h    = waveBuf[idx];
            int yTop = waveY + waveH/2 - h/2;
            uint16_t col = (x == WAVE_W - 1) ? C_ACCENT : C_CYAN;
            spr.drawFastVLine(x, yTop, h, col);
        }

        // centre zero line
        spr.drawFastHLine(0, waveY + waveH/2, WAVE_W, C_PANEL);

        // ── level meter (bottom half) ─────────────────────────────────────
        int meterY  = waveY + waveH + 4;
        int meterH  = CNT_H - waveY - waveH - 8;
        int meterW  = SCR_W - 4;

        // bar background
        spr.fillRect(2, meterY, meterW, meterH, C_PANEL);

        // coloured fill: green → yellow → red
        float norm = constrain(rmsSmooth / 4000.0f, 0.0f, 1.0f);
        int   fill = (int)(norm * meterW);
        int   g  = min(fill, (int)(meterW * 0.6f));
        int   y2 = max(0, min(fill - g, (int)(meterW * 0.2f)));
        int   r  = max(0, fill - g - y2);
        if (g  > 0) spr.fillRect(2,     meterY + 2, g,  meterH - 4, C_GREEN);
        if (y2 > 0) spr.fillRect(2+g,   meterY + 2, y2, meterH - 4, C_YELLOW);
        if (r  > 0) spr.fillRect(2+g+y2,meterY + 2, r,  meterH - 4, C_ACCENT);

        // peak marker
        int peakX = 2 + constrain((int)(peak / 4000.0f * meterW), 0, meterW - 2);
        spr.drawFastVLine(peakX, meterY, meterH, C_WHITE);

        // dB scale labels
        spr.setTextSize(1);
        spr.setTextColor(C_DGRAY, C_BG);
        const int dbMarks[] = {-40, -20, -10, -6, -3, 0};
        for (int db : dbMarks) {
            float lin = powf(10.0f, db / 20.0f);
            int  lx   = 2 + (int)(lin * meterW);
            spr.drawFastVLine(lx, meterY + meterH, 4, C_GRAY);
            spr.setCursor(lx - 6, meterY + meterH + 5);
            spr.printf("%d", db);
        }

        // level text
        float dbVal = (rmsSmooth > 1.0f) ? 20.0f * log10f(rmsSmooth / 32767.0f) : -99.0f;
        spr.setTextColor(C_WHITE, C_BG);
        spr.setTextSize(1);
        spr.setCursor(3, meterY - 10);
        spr.printf("%.1f dBFS", dbVal);

        spr.pushSprite(0, CNT_Y);
    }
};
