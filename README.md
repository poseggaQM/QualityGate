# QualityGate

A **physical GitHub build-status indicator** powered by an **ESP32**, an **RGB NeoPixel ring**, and a **3D‑printed enclosure**.

The gate connects to Wi‑Fi, polls the GitHub API for a repository’s CI status, and visualizes it using expressive LED animations:

* 🟢 **Success** → solid green
* 🟠 **Pending** → slow orange pulse
* 🔴 **Failure / Error** → fast red pulse
* 🔵 **Boot / Wi‑Fi connecting** → calm blue breathing
* 🔄 **State changes** → rotating color takeover animation

This project was built as a **hackathon demo**, optimized for fast setup, strong visual impact, and easy replication.

---

## ✨ Demo Video / Photos

https://drive.google.com/file/d/1cwE_JaRTjXnJE8KFs2QpC7TMxyx5t9Rr/view?usp=drive_link

---

## 🧠 How It Works

1. ESP32 boots and connects to Wi‑Fi
2. While connecting → blue pulsating animation
3. ESP32 polls the GitHub REST API:

   ```
   GET /repos/{owner}/{repo}/commits/main/status
   ```
4. The returned CI state (`success`, `pending`, `failure`, `error`) is mapped to LED animations
5. When the state changes, a **rotating LED transition** visually “takes over” the ring

The device updates every **60 seconds** to avoid GitHub rate limits.

---

## 🧰 Hardware Requirements

| Component        | Notes                       |
| ---------------- | --------------------------- |
| ESP32 (WROOM‑32) | Any ESP32 with Wi‑Fi works  |
| NeoPixel Ring    | 12‑LED RGB ring recommended |
| 5V Power         | USB or external supply      |
| 3D‑printed case  | Optional but awesome        |

**Wiring**:

* NeoPixel **DIN** → ESP32 GPIO **18**
* NeoPixel **5V** → 5V
* NeoPixel **GND** → GND

---

## 💻 Software Requirements

* Arduino IDE (2.x recommended)
* ESP32 Arduino Core (≥ 3.3.x)
* Libraries:

  * `WiFi`
  * `HTTPClient`
  * `WiFiClientSecure`
  * `ArduinoJson`
  * `Adafruit NeoPixel`

---

## ⚙️ Setup Instructions

### 1️⃣ Open the Sketch

This repository already contains the Arduino sketch.

Open the `.ino` file in **Arduino IDE**.

### 2️⃣ Configure Credentials

Edit the following in the sketch:

```cpp
const char* ssid = "YOUR_WIFI";
const char* password = "YOUR_PASSWORD";
const char* githubToken = "YOUR_GITHUB_PAT";

const char* owner = "your-github-username";
const char* repo  = "this-repository-name";
```

🔐 **GitHub Token**:

* Create a **Personal Access Token**
* Scope needed: `repo:status` (or classic `repo`)

### 3️⃣ Select Board & Upload

* Board: **ESP32 Dev Module** (or equivalent)
* Port: your ESP32 serial port

Upload the sketch and open the Serial Monitor (115200 baud).

---

## 🧪 Mocking CI Status with GitHub Actions

This repository already contains a **mock CI GitHub Action** that you can use to drive the gate animations.

The ESP32 polls the status of the **`main` branch**, so every workflow run in this repo directly affects the LED ring.

### How it works

The workflow simulates a CI pipeline:

* While the job is running → GitHub reports `pending`
* If the job exits with `0` → `success`
* If the job exits with `1` → `failure`

The gate reflects this in real time.

### Triggering the mock CI

1. Go to **GitHub → Actions** in this repository
2. Select the **QualityGate Status** workflow
3. Click **Run workflow** (manual trigger)
5. Watch the gate change:

   * 🟠 Orange pulse while running
   * 🟢 Green on success
   * 🔴 Red on failure
