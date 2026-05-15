#pragma once
#include "Screen.h"
#include "Config.h"
#include <WiFi.h>
#include <HTTPClient.h>

class WeatherApp : public Screen {
    enum State { DISCONNECTED, CONNECTING, FETCHING, DONE, ERROR_ST };
    State _state = DISCONNECTED;

    char _temp[8]   = {};
    char _desc[48]  = {};
    char _feels[8]  = {};
    char _humid[8]  = {};
    char _wind[12]  = {};
    char _city[32]  = WEATHER_CITY;

    uint32_t _connStart = 0;
    uint32_t _lastFetch = 0;
    static constexpr uint32_t REFRESH_MS = 5UL * 60 * 1000; // 5 min

public:
    void onEnter() override {
        _state = DISCONNECTED;
        memset(_temp,0,sizeof(_temp));
        memset(_desc,0,sizeof(_desc));
        onDraw();
        _connect();
    }

    void onExit() override {
        // don't disconnect — WiFi may be used by other apps
    }

    void onButtonA() override {
        // manual refresh
        _state = CONNECTING;
        onDraw();
        _connect();
    }

    void onButtonB() override { g_launchApp = -2; }

    void onLoop() override {
        if (_state == CONNECTING) {
            if (WiFi.status() == WL_CONNECTED) {
                _state = FETCHING;
                onDraw();
                _fetch();
            } else if (millis() - _connStart > 15000) {
                _state = ERROR_ST;
                strncpy(_desc, "WiFi timeout", sizeof(_desc)-1);
                onDraw();
            }
        }
        // auto-refresh
        if (_state == DONE && millis() - _lastFetch > REFRESH_MS) {
            _state = FETCHING; onDraw(); _fetch();
        }
    }

    void onDraw() override {
        M5.Lcd.fillRect(0, CNT_Y, SCR_W, CNT_H, C_BG);
        switch (_state) {
            case DISCONNECTED:
            case CONNECTING:  _drawConnecting(); break;
            case FETCHING:    _drawFetching();   break;
            case DONE:        _drawWeather();    break;
            case ERROR_ST:    _drawError();      break;
        }
    }

private:
    void _connect() {
        if (WiFi.status() != WL_CONNECTED) {
            WiFi.begin(WIFI_SSID, WIFI_PASS);
            _connStart = millis();
            _state = CONNECTING;
        } else {
            _state = FETCHING;
            _fetch();
        }
    }

    void _fetch() {
        HTTPClient http;
        // wttr.in plain text format: temp_C|description|feels_C|humidity%|windKmph
        char url[128];
        snprintf(url, sizeof(url),
            "http://wttr.in/%s?format=%%t|%%C|%%f|%%h|%%w", _city);
        http.begin(url);
        http.setTimeout(8000);
        int code = http.GET();
        if (code == 200) {
            String body = http.getString();
            // parse pipe-separated: temp|desc|feels|humid|wind
            _parseResponse(body.c_str());
            _state = DONE;
            _lastFetch = millis();
        } else {
            snprintf(_desc, sizeof(_desc), "HTTP %d", code);
            _state = ERROR_ST;
        }
        http.end();
        onDraw();
    }

    void _parseResponse(const char* s) {
        // format: "+18°C|Partly cloudy|+16°C|65%|↓11km/h"
        char buf[128]; strncpy(buf, s, sizeof(buf)-1);
        char* tok = strtok(buf, "|");
        if (tok) { strncpy(_temp,  tok, sizeof(_temp)-1);  tok = strtok(nullptr,"|"); }
        if (tok) { strncpy(_desc,  tok, sizeof(_desc)-1);  tok = strtok(nullptr,"|"); }
        if (tok) { strncpy(_feels, tok, sizeof(_feels)-1); tok = strtok(nullptr,"|"); }
        if (tok) { strncpy(_humid, tok, sizeof(_humid)-1); tok = strtok(nullptr,"|"); }
        if (tok) { strncpy(_wind,  tok, sizeof(_wind)-1);  }
        // strip trailing newlines
        for (int i=0;i<(int)sizeof(_temp);  i++) if(_temp[i]=='\n'||_temp[i]=='\r') _temp[i]=0;
        for (int i=0;i<(int)sizeof(_desc);  i++) if(_desc[i]=='\n'||_desc[i]=='\r') _desc[i]=0;
        for (int i=0;i<(int)sizeof(_feels); i++) if(_feels[i]=='\n'||_feels[i]=='\r') _feels[i]=0;
        for (int i=0;i<(int)sizeof(_humid); i++) if(_humid[i]=='\n'||_humid[i]=='\r') _humid[i]=0;
        for (int i=0;i<(int)sizeof(_wind);  i++) if(_wind[i]=='\n'||_wind[i]=='\r') _wind[i]=0;
    }

    void _drawConnecting() {
        uint16_t col = (_state==CONNECTING) ? C_CYAN : C_DGRAY;
        _drawIcon(col);
        M5.Lcd.setTextSize(1);
        M5.Lcd.setTextColor(col, C_BG);
        M5.Lcd.setCursor(70, CNT_Y+55);
        M5.Lcd.print(_state==CONNECTING ? "Connecting..." : "Press A to load");
        M5.Lcd.setTextColor(C_DGRAY, C_BG);
        M5.Lcd.setCursor(70, CNT_Y+68);
        M5.Lcd.print(WIFI_SSID);
    }

    void _drawFetching() {
        _drawIcon(C_YELLOW);
        M5.Lcd.setTextSize(1);
        M5.Lcd.setTextColor(C_YELLOW, C_BG);
        M5.Lcd.setCursor(70, CNT_Y+55);
        M5.Lcd.print("Fetching weather...");
        M5.Lcd.setTextColor(C_DGRAY, C_BG);
        M5.Lcd.setCursor(70, CNT_Y+68);
        M5.Lcd.print(_city);
    }

    void _drawWeather() {
        // City header
        M5.Lcd.setTextSize(1);
        M5.Lcd.setTextColor(C_CYAN, C_BG);
        M5.Lcd.setCursor(4, CNT_Y+4);
        M5.Lcd.print(_city);

        // big temperature
        M5.Lcd.setTextSize(3);
        M5.Lcd.setTextColor(C_WHITE, C_BG);
        M5.Lcd.setCursor(4, CNT_Y+18);
        M5.Lcd.print(_temp);

        // description
        M5.Lcd.setTextSize(1);
        M5.Lcd.setTextColor(C_YELLOW, C_BG);
        M5.Lcd.setCursor(4, CNT_Y+56);
        M5.Lcd.print(_desc);

        // details row
        M5.Lcd.setTextColor(C_DGRAY, C_BG);
        M5.Lcd.setCursor(4, CNT_Y+70);
        M5.Lcd.printf("Feels %s", _feels);
        M5.Lcd.setCursor(4, CNT_Y+82);
        M5.Lcd.printf("Humid %s  Wind %s", _humid, _wind);

        // refresh hint
        M5.Lcd.setTextColor(C_DGRAY, C_BG);
        M5.Lcd.setCursor(4, CNT_Y+CNT_H-10);
        M5.Lcd.print("[A]Refresh");

        // weather icon on right
        _drawIcon(C_CYAN);
    }

    void _drawError() {
        M5.Lcd.setTextSize(1);
        M5.Lcd.setTextColor(C_ACCENT, C_BG);
        M5.Lcd.setCursor(60, CNT_Y+45);
        M5.Lcd.print("Error:");
        M5.Lcd.setTextColor(C_GRAY, C_BG);
        M5.Lcd.setCursor(60, CNT_Y+58);
        M5.Lcd.print(_desc);
        M5.Lcd.setTextColor(C_DGRAY, C_BG);
        M5.Lcd.setCursor(60, CNT_Y+80);
        M5.Lcd.print("[A]Retry");
    }

    void _drawIcon(uint16_t col) {
        int cx=200, cy=CNT_Y+40;
        // sun
        M5.Lcd.fillCircle(cx, cy, 14, col);
        for (int t=0; t<8; t++) {
            float a = t*45*DEG_TO_RAD;
            M5.Lcd.drawLine(cx+(int)(16*cosf(a)), cy+(int)(16*sinf(a)),
                            cx+(int)(22*cosf(a)), cy+(int)(22*sinf(a)), col);
        }
    }
};
