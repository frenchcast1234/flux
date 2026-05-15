#pragma once
#include "Screen.h"

class ReactionGame : public Screen {
    M5Canvas spr{&M5.Lcd};

    enum State { WAIT_START, WAITING, FLASH, DONE, TOO_EARLY };
    State    state = WAIT_START;
    uint32_t flashAt  = 0;   // when to flash
    uint32_t flashedAt = 0;  // when flash appeared
    uint32_t reactionMs = 0;
    static constexpr int MAX_HIST = 5;
    uint32_t hist[MAX_HIST] = {};
    int      histCount = 0;
    uint16_t flashColor = C_GREEN;

public:
    void onEnter() override { spr.createSprite(SCR_W, CNT_H); state = WAIT_START; }
    void onExit()  override { spr.deleteSprite(); }

    void onButtonA() override {
        switch (state) {
        case WAIT_START:
            _startRound(); break;
        case WAITING:
            state = TOO_EARLY; onDraw(); break;
        case FLASH:
            reactionMs = millis() - flashedAt;
            _saveResult(reactionMs);
            state = DONE; onDraw(); break;
        case DONE:
        case TOO_EARLY:
            _startRound(); break;
        }
    }
    void onButtonB() override {
        if (state == DONE || state == TOO_EARLY) { histCount = 0; state = WAIT_START; onDraw(); }
    }

    void onLoop() override {
        if (state == WAITING && millis() >= flashAt) {
            flashedAt  = millis();
            flashColor = _randColor();
            state      = FLASH;
            onDraw();
        }
    }

    void onDraw() override {
        spr.fillSprite(C_BG);

        switch (state) {
        case WAIT_START:
            _drawCenter("REACTION TEST", C_WHITE);
            spr.setTextColor(C_CYAN, C_BG);
            spr.setTextSize(1);
            spr.setCursor(70, 75); spr.print("[A] to start");
            spr.setTextColor(C_DGRAY, C_BG);
            spr.setCursor(45, 87); spr.print("[B] clear history");
            break;

        case WAITING:
            _drawCenter("WAIT...", C_DGRAY);
            break;

        case FLASH:
            spr.fillSprite(flashColor);
            spr.setTextColor(C_BG, flashColor);
            spr.setTextSize(3);
            spr.setCursor(68, CNT_H/2 - 16);
            spr.print("PRESS!");
            spr.setTextSize(1);
            spr.setCursor(88, CNT_H/2 + 18);
            spr.print("[A] now!");
            break;

        case TOO_EARLY:
            spr.fillSprite(C_ACCENT);
            spr.setTextColor(C_WHITE, C_ACCENT);
            spr.setTextSize(2);
            spr.setCursor(45, CNT_H/2 - 16);
            spr.print("TOO EARLY!");
            spr.setTextSize(1);
            spr.setCursor(80, CNT_H/2 + 12);
            spr.print("[A] try again");
            break;

        case DONE: {
            // big result
            spr.fillRect(0, 0, SCR_W, 40, C_PANEL);
            spr.setTextColor(C_YELLOW, C_PANEL);
            spr.setTextSize(3);
            char rbuf[12]; snprintf(rbuf, sizeof(rbuf), "%lu ms", reactionMs);
            int tw = strlen(rbuf) * 18;
            spr.setCursor((SCR_W - tw) / 2, 4);
            spr.print(rbuf);

            spr.setTextColor(C_WHITE, C_PANEL);
            spr.setTextSize(1);
            uint16_t rc = reactionMs < 200 ? C_GREEN : (reactionMs < 300 ? C_YELLOW : C_ACCENT);
            const char* grade = reactionMs < 200 ? "EXCELLENT" :
                                (reactionMs < 300 ? "GOOD" : "SLOW");
            spr.setTextColor(rc, C_PANEL);
            spr.setCursor(95, 28); spr.print(grade);

            // history
            spr.drawFastHLine(0, 42, SCR_W, C_ACCENT);
            spr.setTextColor(C_DGRAY, C_BG);
            spr.setTextSize(1);
            spr.setCursor(4, 46); spr.print("History:");

            uint32_t avg = 0;
            for (int i = 0; i < histCount; i++) {
                spr.setTextColor(i == histCount-1 ? C_CYAN : C_DGRAY, C_BG);
                char hb[20]; snprintf(hb, sizeof(hb), "#%d: %lums", i+1, hist[i]);
                spr.setCursor(30, 56 + i*12); spr.print(hb);
                avg += hist[i];
            }
            if (histCount) {
                avg /= histCount;
                spr.setTextColor(C_WHITE, C_BG);
                spr.setCursor(130, CNT_H - 26);
                spr.printf("Avg: %lums", avg);
            }
            spr.setTextColor(C_CYAN, C_BG);
            spr.setCursor(4, CNT_H - 10);
            spr.print("[A] again  [B] clear");
            break;
        }
        }
        spr.pushSprite(0, CNT_Y);
    }

private:
    void _startRound() {
        flashAt = millis() + 1000 + random(4000);
        state   = WAITING;
        onDraw();
    }
    void _drawCenter(const char* msg, uint16_t col) {
        spr.setTextColor(col, C_BG);
        spr.setTextSize(2);
        int tw = strlen(msg) * 12;
        spr.setCursor((SCR_W - tw) / 2, CNT_H/2 - 8);
        spr.print(msg);
    }
    void _saveResult(uint32_t ms) {
        if (histCount < MAX_HIST) hist[histCount++] = ms;
        else { memmove(&hist[0], &hist[1], (MAX_HIST-1)*sizeof(uint32_t)); hist[MAX_HIST-1] = ms; }
    }
    uint16_t _randColor() {
        const uint16_t COLS[] = {C_GREEN, C_CYAN, C_YELLOW, C_ORANGE, C_PURPLE};
        return COLS[random(5)];
    }
};
