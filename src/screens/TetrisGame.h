#pragma once
#include "Screen.h"

// ── Tetris with tilt-to-move (gyro left/right) ────────────────────────────
class TetrisGame : public Screen {
    M5Canvas spr{&M5.Lcd};

    // grid geometry (in content-area coords)
    static constexpr int GX = 2, GY = 8;   // grid top-left in content area
    static constexpr int CS = 5;            // cell size px
    static constexpr int GCOLS = 10, GROWS = 20;

    // piece row masks  [piece][rotation][row]  (4-bit, bit3=leftmost col)
    static constexpr uint8_t SHAPES[7][4][4] = {
        // I
        {{0,0xF,0,0},{0x2,0x2,0x2,0x2},{0,0,0xF,0},{0x4,0x4,0x4,0x4}},
        // O
        {{0x6,0x6,0,0},{0x6,0x6,0,0},{0x6,0x6,0,0},{0x6,0x6,0,0}},
        // T
        {{0x4,0xE,0,0},{0x4,0xC,0x4,0},{0xE,0x4,0,0},{0x4,0x6,0x4,0}},
        // S
        {{0x6,0xC,0,0},{0x4,0x6,0x2,0},{0x6,0xC,0,0},{0x4,0x6,0x2,0}},
        // Z
        {{0xC,0x6,0,0},{0x2,0x6,0x4,0},{0xC,0x6,0,0},{0x2,0x6,0x4,0}},
        // J
        {{0x8,0xE,0,0},{0x6,0x4,0x4,0},{0xE,0x2,0,0},{0x4,0x4,0xC,0}},
        // L
        {{0x2,0xE,0,0},{0x4,0x4,0x6,0},{0xE,0x8,0,0},{0xC,0x4,0x4,0}},
    };
    static constexpr uint16_t PCOLORS[7] = {
        C_CYAN, C_YELLOW, C_PURPLE, C_GREEN, C_ACCENT, 0x001Fu, C_ORANGE
    };

    uint8_t  grid[GROWS][GCOLS] = {};   // 0=empty, 1-7=piece
    int8_t   px=3, py=0, ptype=0, prot=0; // current piece
    int8_t   nextType = 0;
    uint32_t score=0, hiscore=0;
    uint16_t level=1, lines=0;
    bool     over=false, paused=false;
    uint32_t lastStep=0, lastMove=0;
    uint32_t stepMs = 500;

public:
    void onEnter() override {
        spr.createSprite(SCR_W, CNT_H);
        _reset();
    }
    void onExit() override { spr.deleteSprite(); }

    void onButtonA() override { // rotate CW
        if (over) { _reset(); return; }
        if (paused) { paused = false; return; }
        int nr = (prot + 1) % 4;
        if (_canPlace(px, py, nr)) prot = nr;
        onDraw();
    }
    void onButtonB() override { // hard drop
        if (over) { _reset(); return; }
        while (_canPlace(px, py+1, prot)) py++;
        _lock(); onDraw();
    }

    void onLoop() override {
        if (over || paused) return;

        // tilt-based horizontal movement
        float ax, ay, az;
        M5.Imu.getAccelData(&ax, &ay, &az);
        if (millis() - lastMove > 150) {
            if (ay < -0.35f && _canPlace(px-1, py, prot)) { px--; lastMove = millis(); onDraw(); }
            if (ay >  0.35f && _canPlace(px+1, py, prot)) { px++; lastMove = millis(); onDraw(); }
        }

        // gravity step
        uint32_t ms = stepMs / level;
        if (ms < 80) ms = 80;
        if (millis() - lastStep > ms) {
            lastStep = millis();
            if (_canPlace(px, py+1, prot)) { py++; onDraw(); }
            else { _lock(); onDraw(); }
        }
    }

    void onDraw() override {
        spr.fillSprite(C_BG);

        // border
        spr.drawRect(GX-1, GY-1, GCOLS*CS+2, GROWS*CS+2, C_PANEL);

        // grid
        for (int r=0; r<GROWS; r++) for (int c=0; c<GCOLS; c++) {
            if (grid[r][c]) {
                int x = GX + c*CS, y = GY + r*CS;
                spr.fillRect(x, y, CS-1, CS-1, PCOLORS[grid[r][c]-1]);
            }
        }

        // current piece
        if (!over) _drawPiece(px, py, ptype, prot, PCOLORS[ptype]);

        // ghost piece
        {
            int gy = py;
            while (_canPlace(px, gy+1, prot)) gy++;
            if (gy != py) _drawPiece(px, gy, ptype, prot, C_DGRAY);
        }

        // ── info panel ────────────────────────────────────────────────────
        int ix = GX + GCOLS*CS + 6;
        spr.setTextSize(1); spr.setTextColor(C_DGRAY, C_BG);
        spr.setCursor(ix, 2); spr.print("SCORE");
        spr.setTextSize(2); spr.setTextColor(C_WHITE, C_BG);
        spr.setCursor(ix, 10); spr.printf("%lu", score);

        spr.setTextSize(1); spr.setTextColor(C_DGRAY, C_BG);
        spr.setCursor(ix, 28); spr.print("BEST");
        spr.setTextColor(C_CYAN, C_BG);
        spr.setCursor(ix, 36); spr.printf("%lu", hiscore);

        spr.setTextColor(C_DGRAY, C_BG);
        spr.setCursor(ix, 52); spr.print("LVL");
        spr.setTextColor(C_YELLOW, C_BG); spr.setTextSize(2);
        spr.setCursor(ix, 60); spr.printf("%d", level);

        spr.setTextSize(1); spr.setTextColor(C_DGRAY, C_BG);
        spr.setCursor(ix, 76); spr.print("LINES");
        spr.setTextColor(C_GREEN, C_BG);
        spr.setCursor(ix, 84); spr.printf("%d", lines);

        // next piece preview
        spr.setTextSize(1); spr.setTextColor(C_DGRAY, C_BG);
        spr.setCursor(ix, 96); spr.print("NEXT");
        _drawPiece(ix/CS, 21, nextType, 0, PCOLORS[nextType]);

        // controls hint
        spr.setTextColor(C_DGRAY, C_BG); spr.setTextSize(1);
        spr.setCursor(ix, 108); spr.print("[A]rot");
        spr.setCursor(ix, 116); spr.print("[B]drop");

        if (over) {
            spr.fillRect(10, 40, 130, 46, C_PANEL);
            spr.drawRect(10, 40, 130, 46, C_ACCENT);
            spr.setTextColor(C_ACCENT, C_PANEL); spr.setTextSize(2);
            spr.setCursor(16, 46); spr.print("GAME OVER");
            spr.setTextSize(1); spr.setTextColor(C_CYAN, C_PANEL);
            spr.setCursor(18, 68); spr.print("A/B to restart");
        }

        spr.pushSprite(0, CNT_Y);
    }

private:
    bool _cellSet(int type, int rot, int row, int col) {
        if (row<0||row>3||col<0||col>3) return false;
        return (SHAPES[type][rot][row] >> (3-col)) & 1;
    }

    bool _canPlace(int bx, int by, int br) {
        for (int r=0; r<4; r++) for (int c=0; c<4; c++) {
            if (!_cellSet(ptype, br, r, c)) continue;
            int gx=bx+c, gy=by+r;
            if (gx<0||gx>=GCOLS||gy>=GROWS) return false;
            if (gy>=0 && grid[gy][gx]) return false;
        }
        return true;
    }

    void _drawPiece(int bx, int by, int type, int rot, uint16_t col) {
        for (int r=0; r<4; r++) for (int c=0; c<4; c++) {
            if (!_cellSet(type, rot, r, c)) continue;
            int gx=bx+c, gy=by+r;
            if (gx<0||gx>=GCOLS||gy<0||gy>=GROWS) continue;
            int px2 = GX + gx*CS, py2 = GY + gy*CS;
            spr.fillRect(px2, py2, CS-1, CS-1, col);
        }
    }

    void _lock() {
        for (int r=0; r<4; r++) for (int c=0; c<4; c++) {
            if (!_cellSet(ptype, prot, r, c)) continue;
            int gy=py+r, gx=px+c;
            if (gy>=0 && gy<GROWS && gx>=0 && gx<GCOLS)
                grid[gy][gx] = ptype + 1;
        }
        int cleared = _clearLines();
        static const int SC[] = {0,100,300,500,800};
        score += SC[min(cleared,4)] * level;
        lines += cleared;
        level  = lines/10 + 1;
        if (score > hiscore) hiscore = score;
        _spawn();
    }

    int _clearLines() {
        int cnt = 0;
        for (int r=GROWS-1; r>=0; r--) {
            bool full = true;
            for (int c=0; c<GCOLS; c++) if (!grid[r][c]) { full=false; break; }
            if (full) {
                memmove(&grid[1], &grid[0], r * sizeof(grid[0]));
                memset(&grid[0], 0, sizeof(grid[0]));
                cnt++; r++;
            }
        }
        return cnt;
    }

    void _spawn() {
        ptype = nextType;
        nextType = random(7);
        px = GCOLS/2 - 2; py = 0; prot = 0;
        if (!_canPlace(px, py, prot)) over = true;
    }

    void _reset() {
        memset(grid, 0, sizeof(grid));
        score=0; lines=0; level=1; over=false; paused=false;
        nextType = random(7);
        _spawn();
        lastStep = millis();
    }
};
constexpr uint8_t  TetrisGame::SHAPES[7][4][4];
constexpr uint16_t TetrisGame::PCOLORS[7];
