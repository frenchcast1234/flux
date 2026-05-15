#pragma once
#include "Screen.h"
#include <IRremoteESP8266.h>
#include <IRsend.h>

// IR LED pin for M5StickC Plus2 — adjust if needed (try 9 if 19 doesn't work)
static constexpr uint16_t IR_LED_PIN = 19;

class IRApp : public Screen {
    M5Canvas spr{&M5.Lcd};
    IRsend   ir{IR_LED_PIN};
    int      sel   = 0;
    bool     sent  = false;
    uint32_t sentAt = 0;

    struct IRCmd {
        const char* label;
        uint32_t    code;
        const char* icon;
    };

    // Samsung NEC 32-bit codes
    static constexpr IRCmd CMDS[] = {
        {"POWER",  0xE0E040BF, "PWR"},
        {"VOL +",  0xE0E0E01F, "V+"},
        {"VOL -",  0xE0E0D02F, "V-"},
        {"MUTE",   0xE0E0F00F, "M"},
        {"CH +",   0xE0E048B7, "C+"},
        {"CH -",   0xE0E008F7, "C-"},
        {"MENU",   0xE0E058A7, "MN"},
        {"OK",     0xE0E016E9, "OK"},
        {"BACK",   0xE0E01AE5, "BK"},
        {"1",      0xE0E020DF, "1"},
        {"2",      0xE0E0A05F, "2"},
        {"3",      0xE0E0609F, "3"},
    };
    static constexpr int NCMDS = 12;
    static constexpr int COLS  = 4;
    static constexpr int ROWS  = 3;

public:
    void onEnter() override {
        spr.createSprite(SCR_W, CNT_H);
        ir.begin();
        sel  = 0;
        sent = false;
    }
    void onExit() override { spr.deleteSprite(); }

    void onButtonA() override { sel = (sel + 1) % NCMDS; onDraw(); }
    void onButtonB() override { _send(); }

    void onLoop() override {
        if (sent && millis() - sentAt > 400) { sent = false; onDraw(); }
    }

    void onDraw() override {
        spr.fillSprite(C_BG);

        int cellW = SCR_W / COLS;   // 60
        int cellH = CNT_H / ROWS;   // ~39

        for (int i = 0; i < NCMDS; i++) {
            int col = i % COLS, row = i / COLS;
            int cx = col * cellW, cy = row * cellH;

            bool   isSel = (i == sel);
            bool   isSent = (sent && i == sel);
            uint16_t bg = isSent ? C_ACCENT : (isSel ? C_PANEL2 : C_PANEL);

            spr.fillRoundRect(cx + 2, cy + 2, cellW - 4, cellH - 4, 5, bg);
            if (isSel)
                spr.drawRoundRect(cx + 2, cy + 2, cellW - 4, cellH - 4, 5, C_ACCENT);

            // icon label
            spr.setTextSize(2);
            spr.setTextColor(isSent ? C_WHITE : (isSel ? C_CYAN : C_GRAY), bg);
            int tw = strlen(CMDS[i].icon) * 12;
            spr.setCursor(cx + cellW/2 - tw/2, cy + 5);
            spr.print(CMDS[i].icon);

            // name
            spr.setTextSize(1);
            spr.setTextColor(isSent ? C_WHITE : (isSel ? C_WHITE : C_DGRAY), bg);
            int lw = strlen(CMDS[i].label) * 6;
            spr.setCursor(cx + cellW/2 - lw/2, cy + cellH - 12);
            spr.print(CMDS[i].label);
        }

        // bottom bar
        spr.fillRect(0, CNT_H - 12, SCR_W, 12, C_PANEL);
        spr.setTextSize(1);
        spr.setTextColor(C_DGRAY, C_PANEL);
        spr.setCursor(4, CNT_H - 10);
        spr.print("[A] nav  [B] send  Samsung NEC");

        spr.pushSprite(0, CNT_Y);
    }

private:
    void _send() {
        ir.sendSAMSUNG(CMDS[sel].code, 32);
        sent  = true;
        sentAt = millis();
        onDraw();
    }
};
constexpr IRApp::IRCmd IRApp::CMDS[IRApp::NCMDS];
