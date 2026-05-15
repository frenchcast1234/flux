#pragma once
#include "Screen.h"

class PomodoroApp : public Screen {
    static constexpr uint32_t WORK_S  = 25 * 60;
    static constexpr uint32_t BREAK_S = 5  * 60;
    static constexpr uint32_t LBREAK_S= 15 * 60;

    enum Phase { WORK, BREAK, LONG_BREAK };
    Phase    _phase   = WORK;
    int      _pomo    = 0;   // completed pomodoros
    uint32_t _remain  = WORK_S;
    uint32_t _lastTick = 0;
    bool     _running = false;

public:
    void onEnter() override {
        _phase=WORK; _pomo=0; _remain=WORK_S; _running=false; _lastTick=millis();
        onDraw();
    }

    void onButtonA() override {
        _running = !_running;
        if (_running) _lastTick = millis();
        onDraw();
    }

    void onButtonB() override {
        // skip to next phase
        _advance();
        onDraw();
    }

    void onLoop() override {
        if (!_running) return;
        uint32_t now = millis();
        if (now - _lastTick >= 1000) {
            _lastTick += 1000;
            if (_remain > 0) _remain--;
            if (_remain == 0) {
                M5.Speaker.tone(880, 300); delay(350);
                M5.Speaker.tone(660, 300); delay(350);
                M5.Speaker.tone(880, 300);
                _advance();
            }
            _drawTimer();
        }
    }

    void onDraw() override {
        M5.Lcd.fillRect(0, CNT_Y, SCR_W, CNT_H, C_BG);
        _drawPhaseLabel();
        _drawTimer();
        _drawPomos();
        _drawButtons();
    }

private:
    void _advance() {
        if (_phase == WORK) {
            _pomo++;
            if (_pomo % 4 == 0) { _phase=LONG_BREAK; _remain=LBREAK_S; }
            else                 { _phase=BREAK;      _remain=BREAK_S;  }
        } else {
            _phase=WORK; _remain=WORK_S;
        }
        _running = false;
    }

    void _drawPhaseLabel() {
        const char* labels[] = {"WORK", "BREAK", "LONG BREAK"};
        uint16_t cols[] = {C_ACCENT, C_GREEN, C_CYAN};
        M5.Lcd.setTextSize(2);
        M5.Lcd.setTextColor(cols[_phase], C_BG);
        int lx = 120 - strlen(labels[_phase])*6;
        M5.Lcd.setCursor(lx, CNT_Y+5);
        M5.Lcd.print(labels[_phase]);
    }

    void _drawTimer() {
        // large countdown circle
        int cx=120, cy=CNT_Y+68, r=36;
        M5.Lcd.fillCircle(cx, cy, r, C_PANEL);
        M5.Lcd.drawCircle(cx, cy, r, C_DGRAY);

        uint32_t total = (_phase==WORK)?WORK_S:(_phase==BREAK)?BREAK_S:LBREAK_S;
        float prog = 1.0f - (float)_remain/total;
        uint16_t arc_col = (_phase==WORK)?C_ACCENT:(_phase==BREAK)?C_GREEN:C_CYAN;
        // draw arc segments (360 steps)
        int segs = (int)(prog * 360);
        for (int a=0; a<segs; a++) {
            float rad = (a - 90) * DEG_TO_RAD;
            int x1=(int)(cx+r*cosf(rad)); int y1=(int)(cy+r*sinf(rad));
            int x2=(int)(cx+(r-3)*cosf(rad)); int y2=(int)(cy+(r-3)*sinf(rad));
            M5.Lcd.drawLine(x1,y1,x2,y2,arc_col);
        }

        // time text
        char buf[8];
        snprintf(buf, sizeof(buf), "%02lu:%02lu", _remain/60, _remain%60);
        M5.Lcd.setTextSize(2);
        M5.Lcd.setTextColor(C_WHITE, C_PANEL);
        M5.Lcd.setCursor(cx-24, cy-7);
        M5.Lcd.print(buf);
    }

    void _drawPomos() {
        M5.Lcd.setTextSize(1);
        M5.Lcd.setTextColor(C_DGRAY, C_BG);
        M5.Lcd.setCursor(4, CNT_Y+8);
        M5.Lcd.printf("#%d", _pomo+1);
        // tomato icons
        for (int i=0; i<min(_pomo,8); i++) {
            int x = 4 + i*14;
            M5.Lcd.fillCircle(x, CNT_Y+CNT_H-22, 5, C_ACCENT);
            M5.Lcd.fillRect(x-1, CNT_Y+CNT_H-28, 2, 4, C_GREEN);
        }
    }

    void _drawButtons() {
        M5.Lcd.setTextSize(1);
        M5.Lcd.setTextColor(C_DGRAY, C_BG);
        M5.Lcd.setCursor(4, CNT_Y+CNT_H-10);
        M5.Lcd.print(_running ? "[A]Pause [B]Skip" : "[A]Start [B]Skip");
    }
};
