#pragma once
#include "Screen.h"

class AlarmApp : public Screen {
    M5Canvas spr{&M5.Lcd};

    bool    alarmOn    = false;
    int8_t  alarmH     = 7, alarmM = 0;
    bool    alarming   = false;  // alarm triggered, buzzing
    bool    snoozed    = false;
    uint32_t snoozeEnd = 0;
    uint32_t lastBuzz  = 0;
    uint32_t lastDraw  = 0;

    // editing state
    enum EditState { VIEW, EDIT_H, EDIT_M };
    EditState editState = VIEW;

public:
    void onEnter() override { spr.createSprite(SCR_W, CNT_H); alarming=false; }
    void onExit()  override { spr.deleteSprite(); M5.Speaker.stop(); }

    void onButtonA() override { // navigate / increment
        if (alarming) { // snooze
            snoozed = true;
            snoozeEnd = millis() + 5*60*1000UL;
            alarming = false;
            M5.Speaker.stop();
        } else switch (editState) {
            case VIEW:    editState = EDIT_H; break;
            case EDIT_H:  alarmH = (alarmH + 1) % 24; break;
            case EDIT_M:  alarmM = (alarmM + 1) % 60; break;
        }
        onDraw();
    }
    void onButtonB() override { // confirm / toggle
        if (alarming) { // dismiss
            alarming=false; snoozed=false; alarmOn=false;
            M5.Speaker.stop();
        } else switch (editState) {
            case VIEW:    alarmOn = !alarmOn; break;
            case EDIT_H:  editState = EDIT_M; break;
            case EDIT_M:  alarmOn=true; editState=VIEW; break;
        }
        onDraw();
    }

    void onLoop() override {
        // snooze expiry
        if (snoozed && millis() > snoozeEnd) { snoozed=false; alarming=true; }

        // check alarm trigger
        if (alarmOn && !alarming && !snoozed) {
            m5::rtc_time_t t; M5.Rtc.getTime(&t);
            if (t.hours==alarmH && t.minutes==alarmM && t.seconds==0) alarming=true;
        }

        // buzz pattern
        if (alarming && millis()-lastBuzz > 600) {
            lastBuzz = millis();
            M5.Speaker.tone(880, 200);
        }

        if (millis()-lastDraw > 500) { lastDraw=millis(); onDraw(); }
    }

    void onDraw() override {
        spr.fillSprite(C_BG);

        if (alarming) {
            // full-screen alarm overlay
            bool flash = (millis()/300)%2;
            spr.fillSprite(flash ? C_ACCENT : C_PANEL);
            spr.setTextColor(C_WHITE, flash ? C_ACCENT : C_PANEL);
            spr.setTextSize(3); spr.setCursor(50, 20); spr.print("ALARM!");
            spr.setTextSize(2); spr.setCursor(40, 58);
            spr.printf("%02d:%02d", alarmH, alarmM);
            spr.setTextSize(1);
            spr.setCursor(20, 88); spr.print("[A] Snooze 5min  [B] Dismiss");
            spr.pushSprite(0, CNT_Y); return;
        }

        // clock display
        m5::rtc_time_t t; M5.Rtc.getTime(&t);
        char tBuf[8]; snprintf(tBuf, sizeof(tBuf), "%02d:%02d", t.hours, t.minutes);
        spr.setTextColor(C_DGRAY, C_BG); spr.setTextSize(2);
        spr.setCursor(86, 4); spr.print(tBuf);

        // divider
        spr.drawFastHLine(20, 24, SCR_W-40, C_PANEL);

        // alarm time (large)
        char aBuf[8]; snprintf(aBuf, sizeof(aBuf), "%02d:%02d", alarmH, alarmM);
        bool editH = (editState==EDIT_H), editM = (editState==EDIT_M);

        spr.setTextSize(5);
        // hours
        spr.setTextColor(editH ? C_YELLOW : (alarmOn ? C_WHITE : C_DGRAY), C_BG);
        spr.setCursor(20, 32); char hb[3]; snprintf(hb,sizeof(hb),"%02d",alarmH);
        spr.print(hb);
        // colon blink
        spr.setTextColor((millis()/500)%2 ? C_DGRAY : C_BG, C_BG);
        spr.setCursor(90, 32); spr.print(":");
        // minutes
        spr.setTextColor(editM ? C_YELLOW : (alarmOn ? C_WHITE : C_DGRAY), C_BG);
        spr.setCursor(110, 32); char mb[3]; snprintf(mb,sizeof(mb),"%02d",alarmM);
        spr.print(mb);

        // status badge
        uint16_t badgeCol = alarmOn ? C_GREEN : C_DGRAY;
        spr.fillRoundRect(170, 36, 60, 16, 4, badgeCol);
        spr.setTextColor(C_WHITE, badgeCol); spr.setTextSize(1);
        spr.setCursor(181, 40); spr.print(alarmOn ? "ON" : "OFF");

        // snooze indicator
        if (snoozed) {
            spr.setTextColor(C_ORANGE, C_BG); spr.setTextSize(1);
            spr.setCursor(4, 80); spr.print("SNOOZED  +5min");
        }

        // hints
        spr.drawFastHLine(20, 90, SCR_W-40, C_PANEL);
        spr.setTextSize(1); spr.setTextColor(C_DGRAY, C_BG);
        spr.setCursor(4, 96);
        switch (editState) {
        case VIEW:  spr.print("[A] edit time  [B] toggle on/off"); break;
        case EDIT_H: spr.print("[A] hour++  [B] next (minutes)"); break;
        case EDIT_M: spr.print("[A] min++   [B] confirm & activate"); break;
        }

        spr.pushSprite(0, CNT_Y);
    }
};
