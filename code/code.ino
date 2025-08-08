#include <WiFi.h>
#include <Arduino.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <DNSServer.h>
#include <ESP32Servo.h>

// --- Global variables ---
const char* ap_ssid = "crappy wifi";
const char* ap_password = "";
const byte DNS_PORT = 53;
DNSServer dnsServer;
int playbackIndex = 0;
bool playbackActive;
int playbackSlot = -1;
bool active = false;
unsigned long lastPlaybackMillis = 0;

int myHello = 0;
bool poopExplode = false;
WebServer server(80);

bool staConnected = true;
String staSSID = "mypotato";
String staPASS = "mypotato";

Servo myServo;
Servo myServo2;
Servo myServo3;
int servoPos = 0;
bool servoDir = true;

// --- Add these globals for slider/potentiometer logic ---
unsigned long lastSliderMillis = 0;
int lastS1 = 0, lastS2 = 0, lastS3 = 0;
int servo16Pos = 90; // Add this global for precision control

// --- Recording and saves ---
#define MAX_RECORD_LENGTH 300
#define MAX_SAVES 5
struct ServoFrame {
    int s1, s2, s3;
};
ServoFrame recordBuffer[MAX_RECORD_LENGTH];
int recordLength = 0;
bool isRecording = false;
unsigned long lastRecordMillis = 0;

// Saved recordings (renameable)
const char* saveNames[MAX_SAVES] = {"Save1", "Save2", "Save3", "Save4", "Save5"};
ServoFrame saveBuffers[MAX_SAVES][MAX_RECORD_LENGTH];
int saveLengths[MAX_SAVES] = {0,0,0,0,0};

// --- Web handlers ---
void handleConnectInternet() {
    String html = R"rawliteral(
    <!DOCTYPE html>
    <html>
    <head>
        <title>Connect to Internet</title>
        <style>
            body { background: #e0f7fa; font-family: Arial, sans-serif; text-align: center; }
            .formbox { margin: 40px auto; background: #fff; border-radius: 10px; padding: 30px; box-shadow: 0 2px 8px #bbb; display: inline-block; }
            input, button { font-size: 1.2em; margin: 10px; padding: 8px 20px; border-radius: 8px; border: 1px solid #0277bd; }
            button { background: #0277bd; color: #fff; border: none; }
            button:active { background: #015a7a; }
        </style>
    </head>
    <body>
        <div class="formbox">
            <h2>Connect ESP32 to Internet</h2>
            <form action="/connectinternet" method="POST">
                <input name="ssid" placeholder="WiFi SSID" required><br>
                <input name="pass" type="password" placeholder="WiFi Password"><br>
                <button type="submit">Connect</button>
            </form>
            <br>
            <button onclick="window.location.href='/'">Back</button>
        </div>
    </body>
    </html>
    )rawliteral";
    server.send(200, "text/html", html);
}

void handleConnectInternetPost() {
    if (server.hasArg("ssid")) {
        staSSID = server.arg("ssid");
        staPASS = server.arg("pass");
        WiFi.softAPdisconnect(true);
        delay(500);
        WiFi.mode(WIFI_AP_STA);
        WiFi.begin(staSSID.c_str(), staPASS.c_str());
        unsigned long startAttempt = millis();
        staConnected = false;
        while (WiFi.status() != WL_CONNECTED && millis() - startAttempt < 10000) {
            delay(500);
        }
        staConnected = (WiFi.status() == WL_CONNECTED);
        // Restart AP for clients
        WiFi.softAP(ap_ssid, ap_password);
        delay(100);
        IPAddress myIP = WiFi.softAPIP();
        dnsServer.start(DNS_PORT, "*", myIP);
    }
    server.sendHeader("Location", "/", true);
    server.send(302, "text/plain", "");
}

void handleRoot() {
    String message = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <title>ESP32 Robot Control Panel</title>
    <style>
        body { background: #e0f7fa; font-family: Arial, sans-serif; text-align: center; }
        h1 { color: #0277bd; }
        .info { margin: 20px auto; display: inline-block; text-align: left; background: #fff; border-radius: 10px; padding: 20px; box-shadow: 0 2px 8px #bbb; }
        .info td { padding: 6px 12px; }
        .btn { font-size: 1.2em; margin: 10px; padding: 10px 30px; border-radius: 8px; border: none; background: #0277bd; color: #fff; cursor: pointer; }
        .btn:active { background: #015a7a; }
        .slider { width: 200px; }
        .servo-label { font-weight: bold; color: #0277bd; }
        .status-on { color: green; font-weight: bold; }
        .status-off { color: red; font-weight: bold; }
    </style>
</head>
<body>
    <h1>ESP32 Robot Control Panel</h1>
    <div class="info">
        <table>
            <tr><td><b>myHello Counter:</b></td><td><span id="myHello">%MYHELLO%</span></td></tr>
            <tr><td><b>Button 12 (All Servos 0°):</b></td><td><span id="btn12">%BTN12%</span></td></tr>
            <tr><td><b>Button 13 (All Servos 180°):</b></td><td><span id="btn13">%BTN13%</span></td></tr>
            <tr><td><b>Explosion Pending:</b></td><td><span id="explosion">%EXPLODE%</span></td></tr>
            <tr><td><b>Uptime (s):</b></td><td><span id="uptime">%UPTIME%</span></td></tr>
            <tr><td><b>AP IP Address:</b></td><td><span id="ip">%IP%</span></td></tr>
            <tr><td><b>Internet Status:</b></td><td><span id="sta">%STA%</span></td></tr>
        </table>
    </div>
    <div style="margin:20px;">
        <div>
            <span class="servo-label">Servo 1 (Pin 16):</span>
            <input type="range" min="0" max="180" value="%S1%" class="slider" id="servo1">
            <span id="servo1val">%S1%</span>°
        </div>
        <div>
            <span class="servo-label">Servo 2 (Pin 17):</span>
            <input type="range" min="0" max="180" value="%S2%" class="slider" id="servo2">
            <span id="servo2val">%S2%</span>°
        </div>
        <div>
            <span class="servo-label">Servo 3 (Pin 18):</span>
            <input type="range" min="0" max="180" value="%S3%" class="slider" id="servo3">
            <span id="servo3val">%S3%</span>°
        </div>
    </div>
    <div>
        <button class="btn" onclick="triggerExplosion()">Trigger Explosion</button>
        <button class="btn" onclick="location.reload()">Refresh</button>
        <button class="btn" onclick="window.location.href='/connectinternet'">Connect to Internet</button>
    </div>
    <div>
        <button class="btn" onclick="fetch('/startrecording',{method:'POST'}).then(()=>location.reload())">Start Recording</button>
        <button class="btn" onclick="fetch('/stoprecording',{method:'POST'}).then(()=>location.reload())">Stop Recording</button>
        <button class="btn" onclick="window.location.href='/listsaves'">Show Saves</button>
    </div>
    <script>
        function triggerExplosion() {
            fetch('/triggerexplosion', {method: 'POST'}).then(() => setTimeout(()=>location.reload(), 300));
        }
        // Show slider value live and set servo instantly
        document.querySelectorAll('.slider').forEach(function(slider){
            slider.oninput = function() {
                document.getElementById(this.id+'val').textContent = this.value;
                let num = this.id.replace('servo','');
                fetch('/setservo?num='+num+'&val='+this.value, {method:'POST'});
            }
        });
    </script>
</body>
</html>
)rawliteral";
    message.replace("%MYHELLO%", String(myHello));
    message.replace("%BTN12%", digitalRead(12) == LOW ? "PRESSED" : "Released");
    message.replace("%BTN13%", digitalRead(13) == LOW ? "PRESSED" : "Released");
    message.replace("%EXPLODE%", poopExplode ? "YES" : "NO");
    message.replace("%UPTIME%", String(millis() / 1000));
    message.replace("%IP%", WiFi.softAPIP().toString());
    message.replace("%STA%", staConnected ? "<span class='status-on'>Connected</span>" : "<span class='status-off'>Not Connected</span>");
    message.replace("%S1%", String(myServo.read()));
    message.replace("%S2%", String(myServo2.read()));
    message.replace("%S3%", String(myServo3.read()));
    server.send(200, "text/html", message);
}

// --- Suggestions endpoint ---
void handleSuggestions() {
    String json = "[";
    json += "\"Try using the web sliders to control servos.\",";
    json += "\"Press button 12 to move all servos to 0°.\",";
    json += "\"Press button 13 to move all servos to 180°.\",";
    json += "\"Use potentiometers to control servo 2 and 3.\",";
    json += "\"Trigger explosion for a fun malfunction!\",";
    json += "\"Connect to your home WiFi via 'Connect to Internet'.\",";
    json += "\"Check the AP IP address for direct access.\",";
    json += "\"Monitor uptime and status on the main page.\",";
    json += "\"Try accessing esp32.local if mDNS works.\",";
    json += "\"Refresh the page to update all values.\"";
    json += "]";
    server.send(200, "application/json", json);
}

// Add handler for /setservo
void handleSetServo() {
    if (server.hasArg("num") && server.hasArg("val")) {
        int num = server.arg("num").toInt();
        int val = server.arg("val").toInt();
        lastSliderMillis = millis();
        if (num == 1) { myServo.write(val); lastS1 = val; }
        else if (num == 2) { myServo2.write(val); lastS2 = val; }
        else if (num == 3) { myServo3.write(val); lastS3 = val; }
        server.send(200, "text/plain", "OK");
    } else {
        server.send(400, "text/plain", "Missing args");
    }
}

void handleTriggerExplosion() {
    poopExplode = true;
    server.send(200, "text/plain", "OK");
}

void handlePoopStatus() {
    String json = String("{\"poopExplode\":") + (poopExplode ? "true" : "false") + "}";
    poopExplode = false; // Reset after serving
    server.send(200, "application/json", json);
}


void handleStartRecording() {
    isRecording = true;
    recordLength = 0;
    lastRecordMillis = millis();
    server.send(200, "text/plain", "Recording started");
}

void handleStopRecording() {
    isRecording = false;
    // Find first empty save slot
    int slot = -1;
    for (int i = 0; i < MAX_SAVES; i++) {
        if (saveLengths[i] == 0) { slot = i; break; }
    }
    if (slot != -1 && recordLength > 0) {
        memcpy(saveBuffers[slot], recordBuffer, sizeof(ServoFrame) * recordLength);
        saveLengths[slot] = recordLength;
        server.send(200, "text/plain", String("Recording saved as ") + saveNames[slot]);
    } else {
        server.send(200, "text/plain", "No slot available or nothing recorded");
    }
}

void handleListSaves() {
    String html = "<html><body><h2>Saved Recordings</h2><ul>";
    for (int i = 0; i < MAX_SAVES; i++) {
        if (saveLengths[i] > 0) {
            html += "<li>" + String(saveNames[i]) + " (" + String(saveLengths[i]) + " frames) ";
            html += "<button onclick=\"location.href='/loadsave?slot=" + String(i) + "'\">Load</button> ";
            html += "<button onclick=\"renameSave(" + String(i) + ")\">Rename</button></li>";
        }
    }
    html += "</ul><script>function renameSave(i){var n=prompt('New name:');if(n)fetch('/renamesave?slot='+i+'&name='+encodeURIComponent(n),{method:'POST'}).then(()=>location.reload());}</script>";
    html += "<button onclick=\"location.href='/'\">Back</button></body></html>";
    server.send(200, "text/html", html);
}

void handleLoadSave() {
    if (server.hasArg("slot")) {
        int slot = server.arg("slot").toInt();
        if (slot >= 0 && slot < MAX_SAVES && saveLengths[slot] > 0) {
            // Playback in loop (set a flag)
            playbackSlot = slot;
            playbackIndex = 0;
            playbackActive = true;
            server.send(200, "text/plain", "Playback started for " + String(saveNames[slot]));
        } else {
            server.send(400, "text/plain", "Invalid slot");
        }
    } else {
        server.send(400, "text/plain", "Missing slot");
    }
}

void handleRenameSave() {
    if (server.hasArg("slot") && server.hasArg("name")) {
        int slot = server.arg("slot").toInt();
        String newName = server.arg("name");
        if (slot >= 0 && slot < MAX_SAVES && saveLengths[slot] > 0) {
            saveNames[slot] = strdup(newName.c_str());
            server.send(200, "text/plain", "Renamed");
        } else {
            server.send(400, "text/plain", "Invalid slot");
        }
    } else {
        server.send(400, "text/plain", "Missing args");
    }
}

// --- Playback state ---


void drawSmileyFace() {
    recordLength = 0;

    auto addFrame = [](int height, int left, int right) {
        if (recordLength < MAX_RECORD_LENGTH) {
            recordBuffer[recordLength++] = {height, left, right};
        }
    };

    // --- Draw face outline (approximate circle) ---
    for (int i = 0; i <= 360; i += 15) {
        int h = 90 + 30 * sin(i * PI / 180);     // height motor
        int l = 90 + 20 * cos(i * PI / 180);     // left motor
        int r = 90 - 20 * cos(i * PI / 180);     // right motor
        addFrame(h, l, r);
    }

    // --- Pen up (return to center) ---
    addFrame(90, 90, 90);

    // --- Left eye ---
    for (int i = 0; i <= 360; i += 30) {
        int h = 70 + 5 * sin(i * PI / 180);
        int l = 80 + 5 * cos(i * PI / 180);
        int r = 100 - 5 * cos(i * PI / 180);
        addFrame(h, l, r);
    }

    addFrame(90, 90, 90);

    // --- Right eye ---
    for (int i = 0; i <= 360; i += 30) {
        int h = 70 + 5 * sin(i * PI / 180);
        int l = 100 + 5 * cos(i * PI / 180);
        int r = 80 - 5 * cos(i * PI / 180);
        addFrame(h, l, r);
    }

    addFrame(90, 90, 90);

    // --- Smile arc ---
    for (int i = 200; i <= 340; i += 10) {
        int h = 110 + 10 * sin(i * PI / 180);
        int l = 90 + 15 * cos(i * PI / 180);
        int r = 90 - 15 * cos(i * PI / 180);
        addFrame(h, l, r);
    }

    addFrame(90, 90, 90);

    // Save to slot 0
    memcpy(saveBuffers[0], recordBuffer, sizeof(ServoFrame) * recordLength);
    saveLengths[0] = recordLength;
    Serial.println("Smiley face saved to slot 0!");
}
// --- Setup and loop ---
void setup (){
    Serial.begin(9600);

    Serial.println("Booting...");

    // Start AP mode
    WiFi.softAP(ap_ssid, ap_password);
    delay(100);
    IPAddress myAPIP = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(myAPIP);

    // Start STA mode and connect to WiFi
    WiFi.mode(WIFI_AP_STA);
    WiFi.begin("mypotato", "mypotato");
    Serial.print("Connecting to STA SSID: mypotato");
    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < 10000) {
        delay(500);
        Serial.print(".");
    }
    Serial.println();
    if (WiFi.status() == WL_CONNECTED) {
        Serial.print("STA connected! IP address: ");
        Serial.println(WiFi.localIP());
    } else {
        Serial.println("STA connection failed.");
    }

    dnsServer.start(DNS_PORT, "*", myAPIP);

    pinMode(12, INPUT_PULLUP);
    pinMode(13, INPUT_PULLUP);
    pinMode(32, INPUT);
    pinMode(33, INPUT);
    pinMode(2, OUTPUT); // Add this to control D2 LED
    myServo.setPeriodHertz(50);
    myServo.attach(16, 500, 2500);
    myServo2.setPeriodHertz(50);
    myServo2.attach(17, 500, 2500);
    myServo3.setPeriodHertz(50);
    myServo3.attach(18, 500, 2500);

    // Start mDNS for AP and STA
    if (!MDNS.begin("esp32")) {
        Serial.println("Error starting mDNS responder!");
    } else {
        Serial.println("mDNS responder started at esp32.local");
    }

    // Start web server for both AP and STA
    server.on("/", handleRoot);
    server.on("/poopstatus", handlePoopStatus);
    server.on("/triggerexplosion", HTTP_POST, handleTriggerExplosion);
    server.on("/setservo", HTTP_POST, handleSetServo);
    server.on("/connectinternet", HTTP_GET, handleConnectInternet);
    server.on("/connectinternet", HTTP_POST, handleConnectInternetPost);
    server.on("/suggestions", HTTP_GET, handleSuggestions); // <-- Add this line
    server.on("/startrecording", HTTP_POST, handleStartRecording);
    server.on("/stoprecording", HTTP_POST, handleStopRecording);
    server.on("/listsaves", HTTP_GET, handleListSaves);
    server.on("/loadsave", HTTP_GET, handleLoadSave);
    server.on("/renamesave", HTTP_POST, handleRenameSave);
    server.on("/drawsmiley", HTTP_GET, []() {
    drawSmileyFace();
    server.send(200, "text/plain", "Smiley face saved!");
});
    server.onNotFound([]() {
        server.sendHeader("Location", String("http://192.168.4.1/"), true);
        server.send(302, "text/plain", "");
    });

    server.begin();
}

void loop (){
    myHello = myHello + 1;
    // Print analogRead(32) and analogRead(33) separated by a comma
    Serial.print(analogRead(32));
    Serial.print(" , ");
    Serial.println(analogRead(33));
    static unsigned long explosionStart = 0;
    static bool malfunctioning = false;

    // Handle explosion/malfunction
    if (poopExplode && !malfunctioning) {
        malfunctioning = true;
        explosionStart = millis();
        poopExplode = false;
        Serial.println("!!! MALFUNCTION STARTED !!!");
    }

    if (malfunctioning) {
        // Malfunction: blink LED and move servos randomly, but avoid WDT resets
        digitalWrite(2, millis() % 200 < 100 ? HIGH : LOW);

        static unsigned long lastMove = 0;
        if (millis() - lastMove > 150) { // Move even less frequently
            int r1 = esp_random() % 181;
            int r2 = esp_random() % 181;
            int r3 = esp_random() % 181;
            myServo.write(r1);
            myServo2.write(r2);
            myServo3.write(r3);
            lastMove = millis();
        }

        // Only process web and DNS every 10th malfunction loop to reduce load
        static int webCount = 0;
        if (++webCount >= 10) {
            dnsServer.processNextRequest();
            server.handleClient();
            webCount = 0;
        }

        // Call yield() and delay to avoid watchdog resets
        yield();
        delay(20);

        if (millis() - explosionStart > 20000) {
            malfunctioning = false;
            digitalWrite(2, LOW);
            Serial.println("!!! MALFUNCTION ENDED !!!");
        }
        
    }

    // --- Potentiometer/slider control logic ---
    // int pot32 = analogRead(32);
    // int pot33 = analogRead(33);
    // int angle32 = map(pot32, 0, 4095, 180, 0);
    // int angle33 = map(pot33, 0, 4095, 180, 0);

    // Only use potentiometer if no slider used in last 2 seconds
    // --- Potentiometer/slider control logic ---
int pot32 = analogRead(32);
int pot33 = analogRead(33);
int angle32 = map(pot32, 0, 4095, 180, 0);
int angle33 = map(pot33, 0, 4095, 180, 0);

// Only use potentiometer if no slider used in last 2 seconds AND not playing back
if (!playbackActive && millis() - lastSliderMillis > 2000) {
    myServo2.write(angle32);
    myServo3.write(angle33);
}

    // --- Button 12: move servo on pin 16 left by 10 degrees while held ---
    static unsigned long lastMove12 = 0;
    int reading12 = digitalRead(12);
    if (reading12 == LOW) {
        if (millis() - lastMove12 > 100) {
            servo16Pos = max(0, servo16Pos - 10);
            myServo.write(servo16Pos);
            Serial.print("Servo 16 moved LEFT to ");
            Serial.println(servo16Pos);
            lastMove12 = millis();
        }
    } else {
        lastMove12 = millis();
    }

    // --- Button 13: move servo on pin 16 right by 10 degrees while held ---
    static unsigned long lastMove13 = 0;
    int reading13 = digitalRead(13);
    if (reading13 == LOW) {
        if (millis() - lastMove13 > 100) {
            servo16Pos = min(180, servo16Pos + 10);
            myServo.write(servo16Pos);
            Serial.print("Servo 16 moved RIGHT to ");
            Serial.println(servo16Pos);
            lastMove13 = millis();
        }
    } else {
        lastMove13 = millis();
    }

    // --- Recording logic ---
    if (isRecording && millis() - lastRecordMillis > 10) {
        if (recordLength < MAX_RECORD_LENGTH) {
            recordBuffer[recordLength].s1 = myServo.read();
            recordBuffer[recordLength].s2 = myServo2.read();
            recordBuffer[recordLength].s3 = myServo3.read();
            recordLength++;
        }
        lastRecordMillis = millis();
    }

    // --- Playback logic ---
    if (playbackActive && playbackSlot >= 0 && playbackIndex < saveLengths[playbackSlot]) {
        if (millis() - lastPlaybackMillis > 10) {
            ServoFrame f = saveBuffers[playbackSlot][playbackIndex];
            myServo.write(f.s1);
            myServo2.write(f.s2);
            myServo3.write(f.s3);
            playbackIndex++;
            lastPlaybackMillis = millis();
        }
        if (playbackIndex >= saveLengths[playbackSlot]) {
            playbackActive = false;
            playbackSlot = -1;
            playbackIndex = 0;
        }
    }

    dnsServer.processNextRequest();
    server.handleClient();
}