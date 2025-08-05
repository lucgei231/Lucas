#include <WiFi.h>
#include <Arduino.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <DNSServer.h>
#include <ESP32Servo.h> // Add this include

void startup() {
    const char* ap_ssid = "lucas robot";
    const char* ap_password = ""; // Open network, or set a password if you want
    const byte DNS_PORT = 53;
        DNSServer dnsServer;

    bool light = false; // Initialize light state
    int myHello = 0;
    bool poopExplode = false; // Add flag for poop explosion
    WebServer server(80);

    // Add global variables for STA connection
    bool staConnected = true;
    String staSSID = "mypotato";
    String staPASS = "mypotato";
}

Servo myServo; // Servo object
int servoPos = 0;
bool servoDir = true;

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
            .status-on { color: green; font-weight: bold; }
            .status-off { color: red; font-weight: bold; }
        </style>
    </head>
    <body>
        <h1>ESP32 Robot Control Panel</h1>
        <div class="info">
            <table>
                <tr><td><b>Light State:</b></td><td><span id="lightState">%LIGHTSTATE%</span></td></tr>
                <tr><td><b>myHello Counter:</b></td><td><span id="myHello">%MYHELLO%</span></td></tr>
                <tr><td><b>Button 12 (Double Press):</b></td><td><span id="btn12">%BTN12%</span></td></tr>
                <tr><td><b>Button 13 (Toggle Light):</b></td><td><span id="btn13">%BTN13%</span></td></tr>
                <tr><td><b>Explosion Pending:</b></td><td><span id="explosion">%EXPLODE%</span></td></tr>
                <tr><td><b>Uptime (s):</b></td><td><span id="uptime">%UPTIME%</span></td></tr>
                <tr><td><b>AP IP Address:</b></td><td><span id="ip">%IP%</span></td></tr>
                <tr><td><b>Internet Status:</b></td><td><span id="sta">%STA%</span></td></tr>
            </table>
        </div>
        <div>
            <button class="btn" onclick="toggleLight()">Toggle Light</button>
            <button class="btn" onclick="triggerExplosion()">Trigger Explosion</button>
            <button class="btn" onclick="location.reload()">Refresh</button>
            <button class="btn" onclick="window.location.href='/connectinternet'">Connect to Internet</button>
        </div>
        <script>
            function toggleLight() {
                fetch('/togglelight', {method: 'POST'}).then(() => setTimeout(()=>location.reload(), 300));
            }
            function triggerExplosion() {
                fetch('/triggerexplosion', {method: 'POST'}).then(() => setTimeout(()=>location.reload(), 300));
            }
        </script>
    </body>
    </html>
    )rawliteral";
        message.replace("%LIGHTSTATE%", light ? "<span class='status-on'>ON</span>" : "<span class='status-off'>OFF</span>");
        message.replace("%MYHELLO%", String(myHello));
        message.replace("%BTN12%", digitalRead(12) == LOW ? "PRESSED" : "Released");
        message.replace("%BTN13%", digitalRead(13) == LOW ? "PRESSED" : "Released");
        message.replace("%EXPLODE%", poopExplode ? "YES" : "NO");
        message.replace("%UPTIME%", String(millis() / 1000));
        message.replace("%IP%", WiFi.softAPIP().toString());
        message.replace("%STA%", staConnected ? "<span class='status-on'>Connected</span>" : "<span class='status-off'>Not Connected</span>");
        server.send(200, "text/html", message);
    }
void handleToggleLight() {
        light = !light;
        digitalWrite(2, light ? HIGH : LOW);
        server.send(200, "text/plain", "OK");
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
ESP32Servo myServo; // Servo object
void setup (){
    Serial.begin(9600);

    Serial.println("Booting...");
    startup();
    // Start WiFi in AP mode
    WiFi.softAP(ap_ssid, ap_password);
    delay(100); // Give AP time to start
    IPAddress myIP = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(myIP);

    // Captive portal: redirect all DNS queries to the ESP32
    dnsServer.start(DNS_PORT, "*", myIP);

    pinMode(2, OUTPUT); // Set pin 2 as output
    pinMode(12, INPUT_PULLUP); // Set pin 12 as input with pull-up resistor
    pinMode(13, INPUT_PULLUP); // Set pin 13 as input with pull-up resistor

    myServo.setPeriodHertz(50); // Standard 50Hz servo
    myServo.attach(16); // Attach servo to pin 16

    // Try to start mDNS in AP mode (may not work on all devices/OSes)
    if (!MDNS.begin("esp32")) {
        Serial.println("Error starting mDNS responder!");
    } else {
        Serial.println("mDNS responder started at esp32.local");
    }

    // Register handler and start server after WiFi connects
    server.on("/", handleRoot);
    server.on("/poopstatus", handlePoopStatus);
    server.on("/togglelight", HTTP_POST, handleToggleLight);
    server.on("/triggerexplosion", HTTP_POST, handleTriggerExplosion);
    server.on("/connectinternet", HTTP_GET, handleConnectInternet);
    server.on("/connectinternet", HTTP_POST, handleConnectInternetPost);

    // Captive portal: redirect all unknown URLs to root
    server.onNotFound([]() {
        server.sendHeader("Location", String("http://192.168.4.1/"), true);
        server.send(302, "text/plain", "");
    });

    server.begin();
}

void loop (){
    myHello = myHello + 1;
    static bool lastButton12State = HIGH;
    static bool lastButton13State = HIGH;
    static unsigned long lastDebounce12 = 0;
    static unsigned long lastDebounce13 = 0;
    const unsigned long debounceDelay = 50; // ms

    int reading12 = digitalRead(12);
    int reading13 = digitalRead(13);

    // Button 12: spin servo left (0 deg)
    if (reading12 != lastButton12State) {
        lastDebounce12 = millis();
    }
    if ((millis() - lastDebounce12) > debounceDelay) {
        if (lastButton12State == HIGH && reading12 == LOW) {
            myServo.write(0);
            Serial.println("Servo LEFT (0 deg)");
        }
    }
    lastButton12State = reading12;

    // Button 13: spin servo right (180 deg) and toggle light
    if (reading13 != lastButton13State) {
        lastDebounce13 = millis();
    }
    if ((millis() - lastDebounce13) > debounceDelay) {
        if (lastButton13State == HIGH && reading13 == LOW) {
            myServo.write(180);
            Serial.println("Servo RIGHT (180 deg)");
            // Toggle light as before
            if(light == false) {
                light = true;
                digitalWrite(2, HIGH); // Turn on light
                Serial.println("Light turned ON");
                delay(1000);
            } else {
                light = false;
                digitalWrite(2, LOW); // Turn off light
                Serial.println("Light turned OFF");
                delay(1000);
            }
        }
    }
    lastButton13State = reading13;

    if (light == true) {
        digitalWrite(2, HIGH); // Ensure light is on
    } else {
        digitalWrite(2, LOW); // Ensure light is off
    }

    dnsServer.processNextRequest(); // Needed for captive portal DNS
    server.handleClient();
}