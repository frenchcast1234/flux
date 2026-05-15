#pragma once
#include "Screen.h"

class MetronomeApp : public Screen {
    M5Canvas spr{&M5.Lcd};

    int      bpm       = 120;
    bool     running   = false;
    uint32_t lastBeat  = 0;
    uint32_t lastDraw  = 0;
    int      beat      = 0;    // 0 or 1 (left/right)
    bool     accent    = false; // first beat of bar
    int      beatsPerBar = 4;
    int      barCount  = 0;
    float    pendAngle = 0;    // -MAX to +MAX degrees
    float    pendVel   = 0;
    static constexpr float PEND_MAX = 40.0f;
    static constexpr int   PEND_L   = 45;  // pendulum length px

public:
    void onEnter() override { spr.createSprite(SCR_W, CNT_H); running=false; pendAngle=PEND_MAX; }
    void onExit()  override { spr.deleteSprite(); M5.Speaker.stop(); }

    void onButtonA() override { // BPM ++
        bpm = min(bpm + 5, 300);
        _resetPhysics();
        onDraw();
    }
    void onButtonB() override { // start/stop or BPM--
        static uint32_t lastB=0;
        if (millis()-lastB < 250) { bpm=max(bpm-5,20); _resetPhysics(); onDraw(); return; }
        lastB=millis();
        running = !running;
        if (running) lastBeat=millis();
        onDraw();
    }

    void onLoop() override {
        if (!running) {
            // slow pendulum drift even when stopped
            _swingPhysics(60);
            if (millis()-lastDraw>30) { lastDraw=millis(); onDraw(); }
            return;
        }

        uint32_t interval = 60000 / bpm;
        _swingPhysics(bpm);

        if (millis()-lastBeat >= interval) {
            lastBeat = millis();
            beat     = 1 - beat;
            barCount = (barCount+1) % beatsPerBar;
            accent   = (barCount == 0);

            // beep: accent louder/higher
            if (accent) M5.Speaker.tone(1200, 80);
            else        M5.Speaker.tone(800,  50);

            // flip pendulum direction each beat
            pendVel = (beat==0) ? 5.0f : -5.0f;
        }

        if (millis()-lastDraw>20) { lastDraw=millis(); onDraw(); }
    }

    void onDraw() override {
        spr.fillSprite(C_BG);

        // ── BPM display ───────────────────────────────────────────────────
        spr.setTextColor(C_WHITE, C_BG); spr.setTextSize(4);
        char bpmBuf[8]; snprintf(bpmBuf, sizeof(bpmBuf), "%d", bpm);
        int tw = strlen(bpmBuf)*24;
        spr.setCursor((SCR_W-tw)/2 - 20, 6);
        spr.print(bpmBuf);
        spr.setTextSize(1); spr.setTextColor(C_DGRAY, C_BG);
        spr.setCursor((SCR_W+tw)/2 - 16, 18); spr.print("BPM");

        // tempo label
        const char* tempoName =
            bpm<60?"Largo":bpm<80?"Adagio":bpm<100?"Andante":
            bpm<120?"Moderato":bpm<160?"Allegro":bpm<200?"Presto":"Prestissimo";
        spr.setTextColor(C_CYAN, C_BG); spr.setTextSize(1);
        int tl = strlen(tempoName)*6;
        spr.setCursor((SCR_W-tl)/2, 44); spr.print(tempoName);

        // ── pendulum ─────────────────────────────────────────────────────
        int pivotX = SCR_W/2, pivotY = 52;
        float angleRad = pendAngle * DEG_TO_RAD;
        int bobX = pivotX + (int)(PEND_L * sinf(angleRad));
        int bobY = pivotY + (int)(PEND_L * cosf(angleRad));

        spr.drawLine(pivotX, pivotY, bobX, bobY, C_GRAY);
        spr.fillCircle(pivotX, pivotY, 3, C_DGRAY);
        uint16_t bobColor = running ?
            (accent ? C_ACCENT : (beat==0 ? C_CYAN : C_YELLOW)) : C_DGRAY;
        spr.fillCircle(bobX, bobY, 8, bobColor);

        // beat dots bar
        spr.setCursor(4, 106); spr.setTextSize(1); spr.setTextColor(C_DGRAY, C_BG);
        spr.printf("Sig: %d/4  ", beatsPerBar);
        for (int i=0; i<beatsPerBar; i++) {
            uint16_t dc = (running && i==barCount) ? (i==0?C_ACCENT:C_CYAN) : C_PANEL;
            spr.fillCircle(120+i*16, 110, 5, dc);
        }

        // run status
        spr.setTextColor(running ? C_GREEN : C_DGRAY, C_BG);
        spr.setTextSize(1); spr.setCursor(4, 119);
        spr.print(running ? "RUNNING" : "STOPPED");
        spr.setTextColor(C_DGRAY, C_BG);
        spr.setCursor(80, 119);
        spr.print("[A]+5 [B] start/stop  [B][B]-5");

        spr.pushSprite(0, CNT_Y);
    }

private:
    void _swingPhysics(int simulatedBpm) {
        float dt = 0.03f;
        float gravity = (simulatedBpm / 120.0f) * 4.0f;
        pendVel -= gravity * sinf(pendAngle * DEG_TO_RAD) * dt;
        pendVel *= 0.998f;
        pendAngle += pendVel;
        pendAngle = constrain(pendAngle, -PEND_MAX*1.2f, PEND_MAX*1.2f);
    }
    void _resetPhysics() { pendVel = 0; pendAngle = PEND_MAX; }
};
