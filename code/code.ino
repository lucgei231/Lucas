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

    server.onNotFound([]() {
        server.sendHeader("Location", String("http://192.168.4.1/"), true);
        server.send(302, "text/plain", "");
    });

    server.begin();
}

void loop (){
    myHello = myHello + 1;

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
    int pot32 = analogRead(32);
    int pot33 = analogRead(33);
    int angle32 = map(pot32, 0, 4095, 180, 0);
    int angle33 = map(pot33, 0, 4095, 180, 0);

    // Only use potentiometer if no slider used in last 2 seconds
    if (millis() - lastSliderMillis > 2000) {
        myServo2.write(angle32);
        myServo3.write(angle33);
        myServo.write((angle32 + angle33) / 2);
    }

    // --- Button 12: set all servos to 0 deg (debounce fix) ---
    static int lastButton12State = HIGH;
    static unsigned long lastDebounce12 = 0;
    const unsigned long debounceDelay = 50;
    int reading12 = digitalRead(12);
    if (reading12 != lastButton12State) {
        lastDebounce12 = millis();
    }
    if ((millis() - lastDebounce12) > debounceDelay) {
        if (lastButton12State == HIGH && reading12 == LOW) { // Only on transition
            myServo.write(0);
            myServo2.write(0);
            myServo3.write(0);
            Serial.println("All Servos LEFT (0 deg)");
        }
    }
    lastButton12State = reading12;

    // --- Button 13: set all servos to 180 deg (debounce fix) ---
    static int lastButton13State = HIGH;
    static unsigned long lastDebounce13 = 0;
    int reading13 = digitalRead(13);
    if (reading13 != lastButton13State) {
        lastDebounce13 = millis();
    }
    if ((millis() - lastDebounce13) > debounceDelay) {
        if (lastButton13State == HIGH && reading13 == LOW) { // Only on transition
            myServo.write(180);
            myServo2.write(180);
            myServo3.write(180);
            Serial.println("All Servos RIGHT (180 deg)");
        }
    }
    lastButton13State = reading13;

    dnsServer.processNextRequest();
    server.handleClient();
}