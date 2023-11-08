/*
   Copyright 2020 Scott Bezek and the splitflap contributors

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

#include <Arduino.h>
#include <TFT_eSPI.h>
#include <Wire.h>
#include <WiFi.h>
#include <ESP32Time.h>

#include "config.h"

#include "../core/splitflap_task.h"
#include "display_task.h"
#include "serial_task.h"
#include <Ticker.h>

// Create Tickers
Ticker displayTicker; 
Ticker ntpTicker;
Ticker buttonTicker;

// Setup Tasks
SplitflapTask splitflapTask(1, LedMode::AUTO);
SerialTask serialTask(splitflapTask, 0);

// Create Real Time Clock
ESP32Time rtc;
// Booleans for handling button presses
volatile long nextAllowedButtonPress;
volatile bool buttonPressAllowed = false;

#include "webserver_task.h"
WebServerTask webserverTask(splitflapTask, serialTask, 1);

#if ENABLE_DISPLAY
DisplayTask displayTask(splitflapTask, 0);
#endif

#ifdef CHAINLINK_BASE
#include "../base/base_supervisor_task.h"
BaseSupervisorTask baseSupervisorTask(splitflapTask, serialTask, 0);
#endif

#if MQTT
#include "mqtt_task.h"
MQTTTask mqttTask(splitflapTask, serialTask, 0);
#endif

#if HTTP
#include "http_task.h"
HTTPTask httpTask(splitflapTask, displayTask, serialTask, 0);
#endif

// Function that resets the buttonPressAllowed boolean
void buttonStateMachine() {
  if (millis() > nextAllowedButtonPress) {
    buttonPressAllowed = true;
  }
}

// Function that connects to WiFi
void connectToWiFi() {
  // Connect to Wi-Fi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  // Wait for connection with a timeout
  unsigned long startMillis = millis();
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    serialTask.log("Connecting to WiFi...");
    
    // Check for a timeout (e.g., 30 seconds)
    if (millis() - startMillis > 30000) {
      serialTask.log("Wi-Fi connection timed out");
      // Handle the timeout (e.g., display a message or take another action)
      displayTask.wifi_failed = true;
      break;
    }
  }

  // Check the final connection status
  if (WiFi.status() == WL_CONNECTED) {
    serialTask.log("Connected to Wi-Fi");
    // Print the device IP address to the serial log
    serialTask.log(WiFi.localIP().toString().c_str());
  } else {
    serialTask.log("Wi-Fi connection failed");
    // Handle the connection failure (e.g., retry, display an error message, etc.)
  }
}

// Function that shows the current time on the splitflap display
// and handles button presses, lunch time, home time and tinker time
void showCurrentTime() {
  // Get the current time
  int curr_time = rtc.getTime("%H%M").toInt();

  if (!webserverTask.displayingMessage) {
        // Handle time display logic here
        // We must send the string in lower case, as capital letters are not supported.
        if (buttonPressAllowed) {
          // Button Pressed
          splitflapTask.showString("ooh...", NUM_MODULES, false);
          serialTask.log("BUTTON PRESSED");
          delay(5000); // Allow time (5 seconds) for message to be rotated and read
          splitflapTask.showString("..baby", NUM_MODULES, false);
          delay(5000); // Allow time (5 seconds) for message to be rotated and read
          nextAllowedButtonPress = millis() + 1800000; // Set the next allowed button press time to 30 minutes from now
          buttonPressAllowed = false;
        } else if (curr_time > 1158 && curr_time < 1202) {
          //Lunch Time
          splitflapTask.showString("lunch.", NUM_MODULES, false);
          serialTask.log("LUNCH.");
        } else if (curr_time > 1658 && curr_time < 1702) {
          // Home Time
          splitflapTask.showString(" home ", NUM_MODULES, false);
          serialTask.log(" HOME ");
        } else if (rtc.getDayofWeek() == TINKER_DAY && (curr_time > 1758 && curr_time < 1810)) {
          // Tinker Time
          splitflapTask.showString("tinker", NUM_MODULES, false);
          serialTask.log("TINKER");
        } else {
          // Show the current time
          splitflapTask.showString(rtc.getTime(" %H%M ").c_str(), NUM_MODULES, false);
          serialTask.log(rtc.getTime(" %H%M ").c_str());
        }
    } else {
        // Check if it's time to stop displaying the web message
        unsigned long currentTime = millis();
        if (currentTime - webserverTask.messageDisplayStartTime >= webserverTask.messageDisplayDuration) {
            webserverTask.displayingMessage = false;
        }
    }
}

// Function that sets the timezone to GMT/BST
void setTimezone(String timezone){
  serialTask.log("Setting Timezone to GMT/BST");
  setenv("TZ",timezone.c_str(),1);  //  Now adjust the TZ.  Clock settings are adjusted to show the new local time
  tzset();
}

// Function that sets up the NTP system
void setupCalibrateNTP() {
  configTime(0, 0, "ntp.kent.ac.uk", "pool.ntp.org");
  struct tm timeinfo;
  if (getLocalTime(&timeinfo)){
    rtc.setTimeStruct(timeinfo);
    displayTask.displayClockStatus("NTP Clock Connected", TFT_GREEN);
  } else {
    displayTask.displayClockStatus("NTP Clock Failed", TFT_RED);
  }
  // Set the timezone for Europe/London to adjust for daylight savings
  setTimezone("GMT0BST,M3.5.0/1,M10.5.0");
}

// Function that sets up the Ticker to call setupCalibrateNTP once at 3am
void ntpTickerFunction() {
  ntpTicker.attach(86400, setupCalibrateNTP);
  setupCalibrateNTP();
}

// Main setup function
void setup() {
  serialTask.begin();
  splitflapTask.begin();

  #if ENABLE_DISPLAY
  displayTask.begin();
  #endif

  #if MQTT
  mqttTask.begin();
  #endif

  #if HTTP
  httpTask.begin();
  #endif

  #ifdef CHAINLINK_BASE
  baseSupervisorTask.begin();
  #endif

  // Start WiFi connection
  connectToWiFi();

  // Start the NTP System
  setupCalibrateNTP();
  // Get the current time
  int curr_time = rtc.getTime("%H%M").toInt();
  // Calculate the number of seconds until 3am
  int secUntil3am = 2400 - curr_time + 180;
  // Set up the Ticker to call ntpTickerFunction once at 3am
  // This ensures that the clock is calibrated every day at 3am
  ntpTicker.once(secUntil3am, ntpTickerFunction);

  // Set up the Ticker to call showCurrentTime every 1 second
  displayTicker.attach(1, showCurrentTime);

  // Setup the button
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  // Make sure button press is only allowed every 30 minutes
  // buttonTicker.attach(1800, buttonTimer);
  nextAllowedButtonPress = millis() + 5000;
  attachInterrupt(BUTTON_PIN, buttonStateMachine, FALLING);

  webserverTask.begin();
  // Delete the default Arduino loopTask to free up Core 1
  vTaskDelete(NULL);
}

void loop() {
  assert(false);
}
