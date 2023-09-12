#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <Servo.h>
#include <EEPROM.h> // 添加EEPROM库

ESP8266WebServer server(80);
Servo servo1;
Servo servo2;

const char* ssid = "Your_AP_SSID";
const char* password = "Your_AP_Password";

int servo1Pin = 4; // 连接第一个电机的引脚
int servo2Pin = 5; // 连接第二个电机的引脚
int initRad = 90;
int rad = 60;

// EEPROM存储地址
int eepromAddress = 0;

String controlHtml() {
  String html = "<html lang=\"en\">";
  html += "<head>";
  html += "    <meta charset=\"UTF-8\">";
  html += "    <title>开关控制</title>";
  html += "    <link rel=\"stylesheet\" href=\"https://raw.staticdn.net/Gabrlie/ESP8266-Web-LightController/main/style.css\">";
  html += "</head>";
  html += "<body>";
  html += "<button onclick='toggleMotor(\"ao\")'>全部开灯</button>";
  html += "<button onclick='toggleMotor(\"af\")'>全部关灯</button>";
  html += "<button onclick='toggleMotor(\"lo\")'>开左灯</button>";
  html += "<button onclick='toggleMotor(\"lf\")'>关左灯</button>";
  html += "<button onclick='toggleMotor(\"ro\")'>开右灯</button>";
  html += "<button onclick='toggleMotor(\"rf\")'>关右灯</button>";
  html += "<script>";
  html += "function toggleMotor(motor) {";
  html += "var xhr = new XMLHttpRequest();";
  html += "xhr.open('GET', '/toggle?motor=' + motor, true);";
  html += "xhr.onreadystatechange = function () {";
  html += "if (xhr.readyState === XMLHttpRequest.DONE && xhr.status === 200) {";
  html += "var response = JSON.parse(xhr.responseText);";
  html += "if (response.success) {} else {";
  html += "console.log('Failed to toggle Motor ' + motor + '.');";
  html += "}";
  html += "}";
  html += "};";
  html += "xhr.send();";
  html += "}";
  html += "</script>";
  html += "</body>";
  html += "</html>";
  return html;
}

void handleRoot() {
String html = "";
if (WiFi.status() == WL_CONNECTED) {
  // 如果已连接到 Wi-Fi，直接返回电机控制界面的 HTML
  html = controlHtml();
} else {
  // 获取附近的Wi-Fi网络
  int n = WiFi.scanNetworks();
  html = "<html><body>";
  html += "<select name='network'>";
  html += "<option value='' selected disabled hidden>Select Wi-Fi network</option>";
  for (int i = 0; i < n; ++i) {
  html += "<option value='" + WiFi.SSID(i) + "'>" + WiFi.SSID(i) + "</option>";
  }
  html += "</select>";
  html += "<input type='password' name='password' placeholder='Wi-Fi password'>";
  html += "<button onclick='submitForm()'>Connect</button>";
  // 添加JavaScript函数来提交表单数据
  html += "<script>";
  html += "function submitForm() {";
  html += " var network = document.getElementsByName('network')[0].value;";
  html += " var password = document.getElementsByName('password')[0].value;";
  html += " var xhr = new XMLHttpRequest();";
  html += " xhr.open('POST', '/connect', true);";
  html += " xhr.setRequestHeader('Content-Type', 'application/x-www-form-urlencoded');";
  html += " xhr.onreadystatechange = function() {";
  html += " if (xhr.readyState === XMLHttpRequest.DONE && xhr.status === 200) {";
  html += " var response = JSON.parse(xhr.responseText);";
  html += " if (response.success) {";
  html += " document.documentElement.innerHTML = response.html;";
  html += " } else {";
  html += " alert('Failed to connect to Wi-Fi.');";
  html += " }";
  html += " }";
  html += " };";
  html += " xhr.send('network=' + encodeURIComponent(network) + '&password=' + encodeURIComponent(password));";
  html += "}";
  html += "</script>";
  html += "</body></html>";
}

server.send(200, "text/html", html);
}

void handleConnect() {
String network = server.arg("network");
String password = server.arg("password");

// 切换到 STATION 模式
WiFi.mode(WIFI_STA);

// 连接到 Wi-Fi 网络
WiFi.begin(network.c_str(), password.c_str());

unsigned long startTime = millis();
while (WiFi.status() != WL_CONNECTED && millis() - startTime < 10000) {
delay(500);
}

if (WiFi.status() == WL_CONNECTED) {
String ip = WiFi.localIP().toString();

String html = controlHtml();

server.send(200, "application/json", "{\"success\": true, \"html\": \"" + html + "\", \"ip\": \"" + ip + "\"}");

delay(1000);
WiFi.mode(WIFI_STA);

// 保存 Wi-Fi 名称和密码到 EEPROM
  EEPROM.begin(512);
  int address = 0;
  for (int i = 0; i < network.length(); ++i) {
    EEPROM.write(address, network[i]);
    ++address;
  }
  EEPROM.write(address, '\0');
  ++address;

  for (int i = 0; i < password.length(); ++i) {
    EEPROM.write(address, password[i]);
    ++address;
  }
  EEPROM.write(address, '\0');

  EEPROM.commit();
  EEPROM.end();
} else {
server.send(200, "application/json", "{\"success\": false}");
}
}

void handleToggleMotor() {
String motor = server.arg("motor");

if (motor == "ao") {
  servo2.write(initRad - rad);
  delay(800);
  servo2.write(initRad + rad);
  delay(800);
  servo2.write(initRad);
} else if (motor == "af") {
  servo1.write(initRad - rad);
  delay(800);
  servo1.write(initRad + rad);
  delay(800);
  servo1.write(initRad);
} else if (motor == "lo") {
  servo2.write(initRad - rad);
  delay(800);
  servo2.write(initRad);
} else if (motor == "lf") {
  servo1.write(initRad + rad);
  delay(800);
  servo1.write(initRad);
} else if (motor == "ro") {
  servo2.write(initRad + rad);
  delay(800);
  servo2.write(initRad);
} else if (motor == "rf") {
  servo1.write(initRad - rad);
  delay(800);
  servo1.write(initRad);
} else {
server.send(400, "text/plain", "Invalid motor parameter");
return;
}

server.send(200, "application/json", "{\"success\": false}");
}

void setup() {
servo1.attach(servo1Pin);
servo2.attach(servo2Pin);
servo1.write(initRad);
servo2.write(initRad);

// 从 EEPROM 读取 Wi-Fi 名称和密码
EEPROM.begin(512);
int address = 0;
String storedNetwork = "";
char storedChar = EEPROM.read(address);
while (storedChar != '\0' && address < 512) {
  storedNetwork += storedChar;
  ++address;
  storedChar = EEPROM.read(address);
}
++address;

String storedPassword = "";
storedChar = EEPROM.read(address);
while (storedChar != '\0' && address < 512) {
  storedPassword += storedChar;
  ++address;
  storedChar = EEPROM.read(address);
}

EEPROM.end();

// 如果存储的 Wi-Fi 名称和密码不为空，则连接到 Wi-Fi
if (storedNetwork.length() > 0 && storedPassword.length() > 0) {
WiFi.mode(WIFI_STA);
WiFi.begin(storedNetwork.c_str(), storedPassword.c_str());

unsigned long startTime = millis();
while (WiFi.status() != WL_CONNECTED && millis() - startTime < 10000) {
  delay(500);
}
}

if (WiFi.status() != WL_CONNECTED) {
// 如果连接不成功，则启用热点进入 Wi-Fi 初始化功能
WiFi.mode(WIFI_AP_STA);
WiFi.softAP(ssid, password);
}

server.on("/", handleRoot);
server.on("/connect", handleConnect);
server.on("/toggle", handleToggleMotor);

server.begin();
}

void loop() {
server.handleClient();
}