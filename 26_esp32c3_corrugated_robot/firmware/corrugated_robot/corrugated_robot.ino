/*
 * 瓦楞机器人 · ESP32-C3
 * ------------------------------------------------------------
 * 硬件：
 *   - ESP32-C3 开发板（SuperMini / DevKitM-1 均可）
 *   - 2 × SG90 / MG90S 舵机：一个水平转头，一个上下点头
 *   - 1 × TTP223 触摸模块（摸一下换表情 + 上下点头）
 *   - 2 × 红外避障模块（摸左边 / 右边 → 左右摇头）
 *   - 1 × 0.96" SSD1306 OLED（128×64，I2C）
 *
 * 依赖库（Arduino IDE → 库管理器）：
 *   - Adafruit SSD1306
 *   - Adafruit GFX Library
 *   - ESP32Servo（作者 Kevin Harrington）
 *
 * 开发板选择：ESP32C3 Dev Module
 * 分区方案：默认 4MB with spiffs 即可（本程序很小）
 *
 * 接线、供电和调试见项目 README。
 */

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ESP32Servo.h>
#include <math.h>

// ============================================================
// 1. 引脚 / 硬件参数：换板子时主要改这里
// ============================================================

// OLED I2C（ESP32-C3 的 4/5 不占用 USB，也不影响启动）
constexpr uint8_t PIN_I2C_SDA = 4;
constexpr uint8_t PIN_I2C_SCL = 5;

// 舵机信号
constexpr uint8_t PIN_SERVO_PAN  = 6;   // 水平：左右摇头
constexpr uint8_t PIN_SERVO_TILT = 7;   // 垂直：上下点头

// 输入
constexpr uint8_t PIN_TOUCH    = 3;     // TTP223 OUT
constexpr uint8_t PIN_IR_LEFT  = 0;     // 左侧红外 DO
constexpr uint8_t PIN_IR_RIGHT = 1;     // 右侧红外 DO

// 模块极性：买到的模块不一样时，只改这两个开关
constexpr bool TOUCH_ACTIVE_HIGH = true;   // TTP223 通常是“摸住为高”
constexpr bool IR_ACTIVE_LOW     = true;   // TCRT5000 通常是“有遮挡为低”

constexpr uint8_t OLED_ADDR = 0x3C;        // 有些模块是 0x3D，代码会自动再试
constexpr uint8_t OLED_ROTATION = 0;       // 屏幕装反了就改成 2

// ============================================================
// 2. 舵机标定：装好后先看这一节
// ============================================================

// 水平舵机（左右）
constexpr int PAN_CENTER = 90;       // 机械中位
constexpr int PAN_SWING  = 28;       // 左右各摆多少度
constexpr int PAN_MIN_ANGLE = 35;    // 机械安全范围，防止撞限位
constexpr int PAN_MAX_ANGLE = 145;
constexpr int PAN_DIR = 1;           // 左右反了改成 -1

// 垂直舵机（上下）
constexpr int TILT_CENTER = 90;
constexpr int TILT_SWING  = 20;
constexpr int TILT_MIN_ANGLE = 55;
constexpr int TILT_MAX_ANGLE = 125;
constexpr int TILT_DIR = 1;          // 点头方向反了改成 -1

// 动作节奏（毫秒）
constexpr uint16_t NOD_STEP_MS   = 180;   // 每点一下用时
constexpr uint16_t SHAKE_STEP_MS = 190;   // 每摇一下用时
constexpr uint16_t SERVO_MIN_US  = 500;   // SG90 常用 500~2400
constexpr uint16_t SERVO_MAX_US  = 2400;  // 抖动大可以改成 1000~2000

// ============================================================
// 3. 屏幕 / 表情参数
// ============================================================

constexpr uint8_t SCREEN_WIDTH  = 128;
constexpr uint8_t SCREEN_HEIGHT = 64;

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

enum Face : uint8_t {
  FACE_HAPPY = 0,
  FACE_CURIOUS,
  FACE_SURPRISED,
  FACE_LOVE,
  FACE_SLEEPY,
  FACE_ANGRY,
  FACE_COUNT
};

const char *const FACE_NAMES[] = {
  "HAPPY", "CURIOUS", "SURPRISED", "LOVE", "SLEEPY", "ANGRY"
};
static_assert(FACE_COUNT == sizeof(FACE_NAMES) / sizeof(FACE_NAMES[0]),
              "FACE_NAMES 与 Face 枚举数量不一致");

Face currentFace = FACE_HAPPY;
bool oledReady = false;
uint8_t activeOledAddr = OLED_ADDR;

// 眨眼 / 切表情时的闭眼
bool eyesClosed = false;
uint32_t eyesClosedUntil = 0;
uint32_t nextBlinkAt = 0;
constexpr uint16_t BLINK_MS = 110;
constexpr uint16_t FACE_SWITCH_BLINK_MS = 130;
constexpr uint16_t BLINK_MIN_MS = 2600;
constexpr uint16_t BLINK_MAX_MS = 5200;

// ============================================================
// 4. 输入消抖
// ============================================================

constexpr uint16_t DEBOUNCE_MS = 30;
constexpr uint32_t INPUT_ARM_MS = 700;  // 上电后先等模块稳定，期间不响应

struct DebouncedInput {
  uint8_t pin;
  bool activeHigh;
  bool stable;
  bool lastRaw;
  uint32_t changedAt;

  void begin(uint8_t p, bool high) {
    pin = p;
    activeHigh = high;
    pinMode(pin, activeHigh ? INPUT_PULLDOWN : INPUT_PULLUP);
    lastRaw = readRaw();
    stable = lastRaw;
    changedAt = millis();
  }

  bool readRaw() const {
    int level = digitalRead(pin);
    return activeHigh ? (level == HIGH) : (level == LOW);
  }

  // 只在“从未按下变成按下”的消抖完成后返回 true
  bool pressedEvent() {
    bool raw = readRaw();
    if (raw != lastRaw) {
      lastRaw = raw;
      changedAt = millis();
    }
    if ((millis() - changedAt) >= DEBOUNCE_MS && stable != raw) {
      stable = raw;
      return stable;
    }
    return false;
  }
};

DebouncedInput touchInput{PIN_TOUCH, TOUCH_ACTIVE_HIGH, false, false, 0};
DebouncedInput irLeft{PIN_IR_LEFT, !IR_ACTIVE_LOW, false, false, 0};
DebouncedInput irRight{PIN_IR_RIGHT, !IR_ACTIVE_LOW, false, false, 0};

uint32_t inputArmAt = 0;

bool inputsArmed() {
  return (int32_t)(millis() - inputArmAt) >= 0;
}

// ============================================================
// 5. 非阻塞舵机运动
// ============================================================

struct ServoAxis {
  Servo servo;
  float currentDeg;
  float fromDeg;
  float toDeg;
  uint32_t startedAt;
  uint16_t durationMs;

  void begin(uint8_t pin, int centerDeg) {
    servo.setPeriodHertz(50);
    servo.attach(pin, SERVO_MIN_US, SERVO_MAX_US);
    currentDeg = centerDeg;
    fromDeg = centerDeg;
    toDeg = centerDeg;
    durationMs = 0;
    servo.write(centerDeg);
  }

  void moveTo(int targetDeg, uint16_t ms) {
    fromDeg = currentDeg;
    toDeg = targetDeg;
    startedAt = millis();
    durationMs = (ms == 0) ? 1 : ms;
  }

  // 必须每轮 loop 调用；用 smoothstep 让起停柔和一点
  void update() {
    if (durationMs == 0) return;

    uint32_t elapsed = millis() - startedAt;
    if (elapsed >= durationMs) {
      currentDeg = toDeg;
      durationMs = 0;
    } else {
      float t = (float)elapsed / (float)durationMs;
      t = t * t * (3.0f - 2.0f * t);
      currentDeg = fromDeg + (toDeg - fromDeg) * t;
    }
    servo.write((int)(currentDeg + 0.5f));
  }

  bool busy() const {
    return durationMs > 0;
  }
};

ServoAxis panAxis;
ServoAxis tiltAxis;

int panAngle(int offset) {
  int angle = PAN_CENTER + PAN_DIR * offset;
  return constrain(angle, PAN_MIN_ANGLE, PAN_MAX_ANGLE);
}

int tiltAngle(int offset) {
  int angle = TILT_CENTER + TILT_DIR * offset;
  return constrain(angle, TILT_MIN_ANGLE, TILT_MAX_ANGLE);
}

enum Motion : uint8_t {
  MOTION_IDLE = 0,
  MOTION_NOD,
  MOTION_SHAKE
};

Motion motion = MOTION_IDLE;
uint8_t motionStep = 0;
int8_t shakeFirstSide = 1;

bool pendingNod = false;
int8_t pendingShakeSide = 0;   // 0 没有，-1 左，+1 右

void startNod() {
  motion = MOTION_NOD;
  motionStep = 0;
  // 第一次先抬头，再低头，共两个来回
  tiltAxis.moveTo(tiltAngle(-TILT_SWING), NOD_STEP_MS);
  Serial.println("[MOTION] nod");
}

void startShake(int8_t side) {
  motion = MOTION_SHAKE;
  motionStep = 0;
  shakeFirstSide = (side >= 0) ? 1 : -1;
  panAxis.moveTo(panAngle(shakeFirstSide * PAN_SWING), SHAKE_STEP_MS);
  Serial.printf("[MOTION] shake, first=%s\n", shakeFirstSide > 0 ? "right" : "left");
}

void finishMotion() {
  motion = MOTION_IDLE;
  motionStep = 0;
  Serial.println("[MOTION] done");
}

// 每一步只在前一步到位后启动，不用 delay()
void serviceMotion() {
  if (motion == MOTION_IDLE) return;
  if (panAxis.busy() || tiltAxis.busy()) return;

  motionStep++;

  if (motion == MOTION_NOD) {
    switch (motionStep) {
      case 1: tiltAxis.moveTo(tiltAngle(TILT_SWING), NOD_STEP_MS); break;
      case 2: tiltAxis.moveTo(tiltAngle(-TILT_SWING), NOD_STEP_MS); break;
      case 3: tiltAxis.moveTo(tiltAngle(TILT_SWING), NOD_STEP_MS); break;
      case 4: tiltAxis.moveTo(tiltAngle(0), NOD_STEP_MS); break;
      default: finishMotion(); break;
    }
  } else if (motion == MOTION_SHAKE) {
    switch (motionStep) {
      case 1: panAxis.moveTo(panAngle(-shakeFirstSide * PAN_SWING), SHAKE_STEP_MS); break;
      case 2: panAxis.moveTo(panAngle(shakeFirstSide * PAN_SWING), SHAKE_STEP_MS); break;
      case 3: panAxis.moveTo(panAngle(-shakeFirstSide * PAN_SWING), SHAKE_STEP_MS); break;
      case 4: panAxis.moveTo(panAngle(0), SHAKE_STEP_MS); break;
      default: finishMotion(); break;
    }
  }
}

// 动作期间又摸了一下：排队，等当前动作结束再做
void serviceQueue() {
  if (motion != MOTION_IDLE) return;

  if (pendingNod) {
    pendingNod = false;
    startNod();
    return;
  }
  if (pendingShakeSide != 0) {
    int8_t side = pendingShakeSide;
    pendingShakeSide = 0;
    startShake(side);
  }
}

// ============================================================
// 6. 画表情：全部用 GFX 图形现场画，不占 Flash 图片空间
// ============================================================

void drawArcThick(int cx, int cy, int radius, int startDeg, int endDeg,
                  uint8_t thickness, uint16_t color) {
  for (int deg = startDeg; deg <= endDeg; deg += 3) {
    float rad = deg * 0.0174532925f;
    int x = cx + (int)lroundf(cosf(rad) * radius);
    int y = cy + (int)lroundf(sinf(rad) * radius);
    display.fillCircle(x, y, thickness / 2, color);
  }
}

void drawThickLine(int x0, int y0, int x1, int y1, uint8_t thickness,
                   uint16_t color) {
  int half = thickness / 2;
  for (int i = -half; i <= half; i++) {
    display.drawLine(x0, y0 + i, x1, y1 + i, color);
  }
}

void drawHeart(int cx, int cy, int size, uint16_t color) {
  int r = size / 4;
  display.fillCircle(cx - r, cy - r / 2, r, color);
  display.fillCircle(cx + r, cy - r / 2, r, color);
  display.fillTriangle(cx - size / 2, cy - r / 2 + 1,
                       cx + size / 2, cy - r / 2 + 1,
                       cx, cy + size / 2, color);
}

void drawOpenEye(int cx, int cy, int w, int h) {
  display.fillRoundRect(cx - w / 2, cy - h / 2, w, h, 6, SSD1306_WHITE);
  // 黑色瞳孔 + 一个白色高光，眼睛立刻有神
  display.fillRoundRect(cx - 4, cy - 5, 9, 13, 4, SSD1306_BLACK);
  display.fillRect(cx - 2, cy - 3, 3, 3, SSD1306_WHITE);
}

void drawClosedEyes() {
  display.fillRoundRect(33, 21, 18, 4, 2, SSD1306_WHITE);
  display.fillRoundRect(77, 21, 18, 4, 2, SSD1306_WHITE);
}

void drawHappyMouth() {
  drawArcThick(64, 37, 17, 20, 160, 4, SSD1306_WHITE);
}

void drawCuriousMouth() {
  drawArcThick(64, 43, 10, 25, 135, 3, SSD1306_WHITE);
}

void drawSurprisedMouth() {
  drawArcThick(64, 49, 7, 0, 360, 3, SSD1306_WHITE);
  display.fillCircle(64, 49, 2, SSD1306_WHITE);
}

void drawSleepyMouth() {
  drawArcThick(64, 50, 5, 0, 360, 3, SSD1306_WHITE);
}

void drawLoveMouth() {
  drawArcThick(64, 39, 13, 25, 155, 4, SSD1306_WHITE);
}

void drawAngryMouth() {
  drawArcThick(64, 53, 14, 200, 340, 4, SSD1306_WHITE);
}

void drawZ(int x, int y, uint8_t size) {
  int w = 5 * size;
  int h = 6 * size;
  drawThickLine(x, y, x + w, y, size, SSD1306_WHITE);
  drawThickLine(x + w, y, x, y + h, size, SSD1306_WHITE);
  drawThickLine(x, y + h, x + w, y + h, size, SSD1306_WHITE);
}

void drawFaceContent() {
  // 切表情/眨眼时先闭眼；困了表情本身就不需要再眨
  if (eyesClosed && currentFace != FACE_SLEEPY) {
    drawClosedEyes();
  } else {
    switch (currentFace) {
      case FACE_HAPPY:
        drawArcThick(42, 27, 10, 200, 340, 5, SSD1306_WHITE);
        drawArcThick(86, 27, 10, 200, 340, 5, SSD1306_WHITE);
        break;

      case FACE_CURIOUS:
        drawOpenEye(42, 25, 20, 25);
        drawOpenEye(87, 24, 17, 21);
        break;

      case FACE_SURPRISED:
        for (uint8_t i = 0; i < 2; i++) {
          int cx = (i == 0) ? 42 : 86;
          drawArcThick(cx, 24, 11, 0, 360, 3, SSD1306_WHITE);
          display.fillCircle(cx, 25, 4, SSD1306_WHITE);
          display.fillCircle(cx - 1, 24, 1, SSD1306_WHITE);
        }
        break;

      case FACE_LOVE:
        drawHeart(42, 24, 20, SSD1306_WHITE);
        drawHeart(86, 24, 20, SSD1306_WHITE);
        break;

      case FACE_SLEEPY:
        drawArcThick(42, 22, 10, 25, 155, 4, SSD1306_WHITE);
        drawArcThick(86, 22, 10, 25, 155, 4, SSD1306_WHITE);
        drawZ(100, 8, 2);
        drawZ(111, 2, 2);
        break;

      case FACE_ANGRY:
        drawThickLine(30, 15, 52, 21, 4, SSD1306_WHITE);
        drawThickLine(98, 15, 76, 21, 4, SSD1306_WHITE);
        for (uint8_t i = 0; i < 2; i++) {
          int cx = (i == 0) ? 42 : 86;
          display.fillRoundRect(cx - 8, 25, 16, 15, 4, SSD1306_WHITE);
          display.fillRoundRect(cx - 3, 29, 6, 7, 2, SSD1306_BLACK);
        }
        break;

      default:
        break;
    }
  }

  switch (currentFace) {
    case FACE_HAPPY:
      drawHappyMouth();
      display.fillRect(23, 44, 9, 2, SSD1306_WHITE);
      display.fillRect(97, 44, 9, 2, SSD1306_WHITE);
      break;
    case FACE_CURIOUS:  drawCuriousMouth();  break;
    case FACE_SURPRISED: drawSurprisedMouth(); break;
    case FACE_LOVE:     drawLoveMouth();     break;
    case FACE_SLEEPY:   drawSleepyMouth();   break;
    case FACE_ANGRY:    drawAngryMouth();    break;
    default: break;
  }
}

void drawFace() {
  if (!oledReady) return;
  display.clearDisplay();
  drawFaceContent();
  display.display();
}

void cycleFace() {
  currentFace = (Face)((currentFace + 1) % FACE_COUNT);
  Serial.printf("[TOUCH] face -> %s\n", FACE_NAMES[currentFace]);

  // 先闭眼 130ms 再睁开，像屏幕“刷新”了一下
  eyesClosed = true;
  eyesClosedUntil = millis() + FACE_SWITCH_BLINK_MS;
  drawFace();
  nextBlinkAt = millis() + random(BLINK_MIN_MS, BLINK_MAX_MS);
}

void updateEyes() {
  uint32_t now = millis();

  if (eyesClosed) {
    if ((int32_t)(now - eyesClosedUntil) >= 0) {
      eyesClosed = false;
      drawFace();
      nextBlinkAt = now + random(BLINK_MIN_MS, BLINK_MAX_MS);
    }
    return;
  }

  if (currentFace == FACE_SLEEPY) return;
  if ((int32_t)(now - nextBlinkAt) >= 0) {
    eyesClosed = true;
    eyesClosedUntil = now + BLINK_MS;
    drawFace();
  }
}

// ============================================================
// 7. 输入处理
// ============================================================

void handleInputs() {
  // 每个 loop 都要调用，否则消抖状态不会更新
  bool touchPressed = touchInput.pressedEvent();
  bool leftPressed  = irLeft.pressedEvent();
  bool rightPressed = irRight.pressedEvent();

  if (!inputsArmed()) return;

  if (touchPressed) {
    cycleFace();
    if (motion == MOTION_IDLE) {
      startNod();
    } else {
      pendingNod = true;   // 表情已经切了，点头稍后补上
    }
  }

  if (leftPressed || rightPressed) {
    int8_t side = leftPressed ? -1 : 1;
    Serial.printf("[IR] %s\n", leftPressed ? "left" : "right");

    if (motion == MOTION_IDLE) {
      startShake(side);
    } else {
      pendingShakeSide = side;   // 只保留最后一次
    }
  }
}

// ============================================================
// 8. 初始化 / 主循环
// ============================================================

bool initDisplay() {
  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
  Wire.setClock(400000);

  // 第 4 个参数 false：不要再让库重新初始化 I2C 引脚
  uint8_t addr = OLED_ADDR;
  bool ok = display.begin(SSD1306_SWITCHCAPVCC, addr, true, false);
  if (!ok && addr != 0x3D) {
    Serial.println("[OLED] 0x3C 没找到，尝试 0x3D");
    addr = 0x3D;
    ok = display.begin(SSD1306_SWITCHCAPVCC, addr, true, false);
  }

  if (!ok) {
    Serial.println("[OLED] 初始化失败：查 VCC/GND/SDA/SCL，或改 OLED_ADDR");
    return false;
  }

  activeOledAddr = addr;
  display.setRotation(OLED_ROTATION);
  display.clearDisplay();
  display.display();
  Serial.printf("[OLED] ready, addr=0x%02X\n", activeOledAddr);
  return true;
}

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println();
  Serial.println("=== 瓦楞机器人 / Corrugated Robot ===");
  Serial.printf("[PIN] SDA=%d SCL=%d PAN=%d TILT=%d TOUCH=%d IR_L=%d IR_R=%d\n",
                (int)PIN_I2C_SDA, (int)PIN_I2C_SCL,
                (int)PIN_SERVO_PAN, (int)PIN_SERVO_TILT,
                (int)PIN_TOUCH, (int)PIN_IR_LEFT, (int)PIN_IR_RIGHT);

  // 先把两个舵机放中位，避免上电乱甩
  panAxis.begin(PIN_SERVO_PAN, PAN_CENTER);
  tiltAxis.begin(PIN_SERVO_TILT, TILT_CENTER);
  delay(300);

  oledReady = initDisplay();

  touchInput.begin(PIN_TOUCH, TOUCH_ACTIVE_HIGH);
  irLeft.begin(PIN_IR_LEFT, !IR_ACTIVE_LOW);
  irRight.begin(PIN_IR_RIGHT, !IR_ACTIVE_LOW);

  inputArmAt = millis() + INPUT_ARM_MS;
  nextBlinkAt = millis() + random(BLINK_MIN_MS, BLINK_MAX_MS);

  if (oledReady) {
    drawFace();
  }

  Serial.println("[SYS] ready, 摸屏幕中间的触摸键或左右红外");
}

void loop() {
  panAxis.update();
  tiltAxis.update();

  handleInputs();
  updateEyes();
  serviceMotion();
  serviceQueue();

  // 只让出 1ms 给系统任务，动作仍然由 millis() 驱动，不是靠 delay 等舵机
  delay(1);
}
