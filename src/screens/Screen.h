#pragma once
#include <M5StickCPlus2.h>

// ── palette ────────────────────────────────────────────────────────────────
#define C_BG      0x0022u   // #000011 near-black navy
#define C_PANEL   0x1085u   // #101040 dark panel
#define C_PANEL2  0x2106u   // #200C30 slightly lighter panel
#define C_ACCENT  0xEA6Cu   // #E94560 neon pink
#define C_CYAN    0x07FFu   // #00FFFF cyan
#define C_GREEN   0x07E0u   // #00FF00 green
#define C_DGREEN  0x03E0u   // #007700 dark green
#define C_YELLOW  0xFFE0u   // #FFFF00 yellow
#define C_ORANGE  0xFD20u   // #FF6800 orange
#define C_PURPLE  0x781Au   // #7800D0 purple
#define C_WHITE   0xFFFFu   // white
#define C_GRAY    0x7BEFu   // mid gray
#define C_DGRAY   0x2945u   // dark gray
#define C_BLACK   0x0000u   // black

// ── display geometry ───────────────────────────────────────────────────────
#define SCR_W   240
#define SCR_H   135
#define SB_H    18          // status bar height
#define CNT_Y   SB_H        // content start y
#define CNT_H   (SCR_H - SB_H)   // 117px

class Screen {
public:
    virtual ~Screen() = default;
    virtual void onEnter()   {}
    virtual void onExit()    {}
    virtual void onDraw()    = 0;
    virtual void onButtonA() {}
    virtual void onButtonB() {}
    virtual void onLoop()    {}
};
