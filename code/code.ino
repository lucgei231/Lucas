#include <WiFi.h>
#include <Arduino.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <DNSServer.h>
#include <ESP32Servo.h>

// --- Global variables ---
const char* ap_ssid = "lucas robot";
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
        .status-on { color: green; font-weight: bold; }
        .status-off { color: red; font-weight: bold; }
    </style>
</head>
<body>
    <h1>ESP32 Robot Control Panel</h1>
    <div class="info">
        <table>
            <tr><td><b>myHello Counter:</b></td><td><span id="myHello">%MYHELLO%</span></td></tr>
            <tr><td><b>Button 12 (Double Press):</b></td><td><span id="btn12">%BTN12%</span></td></tr>
            <tr><td><b>Button 13 (Servo Right):</b></td><td><span id="btn13">%BTN13%</span></td></tr>
            <tr><td><b>Explosion Pending:</b></td><td><span id="explosion">%EXPLODE%</span></td></tr>
            <tr><td><b>Uptime (s):</b></td><td><span id="uptime">%UPTIME%</span></td></tr>
            <tr><td><b>AP IP Address:</b></td><td><span id="ip">%IP%</span></td></tr>
            <tr><td><b>Internet Status:</b></td><td><span id="sta">%STA%</span></td></tr>
        </table>
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
    server.send(200, "text/html", message);
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
    WiFi.softAP(ap_ssid, ap_password);
    delay(100);
    IPAddress myIP = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(myIP);

    dnsServer.start(DNS_PORT, "*", myIP);

    pinMode(12, INPUT_PULLUP);
    pinMode(13, INPUT_PULLUP);
    pinMode(32, INPUT);
    pinMode(33, INPUT);
    myServo.setPeriodHertz(50);
    myServo.attach(16, 500, 2500);
    myServo2.setPeriodHertz(50);
    myServo2.attach(17, 500, 2500);
    myServo3.setPeriodHertz(50);
    myServo3.attach(18, 500, 2500);

    if (!MDNS.begin("esp32")) {
        Serial.println("Error starting mDNS responder!");
    } else {
        Serial.println("mDNS responder started at esp32.local");
    }

    server.on("/", handleRoot);
    server.on("/poopstatus", handlePoopStatus);
    server.on("/triggerexplosion", HTTP_POST, handleTriggerExplosion);
    server.on("/connectinternet", HTTP_GET, handleConnectInternet);
    server.on("/connectinternet", HTTP_POST, handleConnectInternetPost);

    server.onNotFound([]() {
        server.sendHeader("Location", String("http://192.168.4.1/"), true);
        server.send(302, "text/plain", "");
    });

    server.begin();
}

void loop (){
    myHello = myHello + 1;
    int mySpin;

    // Button 12: spin all servos left (0 deg
        Serial.println(mySpin);


        if (analogRead(32) / 20 > 180) {
            mySpin = analogRead(32) - 180 / 20;
        } else {
            mySpin = analogRead(32) / 20;
        }
        myServo.write(mySpin);
        myServo2.write(mySpin);
        myServo3.write(mySpin);

    // dnsServer.processNextRequest();
    // server.handleClient();
}