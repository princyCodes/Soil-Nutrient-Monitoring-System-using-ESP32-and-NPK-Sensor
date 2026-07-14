#include <WiFi.h>
#include <WebServer.h>

/* ---------- RS485 Pins ---------- */
#define RX2_PIN 16
#define TX2_PIN 17
#define DE_PIN  4
#define RE_PIN  5

/* ---------- Modbus Commands ---------- */
const byte nitroCmd[] = {0x01,0x03,0x00,0x1E,0x00,0x01,0xE4,0x0C};
const byte phospCmd[] = {0x01,0x03,0x00,0x1F,0x00,0x01,0xB5,0xCC};
const byte potasCmd[] = {0x01,0x03,0x00,0x20,0x00,0x01,0x85,0xC0};

/* ---------- WiFi Access Point ---------- */
const char* ap_ssid = "ESP32_NPK";
const char* ap_password = "12345678";

/* ---------- Global Variables ---------- */
WebServer server(80);

int N_val = -1;
int P_val = -1;
int K_val = -1;

unsigned long lastRead = 0;

/* ---------- HTML Dashboard ---------- */

const char MAIN_page[] PROGMEM = R"rawliteral(

<!DOCTYPE html>
<html>

<head>

<meta charset="UTF-8">

<meta name="viewport" content="width=device-width, initial-scale=1.0">

<title>NPK Dashboard</title>

<style>

body{
margin:0;
font-family:Segoe UI,Arial,sans-serif;
background:linear-gradient(135deg,#020024,#090979,#020024);
}

.container{
max-width:420px;
margin:auto;
padding:20px;
}

h1{
text-align:center;
color:white;
margin-bottom:25px;
}

.card{
background:white;
padding:18px;
border-radius:16px;
margin-bottom:18px;
display:flex;
justify-content:space-between;
align-items:center;
box-shadow:0 8px 20px rgba(0,0,0,.2);
}

.label{
font-size:18px;
font-weight:bold;
}

.value{
font-size:26px;
font-weight:bold;
}

.n{color:#2980b9;}
.p{color:#8e44ad;}
.k{color:#d35400;}

.unit{
font-size:14px;
color:#777;
}

</style>

</head>

<body>

<div class="container">

<h1>Soil NPK Dashboard</h1>

<div class="card">

<div>

<div class="label">Nitrogen</div>

<div class="unit">mg/kg</div>

</div>

<div id="nval" class="value n">--</div>

</div>

<div class="card">

<div>

<div class="label">Phosphorus</div>

<div class="unit">mg/kg</div>

</div>

<div id="pval" class="value p">--</div>

</div>

<div class="card">

<div>

<div class="label">Potassium</div>

<div class="unit">mg/kg</div>

</div>

<div id="kval" class="value k">--</div>

</div>

</div>

<script>

async function updateData(){

try{

const r=await fetch('/data');

const j=await r.json();

document.getElementById("nval").innerHTML=(j.N<0?"--":j.N)+" mg/kg";

document.getElementById("pval").innerHTML=(j.P<0?"--":j.P)+" mg/kg";

document.getElementById("kval").innerHTML=(j.K<0?"--":j.K)+" mg/kg";

}

catch(e){}

}

updateData();

setInterval(updateData,1000);

</script>

</body>

</html>

)rawliteral";

/* ---------- RS485 Direction ---------- */

void rs485SendEnable(){

digitalWrite(DE_PIN,HIGH);

digitalWrite(RE_PIN,HIGH);

delay(5);

}

void rs485ReceiveEnable(){

digitalWrite(DE_PIN,LOW);

digitalWrite(RE_PIN,LOW);

delay(5);

}

/* ---------- Read Sensor ---------- */

int sendAndRead(const byte *cmd,size_t len){

byte resp[8];

while(Serial2.available()) Serial2.read();

rs485SendEnable();

Serial2.write(cmd,len);

Serial2.flush();

rs485ReceiveEnable();

unsigned long start=millis();

int idx=0;

while(idx<7 && millis()-start<500){

if(Serial2.available())

resp[idx++]=Serial2.read();

}

if(idx<7)

return -1;

return (resp[3]<<8)|resp[4];

}

/* ---------- Web Pages ---------- */

void handleRoot(){

server.send_P(200,"text/html",MAIN_page);

}

void handleData(){

String json="{";

json+="\"N\":"+String(N_val)+",";

json+="\"P\":"+String(P_val)+",";

json+="\"K\":"+String(K_val);

json+="}";

server.send(200,"application/json",json);

}

/* ---------- Setup ---------- */

void setup(){

Serial.begin(115200);

Serial2.begin(9600,SERIAL_8N1,RX2_PIN,TX2_PIN);

pinMode(DE_PIN,OUTPUT);

pinMode(RE_PIN,OUTPUT);

rs485ReceiveEnable();

WiFi.softAP(ap_ssid,ap_password);

server.on("/",handleRoot);

server.on("/data",handleData);

server.begin();

Serial.println("ESP32 NPK Dashboard Ready");

Serial.println("Connect WiFi : ESP32_NPK");

Serial.println("Password : 12345678");

Serial.println("Open Browser : http://192.168.4.1");

}

/* ---------- Loop ---------- */

void loop(){

server.handleClient();

if(millis()-lastRead>2000){

lastRead=millis();

N_val=sendAndRead(nitroCmd,sizeof(nitroCmd));

delay(100);

P_val=sendAndRead(phospCmd,sizeof(phospCmd));

delay(100);

K_val=sendAndRead(potasCmd,sizeof(potasCmd));

Serial.printf("N:%d mg/kg  P:%d mg/kg  K:%d mg/kg\n",N_val,P_val,K_val);

}

}