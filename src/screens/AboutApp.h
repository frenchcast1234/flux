#pragma once
#include "Screen.h"
#include <WiFi.h>

class AboutApp : public Screen {
    M5Canvas spr{&M5.Lcd};
    int page = 0;
    uint32_t lastDraw = 0;

public:
    void onEnter() override { spr.createSprite(SCR_W, CNT_H); page=0; onDraw(); }
    void onExit()  override { spr.deleteSprite(); }
    void onButtonA() override { page = (page+1)%3; onDraw(); }
    void onButtonB() override { page = (page+2)%3; onDraw(); }
    void onLoop()    override {
        if (millis()-lastDraw>2000) { lastDraw=millis(); onDraw(); }
    }

    void onDraw() override {
        spr.fillSprite(C_BG);

        // header
        spr.fillRect(0,0,SCR_W,14,C_PANEL);
        spr.setTextColor(C_ACCENT,C_PANEL); spr.setTextSize(1);
        spr.setCursor(4,3); spr.print("ABOUT FLUXOS");
        spr.setTextColor(C_DGRAY,C_PANEL);
        spr.setCursor(180,3); spr.printf("[%d/3]",page+1);

        switch (page) {
        case 0: _drawOsInfo();     break;
        case 1: _drawHardware();   break;
        case 2: _drawAppList();    break;
        }

        // page dots
        for (int i=0;i<3;i++) {
            spr.fillCircle(108+i*12, CNT_H-4, 3, (i==page)?C_ACCENT:C_DGRAY);
        }

        spr.pushSprite(0, CNT_Y);
    }

private:
    void _drawOsInfo() {
        // Mini logo
        spr.setTextSize(3); spr.setTextColor(C_WHITE, C_BG);
        spr.setCursor(40, 18); spr.print("FLUX");
        spr.fillRoundRect(135,20,38,18,3,C_ACCENT);
        spr.setTextColor(C_WHITE,C_ACCENT); spr.setTextSize(2);
        spr.setCursor(139,23); spr.print("OS");

        spr.setTextSize(1);
        spr.setTextColor(C_CYAN,C_BG); spr.setCursor(4,44);  spr.print("Version     v2.0");
        spr.setTextColor(C_WHITE,C_BG); spr.setCursor(4,54); spr.print("Build       " __DATE__);
        spr.setCursor(4,64); spr.print("Framework   Arduino/ESP32");
        spr.setCursor(4,74); spr.print("Apps        16 installed");
        spr.setCursor(4,84); spr.print("Author      FluxOS Project");

        uint32_t up=millis()/1000;
        spr.setTextColor(C_DGRAY,C_BG); spr.setCursor(4,94);
        spr.printf("Uptime      %luh%02lum%02lus", up/3600,(up/60)%60,up%60);
    }

    void _drawHardware() {
        spr.setTextSize(1);
        auto row=[&](int y, const char* lbl, String val, uint16_t col=C_WHITE){
            spr.setTextColor(C_CYAN,C_BG); spr.setCursor(4,y); spr.print(lbl);
            spr.setTextColor(col,C_BG);    spr.setCursor(110,y); spr.print(val);
        };

        row(18,  "Device",     "M5StickC Plus2");
        row(28,  "CPU",        String(getCpuFrequencyMhz())+" MHz");
        row(38,  "RAM free",   String(ESP.getFreeHeap()/1024)+" KB");
        row(48,  "Flash",      "4 MB (3MB app)");
        row(58,  "Chip",       String(ESP.getChipModel()));
        row(68,  "SDK",        String(ESP.getSdkVersion()).substring(0,8));
        float t=0; M5.Imu.getTemp(&t);
        row(78,  "IMU temp",   String(t,1)+" C", C_ORANGE);
        row(88,  "Battery",    String(M5.Power.getBatteryLevel())+"%",
            M5.Power.isCharging()?C_CYAN:C_GREEN);

        m5::rtc_date_t d; M5.Rtc.getDate(&d);
        char db[12]; snprintf(db,sizeof(db),"%04d-%02d-%02d",2000+d.year,d.month,d.date);
        row(98,  "Date",       db);
        row(108, "IR LED",     "GPIO 19");
    }

    void _drawAppList() {
        spr.setTextSize(1);
        const char* apps[] = {
            "Clock","Gyro Horizon","Audio Meter","WiFi Scan",
            "Snake","Pong","IR Remote","Settings",
            "Stopwatch","Bubble Level","Sys Monitor","Flashlight",
            "Reaction Test","BLE Scan","Dice","Tetris",
            "Breakout","Flappy Gyro","Calculator","Alarm",
            "Tally","Metronome"
        };
        int n = 22;
        int cols = 2, rows = 11;
        for (int i=0; i<min(n,rows*cols); i++) {
            int col = i/rows, row = i%rows;
            spr.setTextColor(C_DGRAY,C_BG);
            spr.setCursor(col*120+4, 18+row*9);
            spr.printf("• %s", apps[i]);
        }
    }
};
