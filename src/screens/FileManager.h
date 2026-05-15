#pragma once
#include "Screen.h"
#include <LittleFS.h>
extern int  g_launchApp;
extern char g_openFile[64];

class FileManager : public Screen {
    static constexpr int MAX_FILES = 32;
    static constexpr int VISIBLE   = 7;

    struct FileEntry { char path[64]; uint32_t size; };
    FileEntry _files[MAX_FILES];
    int _count = 0;
    int _sel   = 0;
    int _scroll = 0;

    // maps extension to launch app index
    // txt→TextEditor(25), drw→DrawingApp(26), wav→AudioRecorder(27)
    static constexpr int APP_TXT = 25;
    static constexpr int APP_DRW = 26;
    static constexpr int APP_WAV = 27;

public:
    void onEnter() override { _sel=0; _scroll=0; _scan(); onDraw(); }

    void onButtonA() override {
        if (_count == 0) return;
        _sel = (_sel + 1) % _count;
        if (_sel >= _scroll + VISIBLE) _scroll = _sel - VISIBLE + 1;
        if (_sel < _scroll) _scroll = _sel;
        onDraw();
    }

    void onButtonB() override {
        if (_count == 0) return;
        strncpy(g_openFile, _files[_sel].path, sizeof(g_openFile)-1);
        // route by extension
        const char* ext = strrchr(g_openFile, '.');
        if      (ext && strcmp(ext,".txt")==0) g_launchApp = APP_TXT;
        else if (ext && strcmp(ext,".drw")==0) g_launchApp = APP_DRW;
        else if (ext && strcmp(ext,".wav")==0) g_launchApp = APP_WAV;
    }

    void onDraw() override {
        M5.Lcd.fillRect(0, CNT_Y, SCR_W, CNT_H, C_BG);
        if (_count == 0) {
            M5.Lcd.setTextColor(C_DGRAY, C_BG);
            M5.Lcd.setTextSize(1);
            M5.Lcd.setCursor(60, CNT_Y+50);
            M5.Lcd.print("No files found");
            return;
        }
        int shown = min(VISIBLE, _count - _scroll);
        for (int i = 0; i < shown; i++) {
            int idx = _scroll + i;
            bool sel = (idx == _sel);
            int y = CNT_Y + 2 + i * 16;
            uint16_t bg = sel ? C_PANEL2 : C_BG;
            M5.Lcd.fillRect(0, y, SCR_W, 15, bg);
            if (sel) M5.Lcd.drawRect(0, y, SCR_W-1, 15, C_ACCENT);

            // icon by type
            const char* ext = strrchr(_files[idx].path, '.');
            uint16_t icol = C_GRAY;
            const char* tag = "?";
            if (ext && strcmp(ext,".txt")==0) { icol=C_CYAN;   tag="TXT"; }
            if (ext && strcmp(ext,".drw")==0) { icol=C_PURPLE; tag="IMG"; }
            if (ext && strcmp(ext,".wav")==0) { icol=C_GREEN;  tag="WAV"; }

            M5.Lcd.setTextSize(1);
            M5.Lcd.setTextColor(icol, bg);
            M5.Lcd.setCursor(3, y+4);
            M5.Lcd.print(tag);

            // filename (strip leading /)
            const char* name = strrchr(_files[idx].path, '/');
            name = name ? name+1 : _files[idx].path;
            M5.Lcd.setTextColor(sel ? C_WHITE : C_GRAY, bg);
            M5.Lcd.setCursor(28, y+4);
            // truncate to ~22 chars
            char nbuf[24]; strncpy(nbuf, name, 23); nbuf[23]=0;
            M5.Lcd.print(nbuf);

            // size right-aligned
            char sbuf[12];
            uint32_t s = _files[idx].size;
            if (s < 1024) snprintf(sbuf, sizeof(sbuf), "%uB", s);
            else          snprintf(sbuf, sizeof(sbuf), "%uK", s/1024);
            M5.Lcd.setTextColor(C_DGRAY, bg);
            M5.Lcd.setCursor(200, y+4);
            M5.Lcd.print(sbuf);
        }
        // scrollbar
        if (_count > VISIBLE) {
            int barH = VISIBLE * VISIBLE * 15 / _count;
            int barY = CNT_Y + _scroll * VISIBLE * 15 / _count;
            M5.Lcd.fillRect(SCR_W-3, CNT_Y, 3, CNT_H, C_PANEL);
            M5.Lcd.fillRect(SCR_W-3, barY, 3, barH, C_ACCENT);
        }
    }

private:
    void _scan() {
        _count = 0;
        const char* dirs[] = {"/notes", "/drawings", "/audio"};
        for (auto d : dirs) {
            File dir = LittleFS.open(d);
            if (!dir || !dir.isDirectory()) continue;
            File f;
            while ((f = dir.openNextFile()) && _count < MAX_FILES) {
                snprintf(_files[_count].path, sizeof(_files[0].path),
                         "%s/%s", d, f.name());
                _files[_count].size = f.size();
                _count++;
                f.close();
            }
            dir.close();
        }
    }
};
