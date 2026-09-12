#include <WiFi.h>
#include <WebSocketsServer.h>
#include <Wire.h>

// ============================================================
// SPIDER WEB GAME
// ESP32 + MPU6050
// ESP32 = WiFi + Gesture Controller
// PC = Hosts the Game Webpage
// ============================================================


// ============================================================
// WIFI ACCESS POINT
// ============================================================

const char* AP_SSID = "SPIDER_WEB";
const char* AP_PASSWORD = "12345678";

WebSocketsServer webSocket(81);


// ============================================================
// MPU6050
// ============================================================

#define MPU_ADDR 0x68

#define SDA_PIN 21
#define SCL_PIN 22


// ============================================================
// GESTURE SETTINGS
// ============================================================

#define FLICK_THRESHOLD 115.0
#define PULL_THRESHOLD 85.0

#define FLICK_COOLDOWN 900
#define PULL_COOLDOWN 500

#define PULL_BLOCK_AFTER_FLICK 400
#define PULL_CONFIRM_COUNT 3


// ============================================================
// TIMING VARIABLES
// ============================================================

unsigned long lastFlickTime = 0;
unsigned long lastPullTime = 0;
unsigned long flickTime = 0;

int pullConfirm = 0;

bool pullUsed = false;


// ============================================================
// MPU6050 INITIALIZATION
// ============================================================

void setupMPU() {

    Wire.begin(SDA_PIN, SCL_PIN);

    Wire.beginTransmission(MPU_ADDR);
    Wire.write(0x6B);
    Wire.write(0x00);
    byte error = Wire.endTransmission();

    if (error == 0) {
        Serial.println("MPU6050 initialized");
    } 
    else {
        Serial.print("MPU6050 ERROR: ");
        Serial.println(error);
    }

    // Set gyro full scale to ±250 degrees/sec
    Wire.beginTransmission(MPU_ADDR);
    Wire.write(0x1B);
    Wire.write(0x00);
    Wire.endTransmission();
}


// ============================================================
// READ GYRO
// ============================================================

void readGyro(float &gx, float &gy, float &gz) {

    Wire.beginTransmission(MPU_ADDR);
    Wire.write(0x43);
    Wire.endTransmission(false);

    Wire.requestFrom(MPU_ADDR, 6, true);

    int16_t rawGX = Wire.read() << 8 | Wire.read();
    int16_t rawGY = Wire.read() << 8 | Wire.read();
    int16_t rawGZ = Wire.read() << 8 | Wire.read();

    gx = rawGX / 131.0;
    gy = rawGY / 131.0;
    gz = rawGZ / 131.0;
}


// ============================================================
// GESTURE DETECTION
// ============================================================

void detectGestures() {

    float gx, gy, gz;

    readGyro(gx, gy, gz);

    unsigned long now = millis();


    // --------------------------------------------------------
    // FLICK DETECTION
    // --------------------------------------------------------

    if (
        gz > FLICK_THRESHOLD &&
        now - lastFlickTime > FLICK_COOLDOWN
    ) {

        lastFlickTime = now;

        flickTime = now;

        pullUsed = false;

        pullConfirm = 0;

        Serial.println("FLICK");

        webSocket.broadcastTXT("FLICK");
    }


    // --------------------------------------------------------
    // PULL DETECTION
    // --------------------------------------------------------

    if (
        gx > PULL_THRESHOLD &&
        now - lastPullTime > PULL_COOLDOWN &&
        now - flickTime > PULL_BLOCK_AFTER_FLICK &&
        !pullUsed
    ) {

        pullConfirm++;

        if (pullConfirm >= PULL_CONFIRM_COUNT) {

            lastPullTime = now;

            pullUsed = true;

            pullConfirm = 0;

            Serial.println("PULL");

            webSocket.broadcastTXT("PULL");
        }

    } 
    else {

        if (gx < PULL_THRESHOLD) {
            pullConfirm = 0;
        }
    }
}


// ============================================================
// WEBSOCKET EVENTS
// ============================================================

void webSocketEvent(
    uint8_t num,
    WStype_t type,
    uint8_t * payload,
    size_t length
) {

    switch (type) {

        case WStype_CONNECTED:

            Serial.print("WebSocket Client Connected: ");
            Serial.println(num);

            break;


        case WStype_DISCONNECTED:

            Serial.print("WebSocket Client Disconnected: ");
            Serial.println(num);

            break;


        case WStype_TEXT:

            Serial.print("Received from PC: ");

            for (size_t i = 0; i < length; i++) {
                Serial.print((char)payload[i]);
            }

            Serial.println();

            break;


        default:

            break;
    }
}


// ============================================================
// SETUP
// ============================================================

void setup() {

    Serial.begin(115200);

    delay(1000);

    Serial.println();
    Serial.println("================================");
    Serial.println("       SPIDER WEB GAME");
    Serial.println("================================");


    // --------------------------------------------------------
    // MPU6050
    // --------------------------------------------------------

    setupMPU();


    // --------------------------------------------------------
    // START WIFI ACCESS POINT
    // --------------------------------------------------------

    Serial.println();
    Serial.println("Starting WiFi Access Point...");

    WiFi.mode(WIFI_AP);

    delay(200);


    bool apStarted = WiFi.softAP(
        AP_SSID,
        AP_PASSWORD
    );


    if (apStarted) {

        Serial.println("ACCESS POINT STARTED");

    } 
    else {

        Serial.println("ACCESS POINT FAILED");

    }


    // --------------------------------------------------------
    // PRINT WIFI INFORMATION
    // --------------------------------------------------------

    Serial.print("SSID: ");
    Serial.println(AP_SSID);

    Serial.print("Password: ");
    Serial.println(AP_PASSWORD);

    Serial.print("ESP32 IP: ");
    Serial.println(WiFi.softAPIP());


    // --------------------------------------------------------
    // START WEBSOCKET SERVER
    // --------------------------------------------------------

    webSocket.begin();

    webSocket.onEvent(webSocketEvent);


    // --------------------------------------------------------
    // READY
    // --------------------------------------------------------

    Serial.println();
    Serial.println("================================");
    Serial.println("        GAME CONTROLLER READY");
    Serial.println("================================");

    Serial.println("Connect PC to: SPIDER_WEB");

    Serial.println("Game WebSocket:");
    Serial.println("ws://192.168.4.1:81/");

    Serial.println();
}


// ============================================================
// LOOP
// ============================================================

void loop() {

    webSocket.loop();

    detectGestures();

    delay(5);
}