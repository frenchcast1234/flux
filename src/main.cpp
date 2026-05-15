#include <M5StickCPlus2.h>
#include "FluxOS.h"

// page 1 - essentials
#include "screens/HomeScreen.h"
#include "screens/ClockApp.h"
#include "screens/GyroApp.h"
#include "screens/AudioMeter.h"
#include "screens/WifiScanner.h"
#include "screens/SnakeGame.h"
#include "screens/PongApp.h"
#include "screens/IRApp.h"
#include "screens/SettingsApp.h"

// page 2 - tools & sensors
#include "screens/StopwatchApp.h"
#include "screens/LevelApp.h"
#include "screens/SystemMonitor.h"
#include "screens/FlashlightApp.h"
#include "screens/ReactionGame.h"
#include "screens/BleScanner.h"
#include "screens/DiceApp.h"
#include "screens/AlarmApp.h"

int    g_launchApp = -1;
FluxOS gOS;

void setup() {
    gOS.addApp(new HomeScreen());    // 0

    // page 1 (grid 0-7 -> app index 1-8)
    gOS.addApp(new ClockApp());      // 1
    gOS.addApp(new GyroApp());       // 2
    gOS.addApp(new AudioMeter());    // 3
    gOS.addApp(new WifiScanner());   // 4
    gOS.addApp(new SnakeGame());     // 5
    gOS.addApp(new PongApp());       // 6
    gOS.addApp(new IRApp());         // 7
    gOS.addApp(new SettingsApp());   // 8

    // page 2 (grid 8-15 -> app index 9-16)
    gOS.addApp(new StopwatchApp());  // 9
    gOS.addApp(new LevelApp());      // 10
    gOS.addApp(new SystemMonitor()); // 11
    gOS.addApp(new FlashlightApp()); // 12
    gOS.addApp(new ReactionGame());  // 13
    gOS.addApp(new BleScanner());    // 14
    gOS.addApp(new DiceApp());       // 15
    gOS.addApp(new AlarmApp());      // 16

    gOS.begin();
}

void loop() {
    gOS.update();
    delay(5);
}
