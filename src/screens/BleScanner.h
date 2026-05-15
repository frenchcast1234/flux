#pragma once
#include "Screen.h"
#include <BLEDevice.h>
#include <BLEScan.h>

class BleScanner : public Screen {
    M5Canvas spr{&M5.Lcd};

    struct BLEEntry { char name[24]; char addr[18]; int rssi; };
    static constexpr int MAX_DEV = 12;
    BLEEntry devs[MAX_DEV];
    int      devCount  = 0;
    int      scrollOff = 0;
    bool     scanning  = false;
    uint32_t scanStart = 0;
    static constexpr int ROWS = 8;

    static BleScanner* _inst;
    static BLEScanResults* _results;

public:
    void onEnter() override {
        spr.createSprite(SCR_W, CNT_H);
        _inst = this;
        BLEDevice::init("");
        _startScan();
    }
    void onExit() override {
        if (scanning) BLEDevice::getScan()->stop();
        spr.deleteSprite();
    }

    void onButtonA() override {
        if (!scanning) {
            if (devCount > ROWS && scrollOff < devCount - ROWS) scrollOff++;
            else { scrollOff = 0; _startScan(); }
            onDraw();
        }
    }
    void onButtonB() override { scrollOff = 0; _startScan(); onDraw(); }

    void onLoop() override {
        if (scanning && millis() - scanStart > 4000) {
            _finishScan();
        }
    }

    void onDraw() override {
        spr.fillSprite(C_BG);
        spr.fillRect(0, 0, SCR_W, 12, C_PANEL);
        spr.setTextSize(1);
        spr.setTextColor(C_CYAN, C_PANEL);
        spr.setCursor(4, 2);
        if (scanning) {
            uint32_t dots = (millis() / 400) % 4;
            char buf[20]; snprintf(buf, sizeof(buf), "SCANNING BLE%.*s", (int)dots, "...");
            spr.print(buf);
        } else {
            char buf[40]; snprintf(buf, sizeof(buf), "%d devices  [A] scroll  [B] rescan", devCount);
            spr.print(buf);
        }

        if (scanning) {
            spr.setTextColor(C_PURPLE, C_BG);
            spr.setTextSize(2);
            spr.setCursor(70, CNT_H/2 - 8);
            spr.print("SCANNING");
            uint32_t pct = (millis() - scanStart) * 100 / 4000;
            spr.fillRect(20, CNT_H - 20, (int)(pct * 200 / 100), 8, C_PURPLE);
            spr.drawRect(20, CNT_H - 20, 200, 8, C_DGRAY);
            spr.pushSprite(0, CNT_Y);
            return;
        }

        int rowH = (CNT_H - 14) / ROWS;
        for (int i = 0; i < ROWS && (i + scrollOff) < devCount; i++) {
            int idx = i + scrollOff;
            int y   = 14 + i * rowH;
            spr.fillRect(0, y, SCR_W, rowH - 1, (i%2) ? C_PANEL : C_BG);

            int bars = map(constrain(devs[idx].rssi, -90, -30), -90, -30, 1, 4);
            for (int b = 0; b < 4; b++) {
                int bh = 3 + b*2;
                spr.fillRect(3 + b*5, y + rowH - 2 - bh, 4, bh,
                             b < bars ? C_PURPLE : C_DGRAY);
            }
            spr.setTextColor(C_WHITE, (i%2) ? C_PANEL : C_BG);
            spr.setCursor(28, y + 2);
            spr.print(devs[idx].name[0] ? devs[idx].name : "<unknown>");
            spr.setTextColor(C_DGRAY, (i%2) ? C_PANEL : C_BG);
            spr.setCursor(28, y + rowH/2 + 1);
            spr.printf("%s  %ddBm", devs[idx].addr, devs[idx].rssi);
        }

        if (devCount > ROWS) {
            int tH = CNT_H-14, bH = tH*ROWS/devCount, bY = 14+tH*scrollOff/devCount;
            spr.fillRect(SCR_W-3, 14, 3, tH, C_PANEL);
            spr.fillRect(SCR_W-3, bY, 3, bH, C_PURPLE);
        }

        spr.pushSprite(0, CNT_Y);
    }

private:
    void _startScan() {
        devCount = 0; scrollOff = 0; scanning = true; scanStart = millis();
        BLEScan* sc = BLEDevice::getScan();
        sc->setActiveScan(true);
        sc->setInterval(100); sc->setWindow(99);
        sc->start(4, _scanDone, false);
        onDraw();
    }

    void _finishScan() {
        scanning = false; onDraw();
    }

    static void _scanDone(BLEScanResults results) {
        if (!_inst) return;
        _inst->devCount = 0;
        int n = results.getCount();
        for (int i = 0; i < n && _inst->devCount < MAX_DEV; i++) {
            BLEAdvertisedDevice dev = results.getDevice(i);
            BLEEntry& e = _inst->devs[_inst->devCount++];
            strncpy(e.addr, dev.getAddress().toString().c_str(), sizeof(e.addr)-1);
            e.addr[sizeof(e.addr)-1] = 0;
            if (dev.haveName()) strncpy(e.name, dev.getName().c_str(), sizeof(e.name)-1);
            else e.name[0] = 0;
            e.name[sizeof(e.name)-1] = 0;
            e.rssi = dev.getRSSI();
        }
        _inst->scanning = false;
        _inst->onDraw();
    }
};
BleScanner* BleScanner::_inst = nullptr;
