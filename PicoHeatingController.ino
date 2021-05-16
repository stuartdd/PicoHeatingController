//
// Test data from the command line.
//
// curl http://192.168.1.177
// curl --data-binary "@data-full.json" -H 'Content-Type: application/json' http://192.168.1.177/timedata
//
// 0 = SAT
// 6 = FRI
#include <mbed.h>
#include "rtos.h"

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Ethernet.h>
#include <EthernetUdp.h>
#include <Dns.h>
#include <cstring>
#include <Time.h>

#include "FlashIAPBlockDevice.h"
#include "TDBStore.h"

using namespace rtos;

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define SCREEN_MIDDLE 40
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32

#define TITLE_HEIGHT 16 // The height of the yellow bit at the top of the screen
#define MAIN_HEIGHT 48 // The height of the bit below the yellow bit

#define SCR_SAV_X_MAX 125
#define SCR_SAV_Y_MAX 61

#define MODE_BUTTON_PIN 14

#define LINE_HEIGHT 12
#define LINE_0_Y 0
#define LINE_1_Y TITLE_HEIGHT
#define LINE_1X_Y (TITLE_HEIGHT + 2)
#define LINE_2X_Y (TITLE_HEIGHT + 34)

#define LINE_2_Y TITLE_HEIGHT
#define LINE_2_Y (TITLE_HEIGHT + LINE_HEIGHT)
#define LINE_3_Y (TITLE_HEIGHT + (LINE_HEIGHT * 2))
#define LINE_4_Y (TITLE_HEIGHT + (LINE_HEIGHT * 3) + 2)

#define NTP_POOL_URL "pool.ntp.org"
#define NTP_IP 129,6,15,30
#define NTP_TIME_OUT_MS 5000
#define TIME_SYNC_INTERVAL_SEC_LONG 600
#define TIME_SYNC_INTERVAL_SEC_SHORT 20

#define SEC_24_HOUR 86400
#define SEC_1_HOUR 3600
#define MIN_24_HOUR 1440
#define MIN_1_HOUR 60
#define MIN_1_WEEK 10080
#define MS_24_HOUR 86400000
#define MS_1_HOUR 360000
#define MS_1_SEC 1000
#define SLEEP_TIMER_PERIOD 300000
#define LED_TIMER_PERIOD 200
#define HALT_TIMER_PERIOD 10000
#define BUTTON_SCAN_TIMER_PERIOD 175

#define SCR_SAV_TIMER_PERIOD 200
#define SCR_SAV_STEP 2
#define SCR_SAV_WIDTH 2

#define UNIX_TIME_START_2021 1609459200

#define RECEIVE_BUFF_LEN 400
#define PATH_BUFF_LEN 50
#define TEMP_BUFF_LEN 20
#define CONT_TYPE_BUFF_LEN 40
#define HALT_REASON_BUFF_LEN 20
#define IP_ADDR_BUFF_LEN 21
#define TIME_BUFF_LEN 14
#define TIME_BUFF_SHORT_LEN 11
#define NTP_BUFF_LEN 48

#define ORD_OF_CHAR_0 48


#define TIME_STORE_SIZE 56
#define STORE_KEY_LEN 15

#define TIME_STORE_UNSET 65535

#define WATCHDOG_TIMEOUT 30000

#define ch2_width 55
#define ch2_height 26
static unsigned char ch2_bits[] = {
  0x3f, 0x00, 0x1f, 0x3f, 0xe0, 0x03, 0x3e, 0x3f, 0x00, 0x1f, 0x3e, 0xe0,
  0x03, 0x3e, 0x3f, 0x00, 0x1f, 0x3e, 0xf0, 0x03, 0x3f, 0x3f, 0x00, 0x1f,
  0x7e, 0xf0, 0x07, 0x3f, 0x3f, 0x00, 0x1f, 0x7e, 0xf0, 0x07, 0x3f, 0x3f,
  0x00, 0x1f, 0x7e, 0xf0, 0x07, 0x1f, 0x3f, 0x00, 0x1f, 0x7c, 0xf0, 0x07,
  0x1f, 0x3f, 0x00, 0x1f, 0x7c, 0xf8, 0x0f, 0x1f, 0x3f, 0x00, 0x1f, 0xfc,
  0xf8, 0x8f, 0x1f, 0x3f, 0x00, 0x1f, 0xfc, 0xf8, 0x8f, 0x1f, 0x3f, 0x80,
  0x1f, 0xf8, 0xf8, 0x8f, 0x0f, 0xff, 0xff, 0x1f, 0xf8, 0xf8, 0x8f, 0x0f,
  0xff, 0xff, 0x1f, 0xf8, 0x7c, 0x9f, 0x0f, 0xff, 0xff, 0x1f, 0xf8, 0x7c,
  0xdf, 0x0f, 0xff, 0xff, 0x1f, 0xf8, 0x3d, 0xdf, 0x0f, 0x3f, 0x00, 0x1f,
  0xf0, 0x3d, 0xde, 0x07, 0x3f, 0x00, 0x1f, 0xf0, 0x3f, 0xfe, 0x07, 0x3f,
  0x00, 0x1f, 0xf0, 0x3f, 0xfe, 0x07, 0x3f, 0x00, 0x1f, 0xf0, 0x1f, 0xfe,
  0x07, 0x3f, 0x00, 0x1f, 0xf0, 0x1f, 0xfc, 0x03, 0x3f, 0x00, 0x1f, 0xe0,
  0x1f, 0xfc, 0x03, 0x3f, 0x00, 0x1f, 0xe0, 0x1f, 0xfc, 0x03, 0x3f, 0x00,
  0x1f, 0xe0, 0x1f, 0xfc, 0x03, 0x3f, 0x00, 0x1f, 0xe0, 0x0f, 0xf8, 0x03,
  0x3f, 0x00, 0x1f, 0xe0, 0x0f, 0xf8, 0x01, 0x3f, 0x00, 0x1f, 0xc0, 0x0f,
  0xf8, 0x01
};

#define ch1_width 45
#define ch1_height 26
static unsigned char ch1_bits[] = {
  0x00, 0x1f, 0x00, 0x00, 0x00, 0x00, 0xc0, 0xff, 0x00, 0x3f, 0x80, 0x1f,
  0xf0, 0xff, 0x03, 0x3f, 0x80, 0x1f, 0xf8, 0xff, 0x07, 0x3f, 0x80, 0x1f,
  0xfc, 0xff, 0x07, 0x3f, 0x80, 0x1f, 0xfc, 0xc0, 0x0f, 0x3f, 0x80, 0x1f,
  0x7e, 0x80, 0x0f, 0x3f, 0x80, 0x1f, 0x7e, 0x80, 0x1f, 0x3f, 0x80, 0x1f,
  0x3e, 0x80, 0x1f, 0x3f, 0x80, 0x1f, 0x3e, 0x00, 0x00, 0x3f, 0x80, 0x1f,
  0x3f, 0x00, 0x00, 0x3f, 0x80, 0x1f, 0x3f, 0x00, 0x00, 0xff, 0xff, 0x1f,
  0x3f, 0x00, 0x00, 0xff, 0xff, 0x1f, 0x3f, 0x00, 0x00, 0xff, 0xff, 0x1f,
  0x3f, 0x00, 0x00, 0xff, 0xff, 0x1f, 0x3f, 0x00, 0x00, 0x3f, 0x80, 0x1f,
  0x3e, 0x00, 0x00, 0x3f, 0x80, 0x1f, 0x3e, 0x80, 0x1f, 0x3f, 0x80, 0x1f,
  0x7e, 0x80, 0x1f, 0x3f, 0x80, 0x1f, 0x7e, 0x80, 0x0f, 0x3f, 0x80, 0x1f,
  0xfc, 0xc0, 0x0f, 0x3f, 0x80, 0x1f, 0xfc, 0xff, 0x0f, 0x3f, 0x80, 0x1f,
  0xf8, 0xff, 0x07, 0x3f, 0x80, 0x1f, 0xf0, 0xff, 0x03, 0x3f, 0x80, 0x1f,
  0xe0, 0xff, 0x00, 0x3f, 0x80, 0x1f, 0x00, 0x1f, 0x00, 0x00, 0x00, 0x00
};

#define off_width 61
#define off_height 26
static unsigned char off_bits[] = {
  0x00, 0x1e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0xff, 0x00, 0xfe,
  0xff, 0xe1, 0xff, 0x1f, 0xf0, 0xff, 0x03, 0xfe, 0xff, 0xe1, 0xff, 0x1f,
  0xf8, 0xff, 0x07, 0xfe, 0xff, 0xe1, 0xff, 0x1f, 0xf8, 0xff, 0x07, 0xfe,
  0xff, 0xe1, 0xff, 0x1f, 0xfc, 0xc0, 0x0f, 0x7e, 0x00, 0xe0, 0x03, 0x00,
  0x7e, 0x80, 0x1f, 0x7e, 0x00, 0xe0, 0x03, 0x00, 0x7e, 0x80, 0x1f, 0x7e,
  0x00, 0xe0, 0x03, 0x00, 0x3e, 0x00, 0x1f, 0x7e, 0x00, 0xe0, 0x03, 0x00,
  0x3e, 0x00, 0x1f, 0x7e, 0x00, 0xe0, 0x03, 0x00, 0x3f, 0x00, 0x3f, 0x7e,
  0x00, 0xe0, 0x03, 0x00, 0x3f, 0x00, 0x3f, 0xfe, 0xff, 0xe1, 0xff, 0x0f,
  0x3f, 0x00, 0x3f, 0xfe, 0xff, 0xe1, 0xff, 0x0f, 0x3f, 0x00, 0x3f, 0xfe,
  0xff, 0xe1, 0xff, 0x0f, 0x3f, 0x00, 0x3f, 0xfe, 0xff, 0xe1, 0xff, 0x0f,
  0x3f, 0x00, 0x3f, 0x7e, 0x00, 0xe0, 0x07, 0x00, 0x3e, 0x00, 0x1f, 0x7e,
  0x00, 0xe0, 0x03, 0x00, 0x3e, 0x00, 0x1f, 0x7e, 0x00, 0xe0, 0x03, 0x00,
  0x7e, 0x80, 0x1f, 0x7e, 0x00, 0xe0, 0x03, 0x00, 0x7e, 0x80, 0x1f, 0x7e,
  0x00, 0xe0, 0x03, 0x00, 0xfc, 0xc0, 0x0f, 0x7e, 0x00, 0xe0, 0x03, 0x00,
  0xf8, 0xff, 0x0f, 0x7e, 0x00, 0xe0, 0x03, 0x00, 0xf8, 0xff, 0x07, 0x7e,
  0x00, 0xe0, 0x03, 0x00, 0xf0, 0xff, 0x03, 0x7e, 0x00, 0xe0, 0x03, 0x00,
  0xc0, 0xff, 0x00, 0x7e, 0x00, 0xe0, 0x03, 0x00, 0x00, 0x1e, 0x00, 0x7e,
  0x00, 0xe0, 0x03, 0x00
};

#define on_width 46
#define on_height 26
static unsigned char on_bits[] = {
  0x00, 0x1e, 0x00, 0x00, 0x00, 0x00, 0xc0, 0xff, 0x00, 0x7e, 0x00, 0x3f,
  0xf0, 0xff, 0x03, 0xfe, 0x00, 0x3f, 0xf8, 0xff, 0x07, 0xfe, 0x01, 0x3f,
  0xf8, 0xff, 0x0f, 0xfe, 0x01, 0x3f, 0xfc, 0xc0, 0x0f, 0xfe, 0x03, 0x3f,
  0x7e, 0x80, 0x1f, 0xfe, 0x03, 0x3f, 0x7e, 0x80, 0x1f, 0xfe, 0x07, 0x3f,
  0x3e, 0x00, 0x1f, 0xfe, 0x07, 0x3f, 0x3e, 0x00, 0x1f, 0xfe, 0x0f, 0x3f,
  0x3f, 0x00, 0x3f, 0x7e, 0x1f, 0x3f, 0x3f, 0x00, 0x3f, 0x7e, 0x1f, 0x3f,
  0x3f, 0x00, 0x3f, 0x7e, 0x3e, 0x3f, 0x3f, 0x00, 0x3f, 0x7e, 0x3e, 0x3f,
  0x3f, 0x00, 0x3f, 0x7e, 0x7c, 0x3f, 0x3f, 0x00, 0x3f, 0x7e, 0x78, 0x3f,
  0x3e, 0x00, 0x1f, 0x7e, 0xf8, 0x3f, 0x3e, 0x00, 0x1f, 0x7e, 0xf0, 0x3f,
  0x7e, 0x80, 0x1f, 0x7e, 0xf0, 0x3f, 0x7e, 0x80, 0x1f, 0x7e, 0xe0, 0x3f,
  0xfc, 0xc0, 0x0f, 0x7e, 0xe0, 0x3f, 0xf8, 0xff, 0x0f, 0x7e, 0xc0, 0x3f,
  0xf8, 0xff, 0x07, 0x7e, 0xc0, 0x3f, 0xf0, 0xff, 0x03, 0x7e, 0x80, 0x3f,
  0xc0, 0xff, 0x00, 0x7e, 0x00, 0x3f, 0x00, 0x1e, 0x00, 0x00, 0x00, 0x00
};

MbedI2C myi2c(p20, p21);
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &myi2c, 4);

const PROGMEM char CONT_LENGTH_STR[] = "Content-Length:";
const PROGMEM int CONT_LENGTH_STR_LEN = 15;
const PROGMEM char CONT_TYPE_STR[] = "Content-Type:";
const PROGMEM int CONT_TYPE_STR_LEN = 13;
const PROGMEM char GET_STR[] = "GET ";
const PROGMEM int GET_STR_LEN = 4;
const PROGMEM char POST_STR[] = "POST ";
const PROGMEM int POST_STR_LEN = 5;
const PROGMEM char RESP_APP_JSON[] = "application/json";
const PROGMEM char RESP_TXT_HTML[] = "text/html";
const PROGMEM char RESP_CONT_TYPE[] = "Content-Type: ";
const PROGMEM char RESP_CONT_LEN[] = "Content-Length: ";
const PROGMEM char RESP_HTTP_V[] = "HTTP/1.1 ";
const PROGMEM char RESP_HTTP_200[] = "OK";
const PROGMEM char RESP_HTTP_201[] = "CREATED";
const PROGMEM char RESP_HTTP_404[] = "NOT FOUND";
const PROGMEM char WEEK_DAY[][4] = {"---",  "SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT", "+++"};
const PROGMEM char INVALID_TIME[TIME_BUFF_LEN] = "---  --:--:--";
const PROGMEM char INVALID_TIME_SHORT[TIME_BUFF_SHORT_LEN] = "---  --:--";
const PROGMEM char TIME_STORE_C1_KEY[STORE_KEY_LEN] = "timeStoreC1";
const PROGMEM char TIME_STORE_C1_TAG[3] = "CH";
const PROGMEM unsigned int TIME_STORE_C1_ID = 1;
const PROGMEM char TIME_STORE_C2_KEY[STORE_KEY_LEN] = "timeStoreC2";
const PROGMEM char TIME_STORE_C2_TAG[3] = "HW";
const PROGMEM unsigned int TIME_STORE_C2_ID = 2;
const PROGMEM char IP_ADDR_STORE_KEY[STORE_KEY_LEN] = "ipAddrStore";
const PROGMEM unsigned int LOCAL_PORT = 8888;
const PROGMEM int timeZone = 0;

enum REQ_TYPE {
  RT_NOT_FOUND, RT_UNKNOWN, RT_GET, RT_POST
};
REQ_TYPE requestType = RT_NOT_FOUND;

enum SCREEN_MODE {
  SM_STATUS, SM_SUMMARY, SM_CH1, SM_CH2, SM_OFF
};
SCREEN_MODE screenMode = SM_SUMMARY;
SCREEN_MODE restoreScreenMode = SM_SUMMARY;

enum CHANNEL_MODE {
  CM_SCHEDULED, CM_BOOST, CM_OFF, CM_ON
};


// Enter a MAC address and IP address for your controller below.
// The IP address will be dependent on your local network:
struct TimeStoreStruct {
  int id;
  char key[STORE_KEY_LEN];
  char tag[3];
  uint16_t list[TIME_STORE_SIZE];
  uint16_t boost = 0;
  int count = 0;
  bool stateOn = false;
  char onOff[4];
  char onOffTime[TIME_BUFF_SHORT_LEN];
};


// Initialize the Ethernet server library
// with the IP address and port you want to use
// (port 80 is default for HTTP):

uint16_t ipAddressStore[4] = {192, 168, 1, 177};
byte mac[] = {
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED
};
EthernetServer server(80);
EthernetUDP ethernet_UDP;
IPAddress ntpAddress;

FlashIAPBlockDevice bd(XIP_BASE + 1024 * 1024, 1024 * 512);
mbed::TDBStore eeprom(&bd);
mbed::Watchdog &watchdog = mbed::Watchdog::get_instance();

TimeStoreStruct timeStoreC1;
TimeStoreStruct timeStoreC2;
TimeElements timeElements;

char receiveBuff[RECEIVE_BUFF_LEN + 2]; // Receive buffer for IP requests
char pathBuff[PATH_BUFF_LEN + 2];
char cTypeBuff[CONT_TYPE_BUFF_LEN + 2];

char timeBuff[TIME_BUFF_LEN];
char tempBuff[TEMP_BUFF_LEN];
char haltReasonBuff[HALT_REASON_BUFF_LEN]; // Used by haltDelayed!

char ipAddressBuffer[IP_ADDR_BUFF_LEN]; // Holds the (fixed after setup) IP address
byte ntpMessageBuffer[NTP_BUFF_LEN]; // Used by get time process only

int receiveBuffIndex = 0;
int contentLength = 0; // Read from request header Content-Length. We must read this many chars:
int contentCount = 0;  // Actual usable content length, we skip some white space chars.
bool endOfHeader = false;
bool daylightSaving = false;
bool systemIsRunning = true;

unsigned long currentMillis = 0;
unsigned long oneSecondEvent = 0;
unsigned long sleepTimerEvent = 0;
unsigned long scrSavTimerEvent = 0;
unsigned long buttonScanTimerEvent = 0;
unsigned long haltTimerEvent = 0;
unsigned long serialTimeout = 0;
unsigned long ledTimerEvent = 0;

unsigned long daylightSavingAdjust = 0;
unsigned int minuteZero = 0;
unsigned int ledCounter = 0;
unsigned int ledDutyCycle = 5;

int scrSavX = 0;
int scrSavXDir = 3;
int scrSavY = 0;
int scrSavYDir = 3;

bool modeButtonState = false;

Thread thread;

void setup() {
  watchdog.start(WATCHDOG_TIMEOUT);
  watchdog.kick();
  systemIsRunning = true;
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(MODE_BUTTON_PIN, INPUT_PULLUP);
  digitalWrite(LED_BUILTIN, HIGH);

  // Open serial communications and wait for port to open:
  serialTimeout = millis() + 1000;
  Serial.begin(115200);
  while (!Serial) {
    delay(100);
    if (millis()  > serialTimeout) {
      break;
    }
  }
  //
  // Set the speed for the Display I2C clock.
  // Then init and clear the display.
  //
  myi2c.setClock(2000000);
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    halt("SSD1306 failed");
  }
  display.clearDisplay();

  displayStatus("Network Setup", false, true);
  delay(100);

  eeprom.init();

  strcpy(timeStoreC1.key, TIME_STORE_C1_KEY);
  strcpy(timeStoreC1.tag, TIME_STORE_C1_TAG);
  timeStoreC1.id = TIME_STORE_C1_ID;
  resetTimeData(timeStoreC1);
  if (!getStoredTimeData(timeStoreC1)) {
    storeTimeData(timeStoreC1);
  }
  strcpy(timeStoreC2.key, TIME_STORE_C2_KEY);
  strcpy(timeStoreC2.tag, TIME_STORE_C2_TAG);
  timeStoreC2.id = TIME_STORE_C2_ID;
  resetTimeData(timeStoreC2);
  if (!getStoredTimeData(timeStoreC2)) {
    storeTimeData(timeStoreC2);
  }

  getStoredIpAddress();

  // Start the Ethernet connection and the server:
  // GP5 is used for Chip select of the Wiznet 5500 Ethernet SPI.
  // Default ports are GP2 - Serial Clock SCK
  // Default ports are GP3 - MISO (TX) Master IN Slave OUT
  // Default ports are GP4 - MOSI (RX)
  Ethernet.init(5);
  IPAddress ipAddress = IPAddress(ipAddressStore[0], ipAddressStore[1], ipAddressStore[2], ipAddressStore[3]);
  Ethernet.begin(mac, ipAddress);
  delay(100);

  // Check for Ethernet hardware present
  if (Ethernet.hardwareStatus() == EthernetNoHardware) {
    halt("EthernetNoHardware");
  }
  if (Ethernet.linkStatus() == LinkOFF) {
    halt("EthernetNoCable");
  }
  //
  // Start the web server and display the IP address
  //
  server.begin();
  delay(100);

  updateIpAddressBuffer();
  displayIp(LINE_4_Y);

  //
  // Start UDP client so we can call the NTP server to get the time.
  //
  resolveNtpAddress();
  ethernet_UDP.begin(LOCAL_PORT);
  delay(100);

  setSyncProvider(getNtpTime);
  setScreenMode(SM_SUMMARY);
  haltTimerEvent = 0;
  thread.start(displayThread);
  digitalWrite(LED_BUILTIN, LOW);
}

static void displayThread() {
  while (systemIsRunning) {
    currentMillis = millis();

    if ((haltTimerEvent > 0) && (currentMillis > haltTimerEvent)) {
      halt(haltReasonBuff);
    }

    if (currentMillis > sleepTimerEvent) {
      sleepTimerEvent = currentMillis + SLEEP_TIMER_PERIOD;
      setScreenMode(SM_OFF);
    }

    if (currentMillis > ledTimerEvent) {
      ledTimerEvent = currentMillis + LED_TIMER_PERIOD;
      ledCounter = ledCounter + 1;
      if (ledCounter > 10) {
        ledCounter = 0;
      }
      digitalWrite(LED_BUILTIN, (ledCounter > ledDutyCycle ? HIGH : LOW));
    }

    if (currentMillis > buttonScanTimerEvent) {
      buttonScanTimerEvent = currentMillis + BUTTON_SCAN_TIMER_PERIOD;
      if (digitalRead(MODE_BUTTON_PIN) == LOW) {
        if (!modeButtonState) {
          modeButtonState = true;
          switch (screenMode) {
            case SM_OFF:
              wakeupScreen();
              break;
            case SM_STATUS:
              setScreenMode(SM_SUMMARY);
              break;
            case SM_SUMMARY:
              setScreenMode(SM_CH1);
              break;
            case SM_CH1:
              setScreenMode(SM_CH2);
              break;
            case SM_CH2:
              setScreenMode(SM_STATUS);
              break;
          }
        }
      } else {
        modeButtonState = false;
      }
    }

    if (screenMode == SM_OFF) {
      if (currentMillis > scrSavTimerEvent) {
        scrSavTimerEvent = currentMillis + SCR_SAV_TIMER_PERIOD;
        watchdog.kick();
        moveScrSav();
        display.fillRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0);
        display.fillRect(scrSavX, scrSavY, SCR_SAV_WIDTH, SCR_SAV_WIDTH, 1);
        display.display();
      }
    } else {
      if (currentMillis > oneSecondEvent) {
        oneSecondEvent = currentMillis + MS_1_SEC;
        watchdog.kick();
        if (screenMode != SM_OFF) {
          displayTime();
        }
        getNextActionTime(timeStoreC1);
        getNextActionTime(timeStoreC2);
        switch (screenMode) {
          case SM_STATUS:
            statusScreen();
            break;
          case SM_SUMMARY:
            summaryScreen();
            break;
          case SM_CH1:
            channelScreen(timeStoreC1);
            break;
          case SM_CH2:
            channelScreen(timeStoreC2);
            break;
        }
        display.display();
      }
    }
  }
}

void loop() {
  // listen for incoming clients
  EthernetClient client = server.available();
  if (client) {
    digitalWrite(LED_BUILTIN, HIGH);
    if (Serial) {
      Serial.println("new client");
    }
    wakeupScreen();
    clearResp();
    contentLength = 0;
    contentCount = 0;
    requestType = RT_NOT_FOUND;
    endOfHeader = false;
    pathBuff[0] = 0;
    cTypeBuff[0] = 0;

    // an http request ends with a blank line
    while (client.connected()) {
      char c = 0;
      if (client.available()) {
        c = client.read();
        if ((endOfHeader) && (contentLength > 0)) {
          contentCount++;
          if ((c == 10) || (c > 31)) {
            if (receiveBuffIndex < RECEIVE_BUFF_LEN) {
              appendChar(c);
            }
          }
        } else {
          if (c >= 32) {
            if (receiveBuffIndex < RECEIVE_BUFF_LEN) {
              appendChar(c);
            }
          }
          if (c == 10) {
            if (receiveBuffIndex == 0) {
              endOfHeader = true;
            } else {
              processHeader();
            }
            receiveBuffIndex = 0;
          }
        }
      }

      if (endOfHeader) {
        if (requestType == RT_GET) {
          sendGetResponse(client);
          break;
        } else {
          if (contentCount >= contentLength) {
            sendPostResponse(client);
            break;
          }
        }
      }
    }
    // give the web browser time to receive the data
    delay(1);
    // close the connection:
    client.stop();
    digitalWrite(LED_BUILTIN, LOW);
  }
}

void sendGetResponse(EthernetClient client) {
  clearResp();
  if (strcmp("/reset", pathBuff) == 0) {
    appendChar('{');
    appendQuoteEnt("reset", ':');
    appendResp("8}");
    sendResponse(client, 200, RESP_HTTP_200, RESP_APP_JSON);
    haltDelayed("RESET!");
  }

  if (strcmp("/time", pathBuff) == 0) {
    appendChar('{');
    appendQuoteEnt("time", ':');
    appendQuoteEnt(timeBuff, '}');
    sendResponse(client, 200, RESP_HTTP_200, RESP_APP_JSON);
  } else {
    if (strcmp("/index", pathBuff) == 0) {
      sendIndexResponse(client);
    } else {
      sendErrorResponse(client, 404, RESP_HTTP_404);
    }
  }
}

void sendPostResponse(EthernetClient client) {
  trimReceiveBuffer();
  if (Serial) {
    Serial.println("*** POST ***");
    Serial.print("Content-Length(");
    Serial.print(contentCount);
    Serial.print(") Content-Type(");
    Serial.print(String(cTypeBuff));
    Serial.println(") ");
  }
  if (strcmp("/timedata1", pathBuff) == 0) {
    readTimeDataFromPostData(timeStoreC1);
    sendResponseWithBody(client, 201, RESP_HTTP_201, RESP_APP_JSON, "{\"timedata1\":201}");
    sendResponse(client, 201, RESP_HTTP_201, RESP_APP_JSON);
  }
  if (strcmp("/timedata2", pathBuff) == 0) {
    readTimeDataFromPostData(timeStoreC2);
    sendResponseWithBody(client, 201, RESP_HTTP_201, RESP_APP_JSON, "{\"timedata2\":201}");
    sendResponse(client, 201, RESP_HTTP_201, RESP_APP_JSON);
  }
  if (strcmp("/ip", pathBuff) == 0) {
    readIpAddressFromPostData();
    sendResponseWithBody(client, 201, RESP_HTTP_201, RESP_APP_JSON, "{\"ipSet\":201}");
    haltDelayed("IP Address change!");
  }
}

void readIpAddressFromPostData() {
  Serial.print("POST:Read ipAddress:");
  Serial.println(IP_ADDR_STORE_KEY);
  int index = 0;
  int tbIndex = 0;
  char c;
  for (int i = 0; i < RECEIVE_BUFF_LEN; i++) {
    c = receiveBuff[i];
    if ((c == ',') || (c == ']')) {
      if (tbIndex > 0) {
        ipAddressStore[index] = atoi(tempBuff);
        index++;
      }
      tbIndex = 0;
    } else {
      if ((c >= '0') && (c <= '9')) {
        tempBuff[tbIndex] = c;
        tbIndex++;
        tempBuff[tbIndex] = 0;
      }
    }
    if ((c == 0) || (c == ']') || (index >= 4)) {
      storeIpAddress();
      break;
    }
  }
}

void readTimeDataFromPostData(TimeStoreStruct &ts) {
  resetTimeData(ts);
  Serial.print("POST: Read time data:");
  Serial.println(ts.key);
  int index = 0;
  int tbIndex = 0;
  char c;
  for (int i = 0; i < RECEIVE_BUFF_LEN; i++) {
    c = receiveBuff[i];
    if ((c == ',') || (c == ']')) {
      if (tbIndex > 0) {
        ts.list[index] = atoi(tempBuff);
        index++;
      }
      tbIndex = 0;
    } else {
      if ((c >= '0') && (c <= '9')) {
        tempBuff[tbIndex] = c;
        tbIndex++;
        tempBuff[tbIndex] = 0;
      }
    }
    if ((c == 0) || (c == ']') || (index >= (TIME_STORE_SIZE))) {
      storeTimeData(ts);
      break;
    }
  }
  countTimeData(ts);
}

void sendErrorResponse(EthernetClient client, const int code, const char* msg) {
  appendChar('{');
  appendQuoteEnt("error", ':');
  appendQuoteEnt(msg, ',');
  appendQuoteEnt("code", ':');
  itoa(code, tempBuff, 10);
  appendResp(tempBuff);
  appendChar(',');
  appendQuoteEnt("path", ':');
  appendQuoteEnt(pathBuff, '}');
  sendResponse(client, code, msg, RESP_APP_JSON);
}

void sendIndexResponse(EthernetClient client) {
  client.print(RESP_HTTP_V);
  client.print(200);
  client.print(' ');
  client.println(RESP_HTTP_200);
  client.print(RESP_CONT_TYPE);
  client.println(RESP_TXT_HTML);
  client.println();
  client.println("<!DOCTYPE HTML>");
  client.println("<html>");
  client.println("<head>");
  client.println("<title>Page Title</title>");
  client.println("</head>");
  client.println("<body>");
  client.println("<h1>This is a Heading</h1>");
  client.print("<p>Time (");
  client.print(requestTypeStr());
  client.print(") ");
  updateTimeBuff(timeBuff, now(), true);
  client.println(timeBuff);
  client.print("</p>");
  client.println("</body>");
  client.println("</html>");
}

void sendResponseWithBody(EthernetClient client, const int code, const char* codeMsg, const char* contentType, char* body) {
  strcpy(receiveBuff, body);
  sendResponse(client, code, codeMsg, contentType);
}

void sendResponse(EthernetClient client, const int code, const char* codeMsg, const char* contentType) {
  client.print(RESP_HTTP_V);
  client.print(code);
  client.print(' ');
  client.println(codeMsg);
  client.print(RESP_CONT_TYPE);
  client.println(contentType);
  client.print(RESP_CONT_LEN);
  int len = 0;
  for (int i = 0; i < RECEIVE_BUFF_LEN; i++) {
    if (receiveBuff[i] == 0) {
      len = i;
      break;
    }
  }
  client.println(len);
  client.println();
  client.print(receiveBuff);
}

/*
  Detirmine the type of request (GET, POST etc) and then
  read the path. The first line of an HTTP request is defined as follows:
  <type> <path> <http-version>
  If it is a GET then were have nothing to do once we get to an empty line.
  For other types we may need to read another N characters defined by the Content-Length header.
*/
void processHeader() {
  int pos = 0;
  // RT_NOT_FOUND means we have not tried to find the type yet.
  //    We only ever try once.
  // When we find a recognised type we read the path in to pathBuff as a string.
  // If not recognised set type to RT_UNKNOWN and ignore the path!
  if (requestType == RT_NOT_FOUND) {
    pos = matchAt(0, GET_STR, GET_STR_LEN);
    if (pos > 0) {
      requestType = RT_GET;
      readStr(pos, pathBuff, PATH_BUFF_LEN, ' ');
    } else {
      pos = matchAt(0, POST_STR, POST_STR_LEN);
      if (pos > 0) {
        requestType = RT_POST;
        readStr(pos, pathBuff, PATH_BUFF_LEN, ' ');
      } else {
        requestType = RT_UNKNOWN;
      }
    }
    if (Serial) {
      Serial.print("Request PATH(");
      Serial.print(String(pathBuff));
      Serial.print(") ");
      Serial.print("TYPE(");
      Serial.print(String(requestTypeStr()));
      Serial.println(") ");
    }
  }
  if (requestType == RT_POST) {
    // If it is a POST then we need to look for Content-length and Content-Type:
    // Line starts Content-Length: N
    // Line starts Content-Type: <the-content-type>
    // We can use the content type to check the format of the content, JSON, XML, Text etc.
    int matchPos = matchAt(0, CONT_LENGTH_STR, CONT_LENGTH_STR_LEN);
    if (matchPos > 0) {
      // If the line starts with Content-Length: then read N as an integer.
      contentLength = readInt(matchPos);
    }
    //
    matchPos = matchAt(0, CONT_TYPE_STR, CONT_TYPE_STR_LEN);
    if (matchPos > 0) {
      // If the line starts with Content-Type: then read the type into cTypeBuff as a string.
      readStr(matchPos, cTypeBuff, CONT_TYPE_BUFF_LEN, ' ');
    }
  }
  if (Serial) {
    Serial.print("Heading: len(");
    Serial.print(String(receiveBuffIndex));
    Serial.print(") ");
    Serial.println(receiveBuff);
  }
}

void haltDelayed(const char* txt) {
  strcpy(haltReasonBuff, txt);
  haltTimerEvent = millis() + HALT_TIMER_PERIOD;
  if (Serial) {
    Serial.print("HALT Delayed:");
    Serial.println(String(haltReasonBuff));
  }
}

void halt(const char* txt) {
  systemIsRunning = false;
  if (Serial) {
    Serial.println("HALT:" + String(txt));
  }
  while (true) {
    delay(200);
    displayStatus(txt, true, false);
    digitalWrite(LED_BUILTIN, HIGH);
    delay(200);
    displayStatus(txt, false, false);
    digitalWrite(LED_BUILTIN, LOW);
  }
}

void wakeupScreen() {
  setScreenMode(restoreScreenMode);
}

void setScreenMode(SCREEN_MODE mode) {
  sleepTimerEvent = millis() + SLEEP_TIMER_PERIOD;
  if (screenMode != mode) {
    switch (mode) {
      case SM_OFF:
        ledDutyCycle = 9;
        restoreScreenMode = screenMode;
        break;
      default:
        ledDutyCycle = 5;
    }
    display.fillRect(0, TITLE_HEIGHT, SCREEN_WIDTH, MAIN_HEIGHT, 0);
    screenMode = mode;
  }
}

void summaryScreen() {
  display.setTextSize(2);
  display.setTextColor(1);
  int w2 = (SCREEN_WIDTH / 2);
  display.fillRect(0, TITLE_HEIGHT, SCREEN_WIDTH, MAIN_HEIGHT , 0);
  if (timeStoreC2.stateOn) {
    display.fillRect(0, TITLE_HEIGHT, w2,  ch2_height + 5, 1);
    display.drawXBitmap(5, LINE_1X_Y, ch2_bits, ch2_width, ch2_height, 0);
    display.setCursor(20, LINE_2X_Y);
  } else {
    display.drawXBitmap(5, LINE_1X_Y, ch2_bits, ch2_width, ch2_height, 1);
    display.setCursor(16, LINE_2X_Y);
  }
  display.print(timeStoreC2.onOff);

  if (timeStoreC1.stateOn) {
    display.fillRect(w2 + 10, TITLE_HEIGHT, w2 - 10,  ch2_height + 5, 1);
    display.drawXBitmap(w2 + 14, LINE_1X_Y, ch1_bits, ch1_width, ch1_height, 0);
    display.setCursor(w2 + 25, LINE_2X_Y);
  } else {
    display.drawXBitmap(w2 + 14, LINE_1X_Y, ch1_bits, ch1_width, ch1_height, 1);
    display.setCursor(w2 + 20, LINE_2X_Y);
  }
  display.print(timeStoreC1.onOff);
  display.setTextSize(1);
}

void channelScreen(TimeStoreStruct &ts) {
  if (ts.id == TIME_STORE_C1_ID) {
    display.drawXBitmap(0, LINE_1X_Y, ch1_bits, ch1_width, ch1_height, 1);
  } else {
    display.drawXBitmap(0, LINE_1X_Y, ch2_bits, ch2_width, ch2_height, 1);
  }
  if (ts.stateOn) {
    display.fillRect(SCREEN_WIDTH - off_width - 2, LINE_1X_Y - 1 , on_width + 4,  off_height + 2, 1);
    display.drawXBitmap(SCREEN_WIDTH - off_width, LINE_1X_Y, on_bits, on_width, on_height, 0);
  } else {
    display.fillRect(SCREEN_WIDTH - off_width, LINE_1X_Y, off_width,  off_height, 0);
    display.drawXBitmap(SCREEN_WIDTH - off_width, LINE_1X_Y, off_bits, off_width, off_height, 1);
  }
  display.setTextSize(2);
  display.setTextColor(1);
  display.setCursor(0, LINE_2X_Y);
  display.print(ts.onOffTime[0]);
  display.print(ts.onOffTime[1]);
  display.print(ts.onOffTime[2]);
  display.setCursor(SCREEN_WIDTH - off_width, LINE_2X_Y);
  display.print(&ts.onOffTime[4]);
  display.setTextSize(1);
}

void statusScreen() {
  display.fillRect(0, LINE_1_Y, SCREEN_WIDTH, LINE_HEIGHT - 1, 0);
  display.setTextColor(1);
  display.setCursor(2, LINE_1_Y + 2);
  display.print("Min:");
  display.print(deriveMinuteOfWeek());
  display.print(":");
  updateTimeBuff(timeBuff, (minuteZero) * 60, true);
  display.print(timeBuff);
  display.fillRect(0, LINE_2_Y, SCREEN_WIDTH, LINE_HEIGHT - 1, timeStoreC1.stateOn ? 1 : 0);
  display.setTextColor(timeStoreC1.stateOn ? 0 : 1);
  display.drawRect(0, LINE_2_Y, SCREEN_WIDTH, LINE_HEIGHT - 1, 1);

  display.setCursor(2, LINE_2_Y + 2);
  display.print(timeStoreC1.tag);
  display.print(": Untill ");
  display.print(timeStoreC1.onOffTime);
  display.fillRect(0, LINE_3_Y, SCREEN_WIDTH, LINE_HEIGHT - 1, timeStoreC2.stateOn ? 1 : 0);
  display.setTextColor(timeStoreC2.stateOn ? 0 : 1);
  display.drawRect(0, LINE_3_Y, SCREEN_WIDTH, LINE_HEIGHT - 1, 1);

  display.setCursor(2, LINE_3_Y + 2);
  display.print(timeStoreC2.tag);
  display.print(": Untill ");
  display.print(timeStoreC2.onOffTime);
  displayIp(LINE_4_Y);
}

void displayStatus(const char* txt, bool inv, bool echo) {
  if (Serial && echo) {
    Serial.print(millis());
    Serial.println(": Status:" + String(txt));
  }
  if (inv) {
    display.fillRect(0, 0, SCREEN_WIDTH, TITLE_HEIGHT, 1);
    display.setTextColor(0);
  } else {
    display.fillRect(0, 0, SCREEN_WIDTH, TITLE_HEIGHT, 0);
    display.setTextColor(1);
  }
  display.setCursor(2, 4);
  display.print(txt);
  display.display();
}

void displayIp(int y) {
  display.fillRect(0, y, SCREEN_WIDTH, LINE_HEIGHT - 1, 1);
  display.setTextColor(0);
  display.setCursor(2, y + 2);
  display.print(ipAddressBuffer);
}

void displayTime() {
  if (updateTimeBuff(timeBuff, now(), true)) {
    display.fillRect(SCREEN_MIDDLE, LINE_0_Y, SCREEN_WIDTH - SCREEN_MIDDLE, TITLE_HEIGHT, 1);
    display.setTextColor(0);
    display.setCursor(SCREEN_MIDDLE + 4, LINE_0_Y + 4);
    display.print(timeBuff);
  }
}

void moveScrSav() {
  scrSavX = scrSavX + scrSavXDir;
  if (scrSavX >= SCR_SAV_X_MAX) {
    scrSavX = SCR_SAV_X_MAX - SCR_SAV_WIDTH;
    scrSavXDir = -SCR_SAV_STEP;
  }
  if (scrSavX <= 0) {
    scrSavX = 0;
    scrSavXDir = SCR_SAV_STEP;
  }
  scrSavY = scrSavY + scrSavYDir;
  if (scrSavY >= SCR_SAV_Y_MAX) {
    scrSavY = SCR_SAV_Y_MAX - SCR_SAV_WIDTH;
    scrSavYDir = -SCR_SAV_STEP;
  }
  if (scrSavY <= 0) {
    scrSavY = 0;
    scrSavYDir = SCR_SAV_STEP;
  }
}

//
// TimeBuff:
// 0 1 2 3 4 5 6 7 8 9 0 1 2 3
// M O N   + 1 2 : 3 0 : 2 9 0
// - - -   - - - : - - : - - 0
//
bool updateTimeBuff(char* buff, time_t secondsFromEpoch, bool dispSeconds) {
  int ts = timeStatus();
  if (ts == timeNotSet) {
    strcpy(buff, dispSeconds ? INVALID_TIME : INVALID_TIME_SHORT);
    return false;
  }

  char sep = ':';
  if (ts == timeNeedsSync) {
    sep = '.';
  }

  breakTime(secondsFromEpoch, timeElements);
  buff[0] = WEEK_DAY[timeElements.Wday][0];
  buff[1] = WEEK_DAY[timeElements.Wday][1];
  buff[2] = WEEK_DAY[timeElements.Wday][2];
  buff[3] = ' ';
  if (dispSeconds) {
    buff[4] = daylightSaving ? '+' : ' ';
    pushIntToBuff(buff, timeElements.Hour, 5, 2, sep);
    pushIntToBuff(buff, timeElements.Minute, 8, 2, sep);
    pushIntToBuff(buff, timeElements.Second, 11, 2, 0);
  } else {
    pushIntToBuff(buff, timeElements.Hour, 4, 2, sep);
    pushIntToBuff(buff, timeElements.Minute, 7, 2, 0);
  }
  return true;
}

void pushIntToBuff(char* buff, const long value, const int ind, const int len, const char c) {
  if (len < 3) {
    buff[ind] = ORD_OF_CHAR_0 + value / 10;
    buff[ind + 1] = ORD_OF_CHAR_0 +  value % 10;
    buff[ind + 2] = c;
  } else {
    buff[ind] = ORD_OF_CHAR_0 + (value / 100);
    buff[ind + 1] = ORD_OF_CHAR_0 + (value % 100) / 10;
    buff[ind + 2] = ORD_OF_CHAR_0 +  value % 10;
    buff[ind + 3] = c;
  }
}

void updateIpAddressBuffer() {
  strcpy(ipAddressBuffer, "IP: ");
  pushIntToBuff(ipAddressBuffer, Ethernet.localIP()[0], 4, 3, ':');
  pushIntToBuff(ipAddressBuffer, Ethernet.localIP()[1], 8, 3, ':');
  pushIntToBuff(ipAddressBuffer, Ethernet.localIP()[2], 12, 3, ':');
  pushIntToBuff(ipAddressBuffer, Ethernet.localIP()[3], 16, 3, 0);
}

int matchAt(int start, const char* with, int matchLen) {
  if (matchLen < 1) {
    return 0;
  }
  for (int i = 0; i < matchLen; i++) {
    if (receiveBuff[start + i] != with[i]) {
      return 0;
    }
  }
  return matchLen;
}

int findStr(int start, const char* needle, int matchLen) {
  if (matchLen < 1) {
    return -1;
  }
  int mLen = 0;
  for (int i = start; i < RECEIVE_BUFF_LEN; i++) {
    if (receiveBuff[i] == needle[0]) {
      if (matchLen == 1) {
        return i + matchLen;
      }
      mLen = matchAt(i + 1, &needle[1], matchLen - 1);
      if (mLen > 0) {
        return i + matchLen;
      }
    }
  }
  return -1;
}

int readInt(int from) {
  int n = 0;
  bool start = false;
  for (int i = from; i < RECEIVE_BUFF_LEN; i++) {
    char c = receiveBuff[i];
    if ((c != ' ') || start) {
      start = true;
      if ((c >= '0') && (c <= '9')) {
        n = (n * 10) + (c - ORD_OF_CHAR_0);
      } else {
        break;
      }
    }
  }
  return n;
}

int readStr(int from, char*outBuff, int maxLen, char stopAt) {
  bool start = false;
  int outPos = 0;
  outBuff[outPos] = 0;
  for (int i = from; i < RECEIVE_BUFF_LEN; i++) {
    char c = receiveBuff[i];
    if ((c != ' ') || start) {
      start = true;
      if ((c >= ' ') && (c <= 127) && (c != stopAt)) {
        outBuff[outPos] = c;
        outPos++;
        outBuff[outPos] = 0;
        if (outPos >= maxLen) {
          return outPos;
        }
      } else {
        return outPos;
      }
    }
  }
  return outPos;
}

void trimReceiveBuffer() {
  while ((receiveBuffIndex > 0) && (receiveBuff[receiveBuffIndex] < 32)) {
    receiveBuff[receiveBuffIndex] = 0;
    receiveBuffIndex--;
  }
}

const char* requestTypeStr() {
  switch (requestType) {
    case RT_GET: return "GET";
    case RT_POST: return "POST";
  }
  return "UNKNOWN";
}

void clearResp() {
  receiveBuffIndex = 0;
  receiveBuff[receiveBuffIndex] = 0;
}


void appendQuoteEnt(const char* str, const char sep) {
  appendChar('"');
  appendResp(str);
  appendChar('"');
  if (sep != ' ') {
    appendChar(sep);
  }
}

void appendResp(const char* str) {
  int i = 0;
  char c = str[i];
  while (c != 0) {
    appendChar(c);
    i++;
    c = str[i];
  }
}

void appendChar(const char c)  {
  if (receiveBuffIndex <= RECEIVE_BUFF_LEN) {
    receiveBuff[receiveBuffIndex] = c;
    receiveBuffIndex++;
    receiveBuff[receiveBuffIndex] = 0;
  }
}

void resetTimeData(TimeStoreStruct &ts) {
  for (int i = 0; i < TIME_STORE_SIZE; i++) {
    ts.list[i] = TIME_STORE_UNSET;
  }
  ts.stateOn = false;
  ts.count = 0;
  strcpy(ts.onOff, "OFF");
  strcpy(ts.onOffTime, INVALID_TIME_SHORT);
  if (Serial) {
    Serial.print(ts.key);
    Serial.println(" - Data reset");
  }
}

void countTimeData(TimeStoreStruct &ts) {
  int c = 0;
  for (int i = 0; i < TIME_STORE_SIZE; i++) {
    if (ts.list[i] < TIME_STORE_UNSET) {
      c++;
    }
  }
  ts.count = c;
}

int sortDesc(const void *cmp1, const void *cmp2)
{
  uint16_t a = *((uint16_t *)cmp1);
  uint16_t b = *((uint16_t *)cmp2);
  return a > b ? 1 : (a < b ? -1 : 0);
}

void storeTimeData(TimeStoreStruct &ts) {
  qsort(ts.list, TIME_STORE_SIZE, sizeof(ts.list[0]), sortDesc);
  countTimeData(ts);
  eeprom.set(ts.key, ts.list, sizeof(ts.list) , 0);
  if (Serial) {
    Serial.print(ts.key);
    Serial.print(" ");
    Serial.print(ts.count);
    Serial.println(" - items Stored (sorted)");
  }
}

void storeIpAddress() {
  eeprom.set(IP_ADDR_STORE_KEY, ipAddressStore, sizeof(ipAddressStore) , 0);
  if (Serial) {
    Serial.print(IP_ADDR_STORE_KEY);
    Serial.print(" ");
    for (int i = 0; i < 4; i++) {
      Serial.print(ipAddressStore[i]);
      Serial.print(':');
    }
    Serial.println(" - items Stored");
  }
}


void getNextActionTime(TimeStoreStruct &ts) {
  uint16_t mow = deriveMinuteOfWeek();
  bool stOn = false;
  for (int i = 0; i < TIME_STORE_SIZE; i++) {
    if (ts.list[i] < TIME_STORE_UNSET) {
      if (ts.list[i] > mow) {
        ts.stateOn = stOn;
        if (stOn) {
          strcpy(ts.onOff, "ON ");
        } else {
          strcpy(ts.onOff, "OFF");
        }
        updateTimeBuff(ts.onOffTime, (minuteZero + ts.list[i]) * 60, false);
        return;
      }
      stOn = !stOn;
    } else {
      break;
    }
  }
  ts.stateOn = false;
  strcpy(ts.onOff, "OFF");
  strcpy(ts.onOffTime, INVALID_TIME_SHORT);
}

bool getStoredIpAddress() {
  mbed::KVStore::info_t info;
  if (eeprom.get_info(IP_ADDR_STORE_KEY, &info) != MBED_ERROR_ITEM_NOT_FOUND) {
    eeprom.get(IP_ADDR_STORE_KEY, ipAddressStore, sizeof(ipAddressStore));
    return true;
  } else {
    return false;
  }
}

bool getStoredTimeData(TimeStoreStruct &ts) {
  mbed::KVStore::info_t info;
  if (eeprom.get_info(ts.key, &info) != MBED_ERROR_ITEM_NOT_FOUND) {
    eeprom.get(ts.key, ts.list, sizeof(ts.list));
    countTimeData(ts);
    if (Serial) {
      Serial.print(ts.key);
      Serial.print(" ");
      Serial.print(ts.count);
      Serial.println(" - items Loaded");
    }
    return true;
  } else {
    if (Serial) {
      Serial.print(ts.key);
      Serial.println(" - Data Not Found");
    }
    countTimeData(ts);
    return false;
  }
}

uint16_t deriveMinuteOfWeek() {
  return (now() / 60) - minuteZero;
}

int deriveFirstMinuteOfWeek(unsigned long timeSec) {
  TimeElements te;
  breakTime(timeSec, te);
  unsigned long secondsInWeek = te.Second + (te.Minute * 60) + (te.Hour * SEC_1_HOUR) + ((te.Wday - 1) * SEC_24_HOUR);
  return (timeSec - secondsInWeek) / 60;
}

time_t deriveDST(time_t timeToday) {
  if (timeToday > UNIX_TIME_START_2021) {
    time_t dlsStart = makeTime(TimeElements{0, 0, 1, 0, 28, 3, year(timeToday) - 1970}); // 28 March
    time_t dlsEnd = makeTime(TimeElements{0, 0, 1, 0, 31, 10, year(timeToday) - 1970}); // 31 October
    if (Serial) {
      Serial.print("28 March  :");
      Serial.println(dlsStart);
      Serial.print("Now (GMT) :");
      Serial.println(timeToday);
      Serial.print("31 October:");
      Serial.println(dlsEnd);
    }
    if ((timeToday > dlsStart) && (timeToday < dlsEnd)) {
      daylightSaving = true;
      return SEC_1_HOUR;
    } else {
      daylightSaving = false;
      return 0;
    }
  } else {
    if (Serial) {
      Serial.print("Error Unix Time Today:");
      Serial.println(timeToday);
    }
    return 0;
  }
}

time_t getNtpTime() {
  displayStatus("NTP: TX Req", false, true);
  while (ethernet_UDP.parsePacket() > 0) ; // discard packets remaining to be parsed
  // send packet to request time from NTP server
  sendRequest(ntpAddress);
  displayStatus("NTP: Sent", false, true);
  delay(10);
  uint32_t beginWait = millis();
  while (millis() - beginWait < NTP_TIME_OUT_MS) {
    if (ethernet_UDP.parsePacket() >= NTP_BUFF_LEN) {
      displayStatus("NTP: Received", false, true);
      // read data and save to messageBuffer
      ethernet_UDP.read(ntpMessageBuffer, NTP_BUFF_LEN);

      // NTP time received will be the seconds elapsed since 1 January 1900
      unsigned long secsSince1900;

      // convert to an unsigned long integer the reference timestamp found at byte 40 to 43
      secsSince1900 =  (unsigned long)ntpMessageBuffer[40] << 24;
      secsSince1900 |= (unsigned long)ntpMessageBuffer[41] << 16;
      secsSince1900 |= (unsigned long)ntpMessageBuffer[42] << 8;
      secsSince1900 |= (unsigned long)ntpMessageBuffer[43];
      displayStatus("Time:", false, true);
      setSyncInterval(TIME_SYNC_INTERVAL_SEC_LONG);
      time_t t = secsSince1900 - 2208988800UL + (timeZone * SEC_24_HOUR);
      t = t + deriveDST(t);
      if (Serial) {
        Serial.print("NTP (DLS) :");
        Serial.println(t);
      }
      minuteZero = deriveFirstMinuteOfWeek(t);
      return t;
    } else {
      delay(20);
    }
  }
  setSyncInterval(TIME_SYNC_INTERVAL_SEC_SHORT);
  displayStatus("NTP Fail", true, true);
  minuteZero = 0;
  return 0;
}
/*
   helper function for getTime()
   this function sends a request packet 48 bytes long
*/
void sendRequest(IPAddress & address) {
  // set all bytes in messageBuffer to 0
  memset(ntpMessageBuffer, 0, NTP_BUFF_LEN);

  // create the NTP request message

  ntpMessageBuffer[0] = 0b11100011;  // LI, Version, Mode
  ntpMessageBuffer[1] = 0;           // Stratum, or type of clock
  ntpMessageBuffer[2] = 6;           // Polling Interval
  ntpMessageBuffer[3] = 0xEC;        // Peer Clock Precision
  // array index 4 to 11 is left unchanged - 8 bytes of zero for Root Delay & Root Dispersion
  ntpMessageBuffer[12]  = 49;
  ntpMessageBuffer[13]  = 0x4E;
  ntpMessageBuffer[14]  = 49;
  ntpMessageBuffer[15]  = 52;

  // send messageBuffer to NTP server via UDP at port 123
  ethernet_UDP.beginPacket(address, 123);
  ethernet_UDP.write(ntpMessageBuffer, NTP_BUFF_LEN);
  ethernet_UDP.endPacket();
}

void resolveNtpAddress() {
  DNSClient dns;
  dns.begin(Ethernet.dnsServerIP());
  int ret = dns.getHostByName(NTP_POOL_URL, ntpAddress);
  if (ret == 1) {
    if (Serial) {
      Serial.print("NTP [");
      Serial.print(NTP_POOL_URL);
      Serial.print("] ");
    }
  } else {
    if (Serial) {
      Serial.print("NTP [Default] ");
    }
    ntpAddress = IPAddress(NTP_IP);
  }
  if (Serial) {
    Serial.println(ntpAddress);
  }
}
