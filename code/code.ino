#include <WiFi.h>
#include <Arduino.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <DNSServer.h> // Add for captive portal

const char* ap_ssid = "lucas robot";
const char* ap_password = ""; // Open network, or set a password if you want
const byte DNS_PORT = 53;
DNSServer dnsServer;

bool light = false; // Initialize light state
int myHello = 0;
bool poopExplode = false; // Add flag for poop explosion
WebServer server(80);

void handleRoot() {
    String message = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <title>🤖 Robot Party 🤖</title>
    <style>
        body {
            background: #e0f7fa;
            text-align: center;
            font-family: 'Comic Sans MS', cursive, sans-serif;
        }
        .robot {
            font-size: 5em;
            display: inline-block;
            animation: dance 1s infinite alternate;
            cursor: pointer;
        }
        .robotination {
            position: absolute;
            animation: floatbot 4s linear infinite;
            pointer-events: none;
        }
        .robotination.small { font-size: 2em; animation-duration: 3s;}
        .robotination.medium { font-size: 3em; animation-duration: 4.5s;}
        .robotination.large { font-size: 4em; animation-duration: 6s;}
        @keyframes dance {
            0% { transform: translateY(0) rotate(-10deg);}
            100% { transform: translateY(-40px) rotate(10deg);}
        }
        @keyframes floatbot {
            0% { top: 90vh; left: 10vw; opacity: 0.7; }
            25% { left: 30vw; }
            50% { top: 40vh; left: 60vw; opacity: 1; }
            75% { left: 80vw; }
            100% { top: 0vh; left: 90vw; opacity: 0.5; }
        }
        .counter {
            font-size: 2em;
            color: #0277bd;
            margin-top: 20px;
        }
        .bunny {
            font-size: 2em;
            margin-top: 20px;
            animation: wiggle 0.5s infinite alternate;
        }
        @keyframes wiggle {
            0% { transform: rotate(-5deg);}
            100% { transform: rotate(5deg);}
        }
        .robotain {
            position: fixed;
            pointer-events: none;
            left: 0; top: 0; width: 100vw; height: 100vh; z-index: 0;
        }
    </style>
</head>
<body>
    <div class="robot" id="robot">🤖</div>
    <div class="bunny">[::] <br> (•_•) <br> /|\\ <br> / \\</div>
    <div class="counter">Robot counter: <span id="counter">0</span></div>
    <div>Let's dance, robots! 🤖</div>
    <div class="robotain" id="robotain"></div>
    <script>
        let count = %COUNT%;
        function updateCounter() {
            document.getElementById('counter').textContent = count++;
        }
        setInterval(updateCounter, 1000);
        updateCounter();
        // Make the robot spin on click
        document.getElementById('robot').onclick = function() {
            this.style.transition = "transform 0.5s";
            this.style.transform += " rotate(360deg)";
        };

        // Robotinations: floating robots
        function randomRobotination() {
            const bot = document.createElement('div');
            const sizes = ['small', 'medium', 'large'];
            bot.className = 'robotination ' + sizes[Math.floor(Math.random()*sizes.length)];
            bot.style.left = Math.floor(Math.random()*90) + 'vw';
            bot.style.top = '90vh';
            // Randomly pick a robot emoji for more fun
            const robots = ['🤖','🦾','🦿','🔧','⚙️','🤖'];
            bot.textContent = robots[Math.floor(Math.random()*robots.length)];
            document.getElementById('robotain').appendChild(bot);
            setTimeout(() => bot.remove(), 6000);
        }
        setInterval(randomRobotination, 700);

        // Initial burst of robotinations
        for(let i=0;i<5;i++) setTimeout(randomRobotination, i*400);

        // Robot dance explosion function
        function robotExplosion() {
            for(let i=0;i<50;i++) setTimeout(randomRobotination, Math.random()*1000);
        }

        // Poll for robot explosion
        setInterval(() => {
            fetch('/poopstatus')
                .then(r => r.json())
                .then(j => { if(j.poopExplode) robotExplosion(); });
        }, 500);
    </script>
</body>
</html>
)rawliteral";
    message.replace("%COUNT%", String(myHello));
    server.send(200, "text/html", message);
}

void handlePoopStatus() {
    String json = String("{\"poopExplode\":") + (poopExplode ? "true" : "false") + "}";
    poopExplode = false; // Reset after serving
    server.send(200, "application/json", json);
}

void setup (){
    Serial.begin(9600);

    Serial.println("Booting...");

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

    // Try to start mDNS in AP mode (may not work on all devices/OSes)
    if (!MDNS.begin("esp32")) {
        Serial.println("Error starting mDNS responder!");
    } else {
        Serial.println("mDNS responder started at esp32.local");
    }

    // Register handler and start server after WiFi connects
    server.on("/", handleRoot);
    server.on("/poopstatus", handlePoopStatus);

    // Captive portal: redirect all unknown URLs to root
    server.onNotFound([]() {
        server.sendHeader("Location", String("http://192.168.4.1/"), true);
        server.send(302, "text/plain", "");
    });

    server.begin();
}

void loop (){
    myHello = myHello + 1;
    static bool lastButtonState = HIGH;
    static unsigned long lastDebounceTime = 0;
    static int buttonPressCount = 0;
    const unsigned long debounceDelay = 50; // ms
    const unsigned long pressTimeout = 600; // ms, max time between double press

    int reading = digitalRead(12);

    if (reading != lastButtonState) {
        lastDebounceTime = millis();
    }

    if ((millis() - lastDebounceTime) > debounceDelay) {
        // Button press detected (active low)
        if (lastButtonState == HIGH && reading == LOW) {
            buttonPressCount++;
            if (buttonPressCount == 1) {
                lastDebounceTime = millis(); // Start timeout for double press
            }
        }
        // Check for double press
        if (buttonPressCount > 0 && (millis() - lastDebounceTime) > pressTimeout) {
            if (buttonPressCount == 2) {
                Serial.println("Double press detected, toggling explosion!");
                poopExplode = true; // Set flag for explosion
            }
            buttonPressCount = 0; // Reset counter
        }
    }
    lastButtonState = reading;

    // Light control code (unchanged)
    if(digitalRead(13) == LOW){
        if(light == false) {
            light = true;
            digitalWrite(2, HIGH); // Turn on light
            Serial.println("Light turned ON");
        } else {
            light = false;
            digitalWrite(2, LOW); // Turn off light
            Serial.println("Light turned OFF");
        }
    }
    if (light == true) {
        digitalWrite(2, HIGH); // Ensure light is on
    } else {
        digitalWrite(2, LOW); // Ensure light is off
    }
    dnsServer.processNextRequest(); // Needed for captive portal DNS
    server.handleClient();
}