#include <Wire.h>
#include <U8g2lib.h>
#include <time.h>

#define OLED_SDA 21
#define OLED_SCL 22

#define LDR_PIN 13
#define TOUCH_PIN 4   // T0

#define BUTTON_HOUR 26
#define BUTTON_MINUTE 27

#define BUZZER_PIN 25


U8G2_SSD1306_128X64_NONAME_F_HW_I2C display(
  U8G2_R0,
  U8X8_PIN_NONE
);


// -------------------------
// Brightness
// -------------------------

int currentBrightness = 255;
int targetBrightness = 255;


// -------------------------
// Touch
// -------------------------

int touchBaseline;
int touchThreshold;

bool lastTouch = false;


// -------------------------
// MODE
// -------------------------

bool alarmInterface = false;


// -------------------------
// Eye movement
// -------------------------

int eyeOffset = 0;
int targetEyeOffset = 0;

unsigned long nextEyeMove = 0;


// -------------------------
// AUTO BLINK
// -------------------------

bool eyesClosed = false;

unsigned long blinkStart = 0;
unsigned long nextBlink = 0;

bool blinking = false;


// -------------------------
// Alarm
// -------------------------

int alarmHour = 7;
int alarmMinute = 30;

bool alarmTriggered = false;


// -------------------------
// Buttons
// -------------------------

bool lastHourButton = HIGH;
bool lastMinuteButton = HIGH;

unsigned long lastButtonPress = 0;


// =====================================================
// DRAW EYES
// =====================================================

void drawEyes(bool closed) {

  display.clearBuffer();

  if (closed) {

    // Closed eyes
    display.drawHLine(
      10, 32,
      45
    );

    display.drawHLine(
      73, 32,
      45
    );

  } else {

    // Eye whites
    display.drawRBox(
      5, 8,
      52, 48,
      12
    );

    display.drawRBox(
      71, 8,
      52, 48,
      12
    );


    // Pupils
    display.setDrawColor(0);

    display.drawDisc(
      31 + eyeOffset,
      32,
      12
    );

    display.drawDisc(
      97 + eyeOffset,
      32,
      12
    );

    display.setDrawColor(1);
  }

  display.sendBuffer();
}


// =====================================================
// EYE MOVEMENT
// =====================================================

void updateEyes() {

  if (millis() >= nextEyeMove) {

    int direction =
      random(0, 3);


    if (direction == 0) {

      targetEyeOffset = -8;

    }

    else if (direction == 1) {

      targetEyeOffset = 8;

    }

    else {

      targetEyeOffset = 0;
    }


    nextEyeMove =
      millis() + random(1000, 3000);
  }


  // Smooth pupil movement

  if (
    eyeOffset <
    targetEyeOffset
  ) {

    eyeOffset++;

  }

  else if (
    eyeOffset >
    targetEyeOffset
  ) {

    eyeOffset--;
  }
}


// =====================================================
// AUTO BLINK
// =====================================================

void setupBlink() {

  // First blink after 2-5 seconds

  nextBlink =
    millis() + random(2000, 5000);
}


void updateBlink() {

  if (blinking) {

    // Blink duration
    if (
      millis() - blinkStart >= 140
    ) {

      blinking = false;

      eyesClosed = false;

      nextBlink =
        millis() + random(2000, 5000);
    }

    return;
  }


  // Start a new blink

  if (
    millis() >= nextBlink
  ) {

    blinking = true;

    eyesClosed = true;

    blinkStart =
      millis();
  }
}


// =====================================================
// ALARM INTERFACE
// =====================================================

void drawAlarmInterface() {

  struct tm timeinfo;

  if (
    !getLocalTime(
      &timeinfo
    )
  ) {

    return;
  }


  char timeText[10];

  strftime(
    timeText,
    sizeof(timeText),
    "%H:%M",
    &timeinfo
  );


  display.clearBuffer();

  display.setDrawColor(1);


  // Current time

  display.setFont(
    u8g2_font_logisoso32_tf
  );

  display.drawStr(
    20,
    35,
    timeText
  );


  // Alarm time

  display.setFont(
    u8g2_font_6x10_tf
  );

  display.drawStr(
    18,
    48,
    "ALARM"
  );


  char alarmText[10];

  sprintf(
    alarmText,
    "%02d:%02d",
    alarmHour,
    alarmMinute
  );


  display.drawStr(
    62,
    48,
    alarmText
  );


  // Button instructions

display.drawStr(
  7,
  60,
  "Desk Buddy"
);


  display.sendBuffer();
}


// =====================================================
// SETUP CLOCK
// =====================================================

void setupClock() {

  struct tm timeinfo = {};


  /*
     SET STARTING TIME HERE

     Example:
     15:30:00 = 3:30 PM
  */

  timeinfo.tm_year =
    2026 - 1900;

  timeinfo.tm_mon =
    7;

  timeinfo.tm_mday =
    21;

  timeinfo.tm_hour =
    20;

  timeinfo.tm_min =
    45;

  timeinfo.tm_sec =
    0;


  time_t now =
    mktime(&timeinfo);


  struct timeval tv = {
    .tv_sec = now,
    .tv_usec = 0
  };


  settimeofday(
    &tv,
    NULL
  );
}


// =====================================================
// BUTTONS
// =====================================================

void checkButtons() {

  bool hourButton =
    digitalRead(
      BUTTON_HOUR
    );

  bool minuteButton =
    digitalRead(
      BUTTON_MINUTE
    );


  // Hour

  if (
    lastHourButton == HIGH &&
    hourButton == LOW &&
    millis() -
    lastButtonPress > 150
  ) {

    alarmHour++;

    if (
      alarmHour > 23
    ) {

      alarmHour = 0;
    }


    Serial.print(
      "Alarm hour: "
    );

    Serial.println(
      alarmHour
    );


    lastButtonPress =
      millis();
  }


  // Minute

  if (
    lastMinuteButton == HIGH &&
    minuteButton == LOW &&
    millis() -
    lastButtonPress > 150
  ) {

    alarmMinute++;

    if (
      alarmMinute > 59
    ) {

      alarmMinute = 0;
    }


    Serial.print(
      "Alarm minute: "
    );

    Serial.println(
      alarmMinute
    );


    lastButtonPress =
      millis();
  }


  lastHourButton =
    hourButton;

  lastMinuteButton =
    minuteButton;
}


// =====================================================
// TOUCH = MODE SWITCH
// =====================================================

void checkTouch() {

  int touchValue =
    touchRead(
      TOUCH_PIN
    );


  bool touched =
    touchValue <
    touchThreshold;


  // New touch

  if (
    touched &&
    !lastTouch
  ) {

    alarmInterface =
      !alarmInterface;


    Serial.print(
      "Alarm interface: "
    );

    Serial.println(
      alarmInterface
        ? "ON"
        : "OFF"
    );


    // Reset blinking
    blinking = false;

    eyesClosed = false;
  }


  lastTouch =
    touched;
}


// =====================================================
// ALARM
// =====================================================

void triggerAlarm() {

  Serial.println(
    "ALARM!"
  );


  unsigned long start =
    millis();


  while (
    millis() -
    start < 30000
  ) {

    // BEEP

    tone(
      BUZZER_PIN,
      2000
    );


    // Alarm screen

    display.clearBuffer();

    display.setDrawColor(1);

    display.setFont(
      u8g2_font_logisoso32_tf
    );

    display.drawStr(
      15,
      38,
      "ALARM!"
    );

    display.display();


    delay(250);


    noTone(
      BUZZER_PIN
    );


    delay(150);


    // Press either button to stop

    if (
      digitalRead(
        BUTTON_HOUR
      ) == LOW ||

      digitalRead(
        BUTTON_MINUTE
      ) == LOW
    ) {

      break;
    }
  }


  noTone(
    BUZZER_PIN
  );
}


// =====================================================
// CHECK ALARM
// =====================================================

void checkAlarm() {

  struct tm timeinfo;


  if (
    !getLocalTime(
      &timeinfo
    )
  ) {

    return;
  }


  int hour =
    timeinfo.tm_hour;

  int minute =
    timeinfo.tm_min;


  if (
    hour == alarmHour &&
    minute == alarmMinute &&
    !alarmTriggered
  ) {

    alarmTriggered =
      true;

    triggerAlarm();
  }


  // Reset after minute changes

  if (
    minute != alarmMinute
  ) {

    alarmTriggered =
      false;
  }
}


// =====================================================
// SETUP
// =====================================================

void setup() {

  Serial.begin(
    115200
  );


  // OLED

  Wire.begin(
    OLED_SDA,
    OLED_SCL
  );


  display.begin();


  display.setContrast(
    255
  );


  // Buttons

  pinMode(
    BUTTON_HOUR,
    INPUT_PULLUP
  );

  pinMode(
    BUTTON_MINUTE,
    INPUT_PULLUP
  );


  // Buzzer

  pinMode(
    BUZZER_PIN,
    OUTPUT
  );


  // Random

  randomSeed(
    micros()
  );


  // Touch calibration

  delay(500);


  long total = 0;


  for (
    int i = 0;
    i < 20;
    i++
  ) {

    total +=
      touchRead(
        TOUCH_PIN
      );

    delay(20);
  }


  touchBaseline =
    total / 20;


  touchThreshold =
    touchBaseline * 0.7;


  Serial.print(
    "Touch baseline: "
  );

  Serial.println(
    touchBaseline
  );


  Serial.print(
    "Touch threshold: "
  );

  Serial.println(
    touchThreshold
  );


  // Clock

  setupClock();


  // Eye movement

  nextEyeMove =
    millis() + 2000;


  // Blink

  setupBlink();


  // Start in face mode

  drawEyes(false);
}


// =====================================================
// LOOP
// =====================================================

void loop() {

  // ===================================================
  // LDR
  // ===================================================

  int lightState =
    digitalRead(
      LDR_PIN
    );


  /*
     YOUR WORKING LOGIC:

     HIGH = DIM
     LOW  = BRIGHT
  */

  if (
    lightState == HIGH
  ) {

    targetBrightness =
      20;

  } else {

    targetBrightness =
      255;
  }


  // Smooth brightness

  if (
    currentBrightness <
    targetBrightness
  ) {

    currentBrightness += 2;

    if (
      currentBrightness >
      targetBrightness
    ) {

      currentBrightness =
        targetBrightness;
    }
  }


  if (
    currentBrightness >
    targetBrightness
  ) {

    currentBrightness -= 2;

    if (
      currentBrightness <
      targetBrightness
    ) {

      currentBrightness =
        targetBrightness;
    }
  }


  currentBrightness =
    constrain(
      currentBrightness,
      5,
      255
    );


  display.setContrast(
    currentBrightness
  );


  // ===================================================
  // TOUCH
  // ===================================================

  checkTouch();


  // ===================================================
  // BUTTONS
  // ===================================================

  checkButtons();


  // ===================================================
  // ALARM
  // ===================================================

  checkAlarm();


  // ===================================================
  // DISPLAY
  // ===================================================

  if (
    alarmInterface
  ) {

    // Alarm screen
    drawAlarmInterface();

  } else {

    // Face mode

    updateEyes();

    updateBlink();

    drawEyes(
      eyesClosed
    );
  }


  delay(20);
}