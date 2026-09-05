#include <Wire.h>
#include <MPU6050.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>

// ================== WiFi & Telegram ==================
const char* ssid = "*****************";
const char* password = "*************";

#define BOT_TOKEN "***********"
#define CHAT_ID   "***********"

WiFiClientSecure client;
UniversalTelegramBot bot(BOT_TOKEN, client);

// ================== Hardware ==================
MPU6050 mpu;

#define VIBRATOR_PIN 25
#define BUZZER_PIN   26

// TFmini UART
HardwareSerial TFmini(2);

// Variables (sensor task, core 1 / loop())
volatile int distance = 0;
bool fallActive = false;
unsigned long fallStartTime = 0;
const unsigned long BUZZER_DURATION = 5000;   // 5 seconds

unsigned long lastSensorTime = 0;

// Fall Detection Settings
float IMPACT_THRESHOLD = 2.10;
float ALPHA = 0.85;
float prevMagF = 1.0;

// ================== Predictive (Pre-Impact) Fall Detection ==================
// A fall has a brief free-fall phase (near-weightlessness, magF drops close to 0g)
// AND/OR a rapid rotational loss-of-balance spike (gyro) that both precede the
// impact spike by ~200-500ms. Catching either gives lead time before impact,
// not just a reaction after it.
enum FallState { FALL_NORMAL, FALL_PRE, FALL_CONFIRMED };
FallState fallState = FALL_NORMAL;

unsigned long preFallStartTime = 0;
const unsigned long PRE_FALL_WINDOW = 800;   // ms: must see impact within this window or reset
const float FREEFALL_THRESHOLD   = 0.50;     // g: near-weightlessness onset
const float GYRO_SPIKE_THRESHOLD = 200.0;    // deg/s: sudden rotational instability

// Short haptic/audio pre-warning pulse on PRE_FALL onset (non-blocking)
bool preWarnActive = false;
unsigned long preWarnStartTime = 0;
const unsigned long PRE_WARN_DURATION = 150; // ms buzzer pulse, distinct from 5s confirmed-fall buzz

// ================== Async Telegram Queue ==================
// Sending over HTTPS blocks for 100s of ms; runs on its own task/core
// so sensor polling (20ms cadence) and vibration control never stall.
#define TG_QUEUE_LEN   8
#define TG_MSG_MAXLEN  160

QueueHandle_t tgQueue;
TaskHandle_t  tgTaskHandle;

struct TgMessage {
  char text[TG_MSG_MAXLEN];
};

void enqueueTelegram(const String &msg) {
  TgMessage m;
  msg.toCharArray(m.text, TG_MSG_MAXLEN);
  // Non-blocking: if queue is full, drop oldest rather than stall the sensor loop
  if (xQueueSend(tgQueue, &m, 0) != pdPASS) {
    TgMessage discard;
    xQueueReceive(tgQueue, &discard, 0);
    xQueueSend(tgQueue, &m, 0);
  }
}

// Runs on core 0, independent of sensor loop on core 1
void telegramTask(void *param) {
  TgMessage m;
  for (;;) {
    if (xQueueReceive(tgQueue, &m, portMAX_DELAY) == pdPASS) {
      if (WiFi.status() == WL_CONNECTED) {
        bot.sendMessage(CHAT_ID, m.text, "");
      }
    }
  }
}

// ================== Async WiFi Reconnect ==================
unsigned long lastWifiAttempt = 0;
const unsigned long WIFI_RETRY_INTERVAL = 5000;

void maintainWifi(unsigned long now) {
  if (WiFi.status() != WL_CONNECTED && now - lastWifiAttempt >= WIFI_RETRY_INTERVAL) {
    lastWifiAttempt = now;
    WiFi.disconnect();
    WiFi.begin(ssid, password);
    Serial.println("WiFi reconnect attempt...");
  }
}

void setup() {
  Serial.begin(115200);
  TFmini.begin(115200, SERIAL_8N1, 16, 17);

  Wire.begin();
  mpu.initialize();

  pinMode(VIBRATOR_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  ledcAttach(VIBRATOR_PIN, 5000, 8);   // ESP32 core 3.x: pin-based LEDC, no channel/ledcSetup

  WiFi.begin(ssid, password);
  Serial.print("Connecting WiFi");
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 10000) {
    delay(300);
    Serial.print(".");
  }
  Serial.println(WiFi.status() == WL_CONNECTED ? "\nWiFi Connected" : "\nWiFi timeout, will retry in background");
  client.setInsecure();

  tgQueue = xQueueCreate(TG_QUEUE_LEN, sizeof(TgMessage));
  xTaskCreatePinnedToCore(
    telegramTask,
    "TelegramTask",
    8192,
    NULL,
    1,
    &tgTaskHandle,
    0   // pin to core 0, sensor loop stays on core 1
  );

  enqueueTelegram("✅ Smart Glasses Ready\nFall + Close alerts active");

  Serial.println("Smart Glasses Started");
}

void loop() {
  unsigned long now = millis();

  maintainWifi(now);

  if (now - lastSensorTime >= 20) {
    lastSensorTime = now;

    readTFmini();
    controlVibration(distance);
    checkFall(now);
    handleAlerts(now);
  }

  static unsigned long lastPrint = 0;
  if (now - lastPrint > 80) {
    Serial.print("D:");
    Serial.print(distance);
    Serial.println(" cm");
    lastPrint = now;
  }
}

// Read TFmini
void readTFmini() {
  while (TFmini.available() >= 9) {
    if (TFmini.read() == 0x59 && TFmini.read() == 0x59) {
      uint8_t data[7];
      for (int i = 0; i < 7; i++) data[i] = TFmini.read();
      int dist = data[0] + data[1] * 256;
      int strength = data[4] + data[5] * 256;
      if (dist > 0 && dist < 2000 && strength > 70) {
        distance = dist;
      }
    }
  }
}

// Vibration
void controlVibration(int dist) {
  int intensity = 0;
  if (dist > 0 && dist < 200) {
    intensity = map(dist, 200, 20, 0, 255);
    intensity = constrain(intensity, 0, 255);
  }
  ledcWrite(VIBRATOR_PIN, intensity);
}

// Predictive Fall Detection: PRE_FALL (free-fall/gyro spike) -> CONFIRMED (impact) -> 5s buzzer
void checkFall(unsigned long now) {
  int16_t ax, ay, az, gx, gy, gz;
  mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);

  float axg = ax / 16384.0;
  float ayg = ay / 16384.0;
  float azg = az / 16384.0;

  float mag = sqrt(axg*axg + ayg*ayg + azg*azg);
  float magF = ALPHA * prevMagF + (1 - ALPHA) * mag;
  prevMagF = magF;

  // Default MPU6050 gyro sensitivity: 131 LSB per deg/s (±250 dps range)
  float gxr = gx / 131.0;
  float gyr = gy / 131.0;
  float gzr = gz / 131.0;
  float gyroMag = sqrt(gxr*gxr + gyr*gyr + gzr*gzr);

  switch (fallState) {

    case FALL_NORMAL:
      if (magF < FREEFALL_THRESHOLD || gyroMag > GYRO_SPIKE_THRESHOLD) {
        fallState = FALL_PRE;
        preFallStartTime = now;

        // Non-blocking short pre-warning pulse
        preWarnActive = true;
        preWarnStartTime = now;
        digitalWrite(BUZZER_PIN, HIGH);

        Serial.println(">>> PRE-FALL onset (free-fall/gyro spike) - monitoring for impact <<<");
      }
      break;

    case FALL_PRE:
      // Impact confirms the fall predicted by the pre-fall trigger
      if (magF >= IMPACT_THRESHOLD) {
        unsigned long leadTime = now - preFallStartTime;
        fallState = FALL_CONFIRMED;
        fallActive = true;
        fallStartTime = now;
        preWarnActive = false;
        digitalWrite(BUZZER_PIN, HIGH);

        String msg = "🚨 FALL DETECTED in Smart Glasses!\n"
                     "Pre-fall onset -> impact confirmed\n"
                     "Predictive lead time: " + String(leadTime) + " ms\n"
                     "Buzzer running for 5 seconds";
        enqueueTelegram(msg);
        Serial.println(">>> FALL CONFIRMED, lead time " + String(leadTime) + "ms + Telegram queued <<<");
      }
      // No impact followed within the window -> false alarm (e.g. sat down fast, jumped)
      else if (now - preFallStartTime > PRE_FALL_WINDOW) {
        fallState = FALL_NORMAL;
        preWarnActive = false;
        Serial.println(">>> PRE-FALL cleared, no impact - false alarm <<<");
      }
      break;

    case FALL_CONFIRMED:
      // Auto turn OFF buzzer after 5 seconds, then reset state machine
      if (now - fallStartTime >= BUZZER_DURATION) {
        fallActive = false;
        fallState = FALL_NORMAL;
        digitalWrite(BUZZER_PIN, LOW);
        Serial.println("Buzzer OFF after 5 seconds");
      }
      break;
  }

  // End the short pre-warning pulse without blocking (only matters while still in FALL_PRE)
  if (preWarnActive && (now - preWarnStartTime >= PRE_WARN_DURATION)) {
    preWarnActive = false;
    if (fallState == FALL_PRE) {
      digitalWrite(BUZZER_PIN, LOW);
    }
  }
}

// Close Obstacle Alert (Instant, no cooldown)
void handleAlerts(unsigned long now) {
  static bool obstacleAlertSent = false;

  if (distance > 0 && distance < 25) {
    if (!obstacleAlertSent) {
      String msg = "🚨 VERY CLOSE OBSTACLE!\n"
                   "Distance: " + String(distance) + " cm";
      enqueueTelegram(msg);
      Serial.println("Telegram queued: Close obstacle - " + String(distance) + "cm");
      obstacleAlertSent = true;
    }
  } else {
    obstacleAlertSent = false;   // Reset when object moves away
  }
}
