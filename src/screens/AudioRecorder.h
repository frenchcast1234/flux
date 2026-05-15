#pragma once
#include "Screen.h"
#include <LittleFS.h>
extern int  g_launchApp;
extern char g_openFile[64];

class AudioRecorder : public Screen {
    static constexpr int SAMPLE_RATE = 8000;
    static constexpr int REC_SECS    = 10;
    static constexpr int BUF_SAMPLES = 256;
    static constexpr int MAX_SAMPLES = SAMPLE_RATE * REC_SECS; // 80000

    enum State { IDLE, RECORDING, PLAYING };
    State    _state    = IDLE;
    int16_t* _buf      = nullptr; // heap alloc only during record/play
    int      _recLen   = 0;
    int      _playPos  = 0;
    char     _filePath[64] = {};
    bool     _hasFile  = false;

    uint32_t _lastDraw = 0;
    static constexpr int WF_W = 200; // waveform width

public:
    void onEnter() override {
        _state = IDLE; _recLen = 0; _playPos = 0; _hasFile = false;
        if (g_openFile[0]) {
            strncpy(_filePath, g_openFile, sizeof(_filePath)-1);
            _hasFile = true;
        }
        onDraw();
    }

    void onExit() override {
        _stopAll();
        free(_buf); _buf = nullptr;
        g_openFile[0] = 0;
    }

    void onButtonA() override {
        if (_state == RECORDING) { _stopRecording(); return; }
        if (_state == PLAYING)   { _stopAll(); onDraw(); return; }
        // IDLE → start recording
        _startRecording();
    }

    void onButtonB() override {
        if (_state != IDLE) { _stopAll(); onDraw(); return; }
        // play most recently recorded / loaded file
        if (_hasFile || _recLen > 0) _startPlaying();
    }

    void onLoop() override {
        if (_state == RECORDING) _tickRecord();
        if (_state == PLAYING)   _tickPlay();
        uint32_t now = millis();
        if (now - _lastDraw > 80) { _lastDraw = now; _drawWaveform(); }
    }

    void onDraw() override {
        M5.Lcd.fillRect(0, CNT_Y, SCR_W, CNT_H, C_BG);
        _drawUI();
        _drawWaveform();
    }

private:
    void _drawUI() {
        M5.Lcd.setTextSize(1);
        // title
        M5.Lcd.setTextColor(C_ACCENT, C_BG);
        M5.Lcd.setCursor(4, CNT_Y+4);
        M5.Lcd.print("AUDIO RECORDER");

        // status
        const char* stStr = (_state==IDLE ? "IDLE" : _state==RECORDING ? "REC" : "PLAY");
        uint16_t sc = (_state==IDLE ? C_GRAY : _state==RECORDING ? C_ACCENT : C_GREEN);
        M5.Lcd.setTextColor(sc, C_BG);
        M5.Lcd.setCursor(SCR_W-36, CNT_Y+4);
        M5.Lcd.print(stStr);

        // button hints
        M5.Lcd.setTextColor(C_DGRAY, C_BG);
        M5.Lcd.setCursor(4, CNT_Y+CNT_H-12);
        if (_state==IDLE)      M5.Lcd.print("[A]Rec  [B]Play");
        else if (_state==RECORDING) M5.Lcd.print("[A]Stop recording");
        else                   M5.Lcd.print("[A/B]Stop");

        // progress bar
        if (_state == RECORDING || _state == PLAYING) {
            int total = (_state==RECORDING) ? MAX_SAMPLES : _recLen;
            int pos   = (_state==RECORDING) ? _recLen     : _playPos;
            int px = (total > 0) ? (int)((long)pos * 220 / total) : 0;
            M5.Lcd.drawRect(10, CNT_Y+100, 220, 8, C_PANEL);
            uint16_t bc = (_state==RECORDING) ? C_ACCENT : C_GREEN;
            M5.Lcd.fillRect(11, CNT_Y+101, px, 6, bc);
        }

        // file name if loaded
        if (_hasFile && _state==IDLE) {
            const char* n = strrchr(_filePath, '/');
            M5.Lcd.setTextColor(C_CYAN, C_BG);
            M5.Lcd.setCursor(4, CNT_Y+15);
            M5.Lcd.print(n ? n+1 : _filePath);
        }
    }

    void _drawWaveform() {
        int y0 = CNT_Y + 60;
        M5.Lcd.fillRect(20, CNT_Y+30, WF_W, 60, C_BG);
        M5.Lcd.drawRect(20, CNT_Y+30, WF_W, 60, C_PANEL);

        if (_state == RECORDING && _buf && _recLen > 0) {
            int step = max(1, _recLen / WF_W);
            for (int i = 0; i < WF_W && (i*step) < _recLen; i++) {
                int16_t v = _buf[i * step];
                int h = abs(v) * 28 / 32768;
                uint16_t col = (abs(v) > 20000) ? C_ACCENT : C_GREEN;
                M5.Lcd.drawFastVLine(20+i, y0-h, h*2+1, col);
            }
        } else if (_state == PLAYING && _buf && _recLen > 0) {
            int step = max(1, _recLen / WF_W);
            for (int i = 0; i < WF_W && (i*step) < _recLen; i++) {
                int16_t v = _buf[i * step];
                int h = abs(v) * 28 / 32768;
                bool played = (i * step) < _playPos;
                uint16_t col = played ? C_CYAN : C_DGRAY;
                M5.Lcd.drawFastVLine(20+i, y0-h, h*2+1, col);
            }
        } else {
            // flat line
            M5.Lcd.drawFastHLine(20, y0, WF_W, C_DGRAY);
        }
    }

    void _startRecording() {
        free(_buf);
        _buf = (int16_t*)malloc(MAX_SAMPLES * 2);
        if (!_buf) return;
        _recLen = 0;
        M5.Mic.begin();
        _state = RECORDING;
        onDraw();
    }

    void _tickRecord() {
        if (_recLen >= MAX_SAMPLES) { _stopRecording(); return; }
        int chunk = min(BUF_SAMPLES, MAX_SAMPLES - _recLen);
        M5.Mic.record(_buf + _recLen, chunk, SAMPLE_RATE);
        _recLen += chunk;
    }

    void _stopRecording() {
        M5.Mic.end();
        _state = IDLE;
        if (_recLen > 0) {
            _saveFile();
            _hasFile = true;
        }
        onDraw();
    }

    void _startPlaying() {
        if (_recLen == 0 && _hasFile) _loadFile();
        if (_recLen == 0 || !_buf) return;
        _playPos = 0;
        M5.Speaker.begin();
        _state = PLAYING;
        onDraw();
    }

    void _tickPlay() {
        if (_playPos >= _recLen) { _stopAll(); return; }
        int chunk = min(BUF_SAMPLES, _recLen - _playPos);
        M5.Speaker.playRaw(_buf + _playPos, chunk, SAMPLE_RATE);
        _playPos += chunk;
    }

    void _stopAll() {
        if (_state == RECORDING) M5.Mic.end();
        if (_state == PLAYING)   M5.Speaker.end();
        _state = IDLE;
    }

    void _saveFile() {
        char path[64];
        int n = 1;
        while (n < 100) {
            snprintf(path, sizeof(path), "/audio/rec_%03d.wav", n);
            if (!LittleFS.exists(path)) break;
            n++;
        }
        strncpy(_filePath, path, sizeof(_filePath)-1);
        File f = LittleFS.open(path, "w");
        if (!f) return;
        // simple header: "FLXA" + sample_count(u32) + sample_rate(u16)
        f.write((uint8_t*)"FLXA", 4);
        uint32_t sc = _recLen; f.write((uint8_t*)&sc, 4);
        uint16_t sr = SAMPLE_RATE; f.write((uint8_t*)&sr, 2);
        f.write((uint8_t*)_buf, _recLen * 2);
        f.close();
    }

    void _loadFile() {
        free(_buf); _buf = nullptr; _recLen = 0;
        File f = LittleFS.open(_filePath, "r");
        if (!f) return;
        char hdr[4]; f.read((uint8_t*)hdr, 4);
        if (memcmp(hdr,"FLXA",4)!=0) { f.close(); return; }
        uint32_t sc; f.read((uint8_t*)&sc, 4);
        uint16_t sr; f.read((uint8_t*)&sr, 2);
        _buf = (int16_t*)malloc(sc * 2);
        if (!_buf) { f.close(); return; }
        f.read((uint8_t*)_buf, sc * 2);
        _recLen = sc;
        f.close();
    }
};
