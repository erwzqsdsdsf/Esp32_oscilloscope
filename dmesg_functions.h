/*
 
    dmesg_functions.h

    This file is part of Esp32_web_ftp_telnet_server_template project: https://github.com/BojanJurca/Esp32_web_ftp_telnet_server_template

      - Use dmesg functions to put system messages in circular message queue.

      - Use dmesg telnet command to display messages in message queue.

    December, 8, 2021, Bojan Jurca
    
*/


    // ----- includes, definitions and supporting functions -----
    
    #include <WiFi.h>
    #include <rom/rtc.h>


#ifndef __DMESG__
  #define __DMESG__


    // ----- functions and variables in this modul -----

    void dmesg (char *message1, char *message2);
    void dmesg (char *message1, const char *message2);
    void dmesg (const char *message1, char *message2);
    void dmesg (const char *message1, const char *message2);
    void dmesg (char *message1);
    void dmesg (const char *message1);
    void dmesg (String message1); // backward compatibility
    void dmesg (char *message1, int i);
    void dmesg (const char *message1, int i);
    String resetReasonAsString (RESET_REASON reason);
    String wakeupReasonAsString ();
    

    // ----- TUNNING PARAMETERS -----

    #define DMESG_CIRCULAR_QUEUE_LENGTH 256 // how may massages we keep on circular queue
    #define DMESG_MAX_MESSAGE_LENGTH 100    // max lenght of a message


    // ----- CODE -----


    // find and report reset reason (this may help with debugging)
    String resetReason (RESET_REASON reason) {
      switch (reason) {
        case 1:  return F ("POWERON_RESET - 1, Vbat power on reset");
        case 3:  return F ("SW_RESET - 3, Software reset digital core");
        case 4:  return F ("OWDT_RESET - 4, Legacy watch dog reset digital core");
        case 5:  return F ("DEEPSLEEP_RESET - 5, Deep Sleep reset digital core");
        case 6:  return F ("SDIO_RESET - 6, Reset by SLC module, reset digital core");
        case 7:  return F ("TG0WDT_SYS_RESET - 7, Timer Group0 Watch dog reset digital core");
        case 8:  return F ("TG1WDT_SYS_RESET - 8, Timer Group1 Watch dog reset digital core");
        case 9:  return F ("RTCWDT_SYS_RESET - 9, RTC Watch dog Reset digital core");
        case 10: return F ("INTRUSION_RESET - 10, Instrusion tested to reset CPU");
        case 11: return F ("TGWDT_CPU_RESET - 11, Time Group reset CPU");
        case 12: return F ("SW_CPU_RESET - 12, Software reset CPU");
        case 13: return F ("RTCWDT_CPU_RESET - 13, RTC Watch dog Reset CPU");
        case 14: return F ("EXT_CPU_RESET - 14, for APP CPU, reseted by PRO CPU");
        case 15: return F ("RTCWDT_BROWN_OUT_RESET - 15, Reset when the vdd voltage is not stable");
        case 16: return F ("RTCWDT_RTC_RESET - 16, RTC Watch dog reset digital core and rtc module");
        default: return F ("RESET REASON UNKNOWN");
      }
    } 

    // find and report reset reason (this may help with debugging)
    String wakeupReasonAsString () {
      esp_sleep_wakeup_cause_t wakeup_reason;
      wakeup_reason = esp_sleep_get_wakeup_cause ();
      switch (wakeup_reason){
        case ESP_SLEEP_WAKEUP_EXT0:     return F ("ESP_SLEEP_WAKEUP_EXT0 - wakeup caused by external signal using RTC_IO.");
        case ESP_SLEEP_WAKEUP_EXT1:     return F ("ESP_SLEEP_WAKEUP_EXT1 - wakeup caused by external signal using RTC_CNTL.");
        case ESP_SLEEP_WAKEUP_TIMER:    return F ("ESP_SLEEP_WAKEUP_TIMER - wakeup caused by timer.");
        case ESP_SLEEP_WAKEUP_TOUCHPAD: return F ("ESP_SLEEP_WAKEUP_TOUCHPAD - wakeup caused by touchpad.");
        case ESP_SLEEP_WAKEUP_ULP:      return F ("ESP_SLEEP_WAKEUP_ULP - wakeup caused by ULP program.");
        default:                        return String (F ("WAKEUP REASON UNKNOWN - wakeup was not caused by deep sleep: ")) + wakeup_reason + ".";
      }   
    }
    
    struct __dmesgType__ {
      unsigned long milliseconds;    
      time_t        time;
      char          message [DMESG_MAX_MESSAGE_LENGTH + 1];
    };
  
    RTC_DATA_ATTR unsigned int bootCount = 0;
  
    __dmesgType__ __dmesgCircularQueue__ [DMESG_CIRCULAR_QUEUE_LENGTH] = {{millis (), 0, ""}, 
                                                                          {millis (), 0, ""}, 
                                                                          {millis (), 0, ""},
                                                                          {millis (), 0, ""},
                                                                         }; // there are always at least 4 messages in the queue which makes things a little simpler - after reboot or deep sleep the time is preserved
    bool __initializeDmesgCircularQueue__ () {
      strcpy (__dmesgCircularQueue__ [0].message, (String ("[ESP32] CPU0 reset reason: ") + resetReason (rtc_get_reset_reason (0))).c_str ());
      strcpy (__dmesgCircularQueue__ [1].message, (String ("[ESP32] CPU0 reset reason: ") + resetReason (rtc_get_reset_reason (1))).c_str ());
      strcpy (__dmesgCircularQueue__ [2].message, (String ("[ESP32] wakeup reason: ") + wakeupReasonAsString ()).c_str ());
      strcpy (__dmesgCircularQueue__ [3].message, (String ("[ESP32] (re)started ") + ++bootCount + " times").c_str ());
      return true;      
    }
    bool __initializedDmesgCircularQueue__ = __initializeDmesgCircularQueue__ ();
  
    byte __dmesgBeginning__ = 0; // first used location
    byte __dmesgEnd__ = 4;       // the location next to be used
    static SemaphoreHandle_t __dmesgSemaphore__= xSemaphoreCreateMutex (); 
    
    // adds message into dmesg circular queue
    void dmesg (char *message1, char *message2) {
      Serial.printf ("[%10lu] %s%s\n", millis (), message1, message2);
      xSemaphoreTake (__dmesgSemaphore__, portMAX_DELAY);
        __dmesgCircularQueue__ [__dmesgEnd__].milliseconds = millis ();
        struct timeval now; gettimeofday (&now, NULL); __dmesgCircularQueue__ [__dmesgEnd__].time = now.tv_sec >= 946681200 ? now.tv_sec : 0; // only if time is >= 1.1.2000
        strncpy (__dmesgCircularQueue__ [__dmesgEnd__].message, message1, DMESG_MAX_MESSAGE_LENGTH);
        __dmesgCircularQueue__ [__dmesgEnd__].message [DMESG_MAX_MESSAGE_LENGTH] = 0;
        strncat (__dmesgCircularQueue__ [__dmesgEnd__].message, message2, DMESG_MAX_MESSAGE_LENGTH - strlen (__dmesgCircularQueue__ [__dmesgEnd__].message));
        __dmesgCircularQueue__ [__dmesgEnd__].message [DMESG_MAX_MESSAGE_LENGTH] = 0;
        if ((__dmesgEnd__ = (__dmesgEnd__ + 1) % DMESG_CIRCULAR_QUEUE_LENGTH) == __dmesgBeginning__) __dmesgBeginning__ = (__dmesgBeginning__ + 1) % DMESG_CIRCULAR_QUEUE_LENGTH;
      xSemaphoreGive (__dmesgSemaphore__);
    }
  
    // different versions of basically the same function

    void dmesg (char *message1, const char *message2) { dmesg (message1, (char *) message2); }
    void dmesg (const char *message1, char *message2) { dmesg ((char *) message1, message2); }
    void dmesg (const char *message1, const char *message2) { dmesg ((char *) message1, (char *) message2); }
    void dmesg (char *message1) { dmesg (message1, (char *) ""); }
    void dmesg (const char *message1) { dmesg ((char *) message1, (char *) ""); }
    void dmesg (String message1) { dmesg ((char *) message1.c_str (), ""); } // backward compatibilit
    void dmesg (char *message1, int i) { char s [6]; sprintf (s, "%i", i); dmesg (message1, s); }
    void dmesg (const char *message1, int i) { dmesg ((char *) message1, i); }

#endif
