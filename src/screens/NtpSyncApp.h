#pragma once
#include "Screen.h"
#include "Config.h"
#include <WiFi.h>
#include <time.h>

class NtpSyncApp : public Screen {
    enum State { IDLE, CONNECTING, SYNCING, DONE, ERROR_ST };
    State    _state    = IDLE;
    uint32_t _connStart = 0;
    char     _msg[64]  = {};

public:
    void onEnter() override {
        _state = IDLE; _msg[0] = 0;
        onDraw();
    }

    void onButtonA() override {
        if (_state == IDLE || _state == DONE || _state == ERROR_ST) {
            _startSync();
        }
    }

    void onButtonB() override { g_launchApp = -2; }

    void onLoop() override {
        if (_state == CONNECTING) {
            if (WiFi.status() == WL_CONNECTED) {
                _state = SYNCING;
                onDraw();
                _doSync();
            } else if (millis() - _connStart > 15000) {
                _state = ERROR_ST;
                strncpy(_msg, "WiFi timeout", sizeof(_msg)-1);
                onDraw();
            }
        }
    }

    void onDraw() override {
        M5.Lcd.fillRect(0, CNT_Y, SCR_W, CNT_H, C_BG);
        _drawClock();
        _drawStatus();
    }

private:
    void _startSync() {
        _state = CONNECTING;
        _msg[0] = 0;
        onDraw();
        WiFi.begin(WIFI_SSID, WIFI_PASS);
        _connStart = millis();
    }

    void _doSync() {
        configTime(0, 0, NTP_SERVER);
        setenv("TZ", NTP_TZ, 1); tzset();
        // wait up to 5s for NTP
        struct tm ti; bool ok = false;
        for (int i=0; i<50; i++) {
            if (getLocalTime(&ti, 100)) { ok=true; break; }
            delay(100);
        }
        if (ok) {
            // write back to RTC
            m5::rtc_time_t rt;
            rt.hours   = ti.tm_hour;
            rt.minutes = ti.tm_min;
            rt.seconds = ti.tm_sec;
            M5.Rtc.setTime(rt);
            m5::rtc_date_t rd;
            rd.year  = ti.tm_year + 1900;
            rd.month = ti.tm_mon  + 1;
            rd.date  = ti.tm_mday;
            M5.Rtc.setDate(rd);
            snprintf(_msg, sizeof(_msg), "Synced! %02d:%02d:%02d",
                     ti.tm_hour, ti.tm_min, ti.tm_sec);
            _state = DONE;
        } else {
            strncpy(_msg, "NTP timeout", sizeof(_msg)-1);
            _state = ERROR_ST;
        }
        WiFi.disconnect(true);
        onDraw();
    }

    void _drawClock() {
        m5::rtc_time_t t; M5.Rtc.getTime(&t);
        m5::rtc_date_t d; M5.Rtc.getDate(&d);
        char tbuf[16], dbuf[20];
        snprintf(tbuf, sizeof(tbuf), "%02d:%02d:%02d", t.hours, t.minutes, t.seconds);
        snprintf(dbuf, sizeof(dbuf), "%04d-%02d-%02d", d.year, d.month, d.date);

        M5.Lcd.setTextSize(3);
        M5.Lcd.setTextColor(C_WHITE, C_BG);
        M5.Lcd.setCursor(20, CNT_Y+20);
        M5.Lcd.print(tbuf);
        M5.Lcd.setTextSize(1);
        M5.Lcd.setTextColor(C_CYAN, C_BG);
        M5.Lcd.setCursor(60, CNT_Y+52);
        M5.Lcd.print(dbuf);
    }

    void _drawStatus() {
        const char* labels[] = {"[A] Sync NTP", "Connecting...", "Syncing...", "Done!", "Error"};
        uint16_t    cols[]   = {C_DGRAY, C_YELLOW, C_CYAN, C_GREEN, C_ACCENT};
        M5.Lcd.setTextSize(1);
        M5.Lcd.setTextColor(cols[_state], C_BG);
        M5.Lcd.setCursor(20, CNT_Y+70);
        M5.Lcd.print(labels[_state]);

        if (_msg[0]) {
            M5.Lcd.setTextColor(C_GRAY, C_BG);
            M5.Lcd.setCursor(20, CNT_Y+85);
            M5.Lcd.print(_msg);
        }

        // server info
        M5.Lcd.setTextColor(C_DGRAY, C_BG);
        M5.Lcd.setCursor(20, CNT_Y+100);
        M5.Lcd.print(NTP_SERVER);
        M5.Lcd.print(" / ");
        M5.Lcd.print(NTP_TZ);
    }
};
