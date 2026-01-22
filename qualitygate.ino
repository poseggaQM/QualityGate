#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Adafruit_NeoPixel.h>
#include <math.h>

/* ========= CONFIG ========= */

const char* ssid = "YOUR_SSID"; // 2.4Ghz only
const char* password = "WIFI_PASSWORD";
const char* githubToken = "YOUR_TOKEN";

const char* owner = "tassilo-posegga"; // github owner
const char* repo  = "QualityGate"; // github repo

#define NEOPIXEL_PIN  18
#define NUM_PIXELS    12
#define BRIGHTNESS    80

/* ========= CI STATE ========= */

enum CIState {
  CI_STATE_LOADING = 0,
  CI_STATE_SUCCESS = 1,
  CI_STATE_FAILURE = 2,
  CI_STATE_PENDING = 3,
  CI_STATE_ERROR   = 4
};

CIState currentState  = CI_STATE_LOADING;
CIState previousState = CI_STATE_LOADING;

/* ========= TIMING ========= */

unsigned long lastAnim = 0;
unsigned long lastPoll = 0;

/* ========= NEOPIXEL ========= */

Adafruit_NeoPixel pixels(NUM_PIXELS, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);

/* ========= TRANSITION STATE ========= */

bool inTransition = false;
uint8_t transitionPixels = 1;
uint8_t transitionHead = 0;
unsigned long transitionLastStep = 0;

/* ========= SETUP ========= */

void setup() {
  Serial.begin(115200);
  delay(500);

  pixels.begin();
  pixels.setBrightness(BRIGHTNESS);
  pixels.clear();
  pixels.show();

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  Serial.println("🚦 GitHub CI Totem booting...");
}

/* ========= LOOP ========= */

void loop() {
  handleWiFi();
  animate();
  delay(20);
}

/* ========= WIFI ========= */

void handleWiFi() {
  if (WiFi.status() != WL_CONNECTED) {
    currentState = CI_STATE_LOADING;
    return;
  }

  if (millis() - lastPoll > 5000) {   // poll every 60s
    lastPoll = millis();
    getRepoStatus();
  }
}

/* ========= ANIMATION ========= */

void animate() {
  if (millis() - lastAnim < 30) return;
  lastAnim = millis();

  pixels.clear();

  if (inTransition) {
    rotatingTransition();
  } else if (currentState != previousState) {
    startTransition();
  } else {
    renderState();
  }

  pixels.show();
}

/* ========= STATE RENDER ========= */

void renderState() {
  switch (currentState) {
    case CI_STATE_LOADING: bluePulse(); break;
    case CI_STATE_SUCCESS: solidGreen(); break;
    case CI_STATE_PENDING: slowPulseOrange(); break;
    case CI_STATE_FAILURE:
    case CI_STATE_ERROR:   fastPulseRed(); break;
  }
}

/* ========= NEW TRANSITION ========= */

void startTransition() {
  inTransition = true;
  transitionPixels = 1;
  transitionHead = 0;
  transitionLastStep = millis();
}

void rotatingTransition() {
  uint32_t oldColor = stateColor(previousState);
  uint32_t newColor = stateColor(currentState);

  // Draw old background
  for (int i = 0; i < NUM_PIXELS; i++) {
    pixels.setPixelColor(i, oldColor);
  }

  // Draw rotating new-color segment
  for (int i = 0; i < transitionPixels; i++) {
    int idx = (transitionHead + i) % NUM_PIXELS;
    pixels.setPixelColor(idx, newColor);
  }

  // Rotate fast
  transitionHead = (transitionHead + 1) % NUM_PIXELS;

  // Grow segment over time
  if (millis() - transitionLastStep > 120) {
    transitionLastStep = millis();
    transitionPixels++;

    if (transitionPixels >= NUM_PIXELS) {
      inTransition = false;
      previousState = currentState;
    }
  }
}

/* ========= STATE EFFECTS ========= */

void bluePulse() {
  static float phase = 0;
  phase += 0.04;
  uint8_t b = (sin(phase) * 50) + 120;

  for (int i = 0; i < NUM_PIXELS; i++) {
    pixels.setPixelColor(i, pixels.Color(0, 0, b));
  }
}

void solidGreen() {
  for (int i = 0; i < NUM_PIXELS; i++) {
    pixels.setPixelColor(i, pixels.Color(0, 255, 0));
  }
}

void slowPulseOrange() {
  static float phase = 0;
  phase += 0.03;
  uint8_t b = (sin(phase) * 60) + 140;

  for (int i = 0; i < NUM_PIXELS; i++) {
    pixels.setPixelColor(i, pixels.Color(b, b / 2, 0));
  }
}

void fastPulseRed() {
  static float phase = 0;
  phase += 0.18;
  uint8_t b = (sin(phase) * 100) + 155;

  for (int i = 0; i < NUM_PIXELS; i++) {
    pixels.setPixelColor(i, pixels.Color(b, 0, 0));
  }
}

/* ========= COLORS ========= */

uint32_t stateColor(CIState s) {
  switch (s) {
    case CI_STATE_LOADING: return pixels.Color(0, 0, 180);
    case CI_STATE_SUCCESS: return pixels.Color(0, 255, 0);
    case CI_STATE_PENDING: return pixels.Color(255, 140, 0);
    case CI_STATE_FAILURE:
    case CI_STATE_ERROR:   return pixels.Color(255, 0, 0);
    default:               return pixels.Color(0, 0, 255);
  }
}

/* ========= GITHUB ========= */

void getRepoStatus() {
  previousState = currentState;

  WiFiClientSecure client;
  client.setInsecure();   // IMPORTANT: avoid TLS issues

  HTTPClient http;
  http.setTimeout(15000);

  String url = "https://api.github.com/repos/" +
               String(owner) + "/" +
               String(repo) +
               "/commits/main/status";

  http.begin(client, url);
  http.addHeader("Authorization", "Bearer " + String(githubToken));
  http.addHeader("Accept", "application/vnd.github+json");
  http.addHeader("User-Agent", "ESP32-CI-Totem");

  int code = http.GET();
  Serial.printf("HTTP code: %d\n", code);

  if (code == 200) {
    JsonDocument doc;
    deserializeJson(doc, http.getString());

    const char* state = doc["state"];
    Serial.print("CI State: ");
    Serial.println(state);

    if (strcmp(state, "success") == 0) currentState = CI_STATE_SUCCESS;
    else if (strcmp(state, "pending") == 0) currentState = CI_STATE_PENDING;
    else if (strcmp(state, "failure") == 0) currentState = CI_STATE_FAILURE;
    else currentState = CI_STATE_ERROR;
  } else if (code == -1) {
    Serial.println("⚠️ Network/TLS error, keeping last state");
  } else {
    currentState = CI_STATE_ERROR;
  }

  http.end();
}
