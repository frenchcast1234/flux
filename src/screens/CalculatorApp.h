#pragma once
#include "Screen.h"

// Scroll-wheel calculator: BtnA = cycle value, BtnB = confirm + advance
class CalculatorApp : public Screen {
    M5Canvas spr{&M5.Lcd};

    enum Field { F_N1D1, F_N1D2, F_N1D3, F_OP, F_N2D1, F_N2D2, F_N2D3, F_EQ, F_RESULT };
    Field  field = F_N1D1;
    int8_t digits[7] = {}; // N1D1..D3, OP, N2D1..D3  (op: 0=+ 1=- 2=× 3=÷)
    float  result    = 0;
    bool   hasResult = false;
    bool   divByZero = false;

    static constexpr int WHEEL_ITEMS = 11; // 0-9 + C(clear)

public:
    void onEnter() override { spr.createSprite(SCR_W, CNT_H); _clear(); }
    void onExit()  override { spr.deleteSprite(); }

    void onButtonA() override { // scroll up
        if (field == F_RESULT) { _clear(); return; }
        if (field == F_OP) {
            digits[3] = (digits[3] + 1) % 4;
        } else {
            int i = _fieldIdx();
            digits[i] = (digits[i] + 1) % WHEEL_ITEMS;
        }
        onDraw();
    }

    void onButtonB() override { // confirm
        if (field == F_RESULT) { _useResult(); return; }
        if (field == F_EQ) { _compute(); return; }

        // if current digit is 10 = C (clear this number)
        int i = _fieldIdx();
        if (i < 3 && digits[i] == 10) { digits[0]=digits[1]=digits[2]=0; field=F_N1D1; onDraw(); return; }
        if (i > 3 && digits[i] == 10) { digits[4]=digits[5]=digits[6]=0; field=F_N2D1; onDraw(); return; }

        field = (Field)((int)field + 1);
        onDraw();
    }

    void onDraw() override {
        spr.fillSprite(C_BG);

        // ── expression display ────────────────────────────────────────────
        char n1[5], n2[5], op[3], res[12];
        _buildNum(0, n1); _buildNum(4, n2);
        const char* OPS[] = {"+", "-", "x", "/"};
        strncpy(op, OPS[digits[3]], sizeof(op));

        spr.fillRect(0, 0, SCR_W, 18, C_PANEL);
        spr.setTextColor(C_WHITE, C_PANEL); spr.setTextSize(2);
        if (hasResult) {
            snprintf(res, sizeof(res), divByZero ? "ERR" : "%.4g", result);
            char expr[32]; snprintf(expr, sizeof(expr), "%s%s%s=%s", n1, op, n2, res);
            spr.setCursor(4, 1); spr.print(expr);
        } else {
            char expr[20]; snprintf(expr, sizeof(expr), "%s%s%s", n1, op, n2);
            spr.setCursor(4, 1); spr.print(expr);
        }

        // ── current field indicator ───────────────────────────────────────
        spr.setTextSize(1); spr.setTextColor(C_CYAN, C_BG);
        spr.setCursor(4, 24);
        const char* FN[] = {"digit","digit","digit","op","digit","digit","digit","[=]","result"};
        spr.printf("Editing: %s", FN[(int)field]);

        // ── large wheel display ────────────────────────────────────────────
        int cy = 70;
        if (field == F_RESULT) {
            spr.setTextSize(3); spr.setTextColor(C_GREEN, C_BG);
            if (divByZero) { spr.setCursor(60,cy-16); spr.print("ERROR"); }
            else { char rb[16]; snprintf(rb,sizeof(rb),"%.6g",result);
                   int tw=strlen(rb)*18; spr.setCursor((SCR_W-tw)/2,cy-16); spr.print(rb); }
            spr.setTextSize(1); spr.setTextColor(C_CYAN, C_BG);
            spr.setCursor(50, cy+20); spr.print("[A] new calc  [B] use result");
        } else if (field == F_EQ) {
            spr.setTextSize(3); spr.setTextColor(C_YELLOW, C_BG);
            spr.setCursor(100, cy-16); spr.print("=");
            spr.setTextSize(1); spr.setTextColor(C_CYAN, C_BG);
            spr.setCursor(70, cy+20); spr.print("[B] to compute");
        } else if (field == F_OP) {
            // show 4 ops, highlight current
            for (int i=0; i<4; i++) {
                bool sel = (i == digits[3]);
                uint16_t bg = sel ? C_PANEL2 : C_BG;
                spr.fillRect(30+i*48, cy-18, 40, 30, bg);
                if (sel) spr.drawRect(30+i*48, cy-18, 40, 30, C_ACCENT);
                spr.setTextColor(sel ? C_WHITE : C_DGRAY, bg);
                spr.setTextSize(3);
                spr.setCursor(44+i*48, cy-14); spr.print(OPS[i]);
            }
            spr.setTextSize(1); spr.setTextColor(C_DGRAY, C_BG);
            spr.setCursor(70, cy+20); spr.print("[A] cycle  [B] confirm");
        } else {
            // digit wheel: show -2 to +2
            int cur = digits[_fieldIdx()];
            for (int offset=-2; offset<=2; offset++) {
                int val = ((cur + offset) % WHEEL_ITEMS + WHEEL_ITEMS) % WHEEL_ITEMS;
                bool sel = (offset == 0);
                int wy = cy + offset*22;
                uint16_t fg = sel ? C_WHITE : C_DGRAY;
                int size = sel ? 3 : (abs(offset)==1 ? 2 : 1);
                if (sel) {
                    spr.fillRect(90, wy-16, 60, 28, C_PANEL2);
                    spr.drawRect(90, wy-16, 60, 28, C_ACCENT);
                }
                spr.setTextSize(size);
                spr.setTextColor(fg, sel ? C_PANEL2 : C_BG);
                if (val == 10) {
                    spr.setCursor(sel?110:112, wy-(size*4));
                    spr.print("CLR");
                } else {
                    spr.setCursor(sel?115:113, wy-(size*4));
                    spr.printf("%d", val);
                }
            }
            spr.setTextSize(1); spr.setTextColor(C_DGRAY, C_BG);
            spr.setCursor(70, cy+56); spr.print("[A] scroll  [B] confirm");
        }

        spr.pushSprite(0, CNT_Y);
    }

private:
    int _fieldIdx() {
        // digits array: [0..2]=N1 digits, [3]=op, [4..6]=N2 digits
        if (field <= F_N1D3) return (int)field;
        if (field == F_OP)   return 3;
        if (field <= F_N2D3) return (int)field - 1; // F_N2D1=5→4, etc.
        return 0;
    }

    void _buildNum(int start, char* out) {
        // skip leading zeros, show at least 1 digit
        int d0=digits[start]%10, d1=digits[start+1]%10, d2=digits[start+2]%10;
        if (d0>0) snprintf(out, 5, "%d%d%d", d0, d1, d2);
        else if (d1>0) snprintf(out, 5, "%d%d", d1, d2);
        else snprintf(out, 5, "%d", d2);
    }

    float _getNum(int start) {
        return digits[start]*100.0f + digits[start+1]*10.0f + digits[start+2];
    }

    void _compute() {
        float a = _getNum(0), b = _getNum(4);
        divByZero = false;
        switch (digits[3]) {
        case 0: result = a + b; break;
        case 1: result = a - b; break;
        case 2: result = a * b; break;
        case 3:
            if (fabsf(b) < 0.001f) divByZero = true;
            else result = a / b;
            break;
        }
        hasResult = true;
        field = F_RESULT;
        onDraw();
    }

    void _useResult() {
        // put result into N1
        int r = (int)constrain(result, 0, 999);
        digits[0] = r/100; digits[1]=(r/10)%10; digits[2]=r%10;
        memset(&digits[3], 0, 4);
        hasResult=false; field=F_OP; onDraw();
    }

    void _clear() {
        memset(digits, 0, sizeof(digits));
        result=0; hasResult=false; divByZero=false; field=F_N1D1;
    }
};
