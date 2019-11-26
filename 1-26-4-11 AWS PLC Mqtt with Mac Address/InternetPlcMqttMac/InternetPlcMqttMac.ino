#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <time.h>
#include <ArduinoJson.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>

//json을 위한 설정
StaticJsonDocument<200> doc;
DeserializationError error;
JsonObject root;

char ssid[32] = "";
char password[32] = "";

// AP 설정을 위한 변수
const char *softAP_ssid = "ap_";
const char *softAP_password = "00000000";
/* Soft AP network parameters */
IPAddress apIP(192, 168, 4, 1);
IPAddress netMsk(255, 255, 255, 0);
boolean connect;
unsigned long lastConnectTry = 0;
String sAP_ssid; // mac address를 문자로 기기를 구분하는 기호로 사용
char cAP_ssid[40];

//mqtt 통신 변수
const char *thingId = "";          // 사물 이름 (thing ID) 
const char *host = "a8i4lgiqa43pw-ats.iot.us-west-2.amazonaws.com"; // AWS IoT Core 주소
const char* outTopic = "/kdi/outTopic"; // 이름이 중복되지 않게 설정 기록
const char* inTopic = "/kdi/inTopic"; // 이름이 중복되지 않게 설정 기록
const char* clientName = "";  // setup 함수에서 자동생성
String sChipID; // mac address를 문자로 기기를 구분하는 기호로 사용
char cChipID[40];
int value=0;
//WiFiClient espClient;
//PubSubClient client(espClient);

//plc 제어 변수
String s="";
int P4[4]={0};
int P0[6]={0};
int lastRead=0,lastMqtt=0,autoRead=0;
boolean stringComplete = false; 
String inputString;
char msg[100];

ESP8266WebServer server(80);

// 사물 인증서 (파일 이름: xxxxxxxxxx-certificate.pem.crt)
const char cert_str[] PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
MIIDWjCCAkKgAwIBAgIVAOQ3aNyDDRz1urVllhcEPX7S8wJeMA0GCSqGSIb3DQEB
CwUAME0xSzBJBgNVBAsMQkFtYXpvbiBXZWIgU2VydmljZXMgTz1BbWF6b24uY29t
IEluYy4gTD1TZWF0dGxlIFNUPVdhc2hpbmd0b24gQz1VUzAeFw0xOTA4MDIwNTI5
MTRaFw00OTEyMzEyMzU5NTlaMB4xHDAaBgNVBAMME0FXUyBJb1QgQ2VydGlmaWNh
dGUwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQC7F1eU1Vtab4MHXzlx
oH0DJelGYVFgDIja/d1VOb9vP/kjUKwBTINsJuccFGyXC1NFK23sVwHqwgPyyPl3
6ssUYMsoHtyZ3D20Rw5LhDV3QfFK6BfI9oCKOfNdmzEEMCg9OkOPmtxcpjuNCF18
tmpY8Fs2rP8kGWtf1YPQLPQl+quhoFDkuxkWkFrS/K1JhL4SF7KMuQ7BAHz6HZjt
sKLWVkg4h46HXgRwnW2O/EJo46U6SyYfiZkmLVdZzprM8Mkt25/vNxffqzz7FPbP
+vYIU58koyOT8PGSRjxyntuFbbm8rJBitwGu1/7IqRqkb3jyqcX3HqNQD1dTSjbr
XblLAgMBAAGjYDBeMB8GA1UdIwQYMBaAFOysbIH6lXFKH8JvdqBaIoMleEupMB0G
A1UdDgQWBBQhVKbhQ/7CzPtoGVm8xQSBaTPlQDAMBgNVHRMBAf8EAjAAMA4GA1Ud
DwEB/wQEAwIHgDANBgkqhkiG9w0BAQsFAAOCAQEAm1I5OkT/hAelO9fns3WJC5jg
DE4p+yWnoTVZs5aKli7nXNrKuMStq8sJ0hwLUjBW1/SDWs+qq1Er9XsXRZ2ssOzO
AGiZlYJBKMzlx5T4S7pUow4AMceUaXmN0MijD20GxEfcCtdbYxqamVk8wNoq5T2q
2C72J9fylSWjz8TwJb4PVgv77gs9QraTCZWt+Lo3IftgJJHOM0R9Cv8+/BMo/yVu
uhJYX28+7PORM0FHMFESTGYiccyii1Z6Cj2otg59a770RU+GlDJ0t80xM3THavPa
Wa69k6H+F6gGOz9EUMKsGHvR97ZfX3gUHDkV0MsrOx78u7KO6ASTyLHq2caDoA==
-----END CERTIFICATE-----
)EOF";
// 사물 인증서 프라이빗 키 (파일 이름: xxxxxxxxxx-private.pem.key)
const char key_str[] PROGMEM = R"EOF(
-----BEGIN RSA PRIVATE KEY-----
MIIEpAIBAAKCAQEAuxdXlNVbWm+DB185caB9AyXpRmFRYAyI2v3dVTm/bz/5I1Cs
AUyDbCbnHBRslwtTRStt7FcB6sID8sj5d+rLFGDLKB7cmdw9tEcOS4Q1d0HxSugX
yPaAijnzXZsxBDAoPTpDj5rcXKY7jQhdfLZqWPBbNqz/JBlrX9WD0Cz0JfqroaBQ
5LsZFpBa0vytSYS+EheyjLkOwQB8+h2Y7bCi1lZIOIeOh14EcJ1tjvxCaOOlOksm
H4mZJi1XWc6azPDJLduf7zcX36s8+xT2z/r2CFOfJKMjk/DxkkY8cp7bhW25vKyQ
YrcBrtf+yKkapG948qnF9x6jUA9XU0o26125SwIDAQABAoIBAARm6jKgSoP4N7cG
sI1R318hlzmGtKlz4gx1CK4mq7Bsauo/zaxCJp121N0+RcfQBmeMPAvhiDQD2J/v
xp7hsWGLXXxWLY6ZNgJ14Yo5VCC4NnsytsyNsDyQXH+JVT/p+ihmpIxOcnzjlGcf
GUQD7sCk9yB0NZSd3H7mwTE2vY/fKbowRxSDBjpfBo07qIRu8s+1yBYB/qjeMk28
Ckt/zd80AfUcxcmK98hIvYucCdbQuB9DAdf/Ii7ogZ5549RujF859j+ZAkDrUDVM
ObmrGItBX9m1Bo3MKJpSYUkvt2WyGMLduMMN3/rANbzvlWMEww1NfBChQa3xAszn
j/wSeDECgYEA2t5MOGR6xJVMCKeijr2PaRtorVTX2QyRu0RntNvDQ1hN2hijVGBU
Jcmzl7gHIWdeKQ87z0A4zGPv6+uMkTCKcw7ZOhX/Lt+svOIR1IVSxMsWyuEtWpgk
34s5fcCMKeVH3+xzDbloQHDMQKPr8OMQkEwTE4ZEIQUQTS6M+C6XU80CgYEA2tTs
9C2OkV4x59/x5r/YPPKZzc2dXRLrvWef+LuU0t+r7m/KgtDqcDVFLHFsa+eHp1YJ
gt3TQp/s+rUQLmneEW1Qg3kNGic/D2+aCmcTZ6KFsY1b18nfmBcoCXnK4IpRRZC5
vPiO/4+yAwRP3p18csRiAhxMA5aV/jRI4n3x2XcCgYBEwYjYPliC4RPdtCvRA6PF
tSKRMRIN0XEGj1q06kTRJzBtz0ef/hLxPAf7JYi36a0e4PY9CUPOkaaRSpICGmmj
84oyQS7FHuc4/xvy019JCzPB0DNbPmh3AVmyM4bUsn59zd3m9dPWSpkfxV8dcXeG
nR9xBsDoTprsZC9M1/YaEQKBgQCFDgIlVa3h5YwimJ/U1cD9DeVGpUaRWPfNajxy
WFvc+/LmP0K615w0La+pni12TojEziNqVsYlNGg65+y2y9gicH0L84Zr6IEHOaxH
tvSthrwTgQfe5Pf22d2WJV+dt3xC+AR3SgpoiU6LVwvZJ5iLj6yn69ysMdtxfQjz
7fAncQKBgQC2I45sOS6xVx1+KOhQ+3QvST4O2hrw3sedA3vlorS7Wg0vajSZFELo
qjKDU8njRc8Lgdx/K71XIlo+CoBoLflxUHvmYpCN1gycyWJMvgKbnApVwttJm30z
zJQkfoMU4uPy/L2JJRyIpLKKxlVAqhYpw0koEMwTI7+cJsFe5BmNEw==
-----END RSA PRIVATE KEY-----

)EOF";
// Amazon Trust Services(ATS) 엔드포인트 CA 인증서 (서버인증 > "RSA 2048비트 키: Amazon Root CA 1" 다운로드)
const char ca_str[] PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
MIIDQTCCAimgAwIBAgITBmyfz5m/jAo54vB4ikPmljZbyjANBgkqhkiG9w0BAQsF
ADA5MQswCQYDVQQGEwJVUzEPMA0GA1UEChMGQW1hem9uMRkwFwYDVQQDExBBbWF6
b24gUm9vdCBDQSAxMB4XDTE1MDUyNjAwMDAwMFoXDTM4MDExNzAwMDAwMFowOTEL
MAkGA1UEBhMCVVMxDzANBgNVBAoTBkFtYXpvbjEZMBcGA1UEAxMQQW1hem9uIFJv
b3QgQ0EgMTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBALJ4gHHKeNXj
ca9HgFB0fW7Y14h29Jlo91ghYPl0hAEvrAIthtOgQ3pOsqTQNroBvo3bSMgHFzZM
9O6II8c+6zf1tRn4SWiw3te5djgdYZ6k/oI2peVKVuRF4fn9tBb6dNqcmzU5L/qw
IFAGbHrQgLKm+a/sRxmPUDgH3KKHOVj4utWp+UhnMJbulHheb4mjUcAwhmahRWa6
VOujw5H5SNz/0egwLX0tdHA114gk957EWW67c4cX8jJGKLhD+rcdqsq08p8kDi1L
93FcXmn/6pUCyziKrlA4b9v7LWIbxcceVOF34GfID5yHI9Y/QCB/IIDEgEw+OyQm
jgSubJrIqg0CAwEAAaNCMEAwDwYDVR0TAQH/BAUwAwEB/zAOBgNVHQ8BAf8EBAMC
AYYwHQYDVR0OBBYEFIQYzIU07LwMlJQuCFmcx7IQTgoIMA0GCSqGSIb3DQEBCwUA
A4IBAQCY8jdaQZChGsV2USggNiMOruYou6r4lK5IpDB/G/wkjUu0yKGX9rbxenDI
U5PMCCjjmCXPI6T53iHTfIUJrU6adTrCC2qJeHZERxhlbI1Bjjt/msv0tadQ1wUs
N+gDS63pYaACbvXy8MWy7Vu33PqUXHeeE6V/Uq2V8viTO96LXFvKWlJbYK8U90vv
o/ufQJVtMVT8QtPHRh8jrdkPSHCa2XV4cdFyQzR1bldZwgJcJmApzyMZFo6IQ6XU
5MsI+yMRQ+hDKXJioaldXgjUkK642M4UwtBV8ob2xJNDd2ZhwLnoQdeXeGADbkpy
rqXRfboQnoZsG4q5WTP468SQvvG5
-----END CERTIFICATE-----
)EOF";

// 통신에서 문자가 들어오면 이 함수의 payload 배열에 저장된다.
void callback(char* topic, byte* payload, unsigned int length) {
  /*
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
    Serial1.print((char)payload[i]);
  }
  Serial.println();
  */
  deserializeJson(doc,payload);
  root = doc.as<JsonObject>();
  const char* rChip = root["chip"];
  const char* prt = root["prt"];
  if(!rChip)
    return;
  Serial.println(prt);
  if(strcmp(rChip,cChipID)==0)
    Serial1.print(prt);
}

X509List ca(ca_str);
X509List cert(cert_str);
PrivateKey key(key_str);
WiFiClientSecure wifiClient;
PubSubClient client(host, 8883, callback, wifiClient); //set  MQTT port number to 8883 as per //standard

// mqtt 통신에 지속적으로 접속한다.
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(thingId)) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish(outTopic, "hello world");
      // ... and resubscribe
      client.subscribe(inTopic);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");

      char buf[256];
      wifiClient.getLastSSLError(buf,256);
      Serial.print("WiFiClientSecure SSL error: ");
      Serial.println(buf);

      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

// Set time via NTP, as required for x.509 validation
void setClock() {
  configTime(3 * 3600, 0, "pool.ntp.org", "time.nist.gov");

  Serial.print("Waiting for NTP time sync: ");
  time_t now = time(nullptr);
  while (now < 8 * 3600 * 2) {
    delay(500);
    Serial.print(".");
    now = time(nullptr);
  }
  Serial.println("");
  struct tm timeinfo;
  gmtime_r(&now, &timeinfo);
  Serial.print("Current time: ");
  Serial.print(asctime(&timeinfo));
}


void setup(void) {
  Serial.begin(9600);
  Serial1.begin(9600);
  Serial.setDebugOutput(true);
  Serial.println();

  //이름 자동으로 생성
  uint8_t chipid[6]="";
  WiFi.macAddress(chipid);
  sprintf(cChipID,"%02x%02x%02x%02x%02x%02x%c",chipid[5], chipid[4], chipid[3], chipid[2], chipid[1], chipid[0],0);
  sChipID=String(cChipID);
  sAP_ssid=String(softAP_ssid)+sChipID;
  sAP_ssid.toCharArray(cAP_ssid,sAP_ssid.length()+1);
  softAP_ssid=&cAP_ssid[0];
  setupAp();
  Serial.println(sChipID);
  Serial.println(softAP_ssid);
  
  server.on("/", handleRoot);
  server.on("/onoffP40", handleOnOffP40);
  server.on("/onoffP41", handleOnOffP41);
  server.on("/onoffP42", handleOnOffP42);
  server.on("/onoffP43", handleOnOffP43);
  server.on("/read", handleRead);
  server.on("/wifi", handleWifi);
  server.on("/wifisave", handleWifiSave);
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("HTTP server started");

  loadCredentials(); // Load WLAN credentials from network
  if(strlen(ssid) > 0) { // EEPROM에 와이파이 이름 저장 되어 있으면 WLAN 연결
    connectWifi();
  }
}

void setupAp() {
  Serial.println("");
  Serial.println(softAP_ssid);
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(apIP, apIP, netMsk);
  WiFi.softAP(softAP_ssid, softAP_password);
  delay(500); // Without delay I've seen the IP address blank
  Serial.print("AP IP address: ");
  Serial.println(WiFi.softAPIP());
}

void connectWifi() {
  Serial.println("Connecting as wifi client...");
  WiFi.disconnect();
  //WiFi.mode(WIFI_STA); //이 모드를 설정 않해야 AP가 살아있습니다.
  WiFi.begin(ssid, password);

  int connRes = WiFi.waitForConnectResult();
  Serial.print ( "connRes: " );
  Serial.println ( connRes );
  if(connRes == WL_CONNECTED){
    wifiClient.setTrustAnchors(&ca);
    wifiClient.setClientRSACert(&cert, &key);
    setClock();
    client.setCallback(callback);
    //Serial.println("Certifications and key are set");
    Serial.println("AWS connected");
    Serial.println(WiFi.localIP());
  }
  else
    WiFi.disconnect();
}


void loop(void) {
  unsigned int sWifi = WiFi.status();
  if (!client.connected() && sWifi==WL_CONNECTED) {
    reconnect();
  }
  client.loop();
  server.handleClient();

  long now = millis();
  //6초에 한번 와이파이 끊기면 다시 연결
  if (sWifi == WL_IDLE_STATUS && now > (lastConnectTry + 60000) && strlen(ssid) > 0 ) {
    lastConnectTry=now;
    Serial.println ( "Connect requested" );
    connectWifi();
  }
  
  if ((now - lastRead > 1000)){
    lastRead = now;
    serialEvent();
    if(autoRead==1) {
      String s="";
      s=char(5);
      s+="00RSS0104%PW0";
      s+=char(4);
      Serial1.print(s);
    }
  }
}

void serialEvent() {
  // print the string when a newline arrives:
  if (stringComplete) {
    Serial.println(inputString);
    inputString.toCharArray(msg, inputString.length());
    if(msg[3]=='R') {
        byte inC=msg[13];
        byte inC1=msg[12];
        P0[0]=bitRead(inC,0);
        P0[1]=bitRead(inC,1);
        P0[2]=bitRead(inC,2);
        P0[3]=bitRead(inC,3);
        P0[4]=bitRead(inC1,0);
        P0[5]=bitRead(inC1,1);
        GoHome();
    }
    inputString="{\"chip\":\""+sChipID+"\",\"prt\":\""+inputString+"\"} ";
    inputString.toCharArray(msg, inputString.length());
    client.publish(outTopic, msg);
    // clear the string:
    inputString = "";
    stringComplete = false;
    
  }
  
  if(Serial.available() == false) 
    return;
  while (Serial.available()) {
    // get the new byte:
    char inChar = (char)Serial.read();
    //Serial.print(inChar);
    // add it to the inputString:
    inputString += inChar;
  }
  stringComplete = true;
}

void plcOut() {
  //int out=P40+P41*2+P42*4+P43*8;
  String out =  String(P4[0]+P4[1]*2+P4[2]*4+P4[3]*8, HEX);
  String s="";
  s=char(5);
  s+="00WSS0104%PW4000";
  s+=out;
  s+=char(4);
  Serial1.print(s);
}
