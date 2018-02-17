
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <IRremoteESP8266.h>
#include <ArduinoJson.h>

int RECV_PIN = D2; //an IR detector/demodulatord is connected to GPIO pin D2
int SEND_PIN = D4; //TX
// Update these with values suitable for your network.
const char* ssid     = "****";
const char* password = "****";
const char* mqtt_server = "*****";
//const char* mqtt_server = "iot.eclipse.org";
const char* subTopic="Fernbedienung/in/#";

WiFiClient espClient;
PubSubClient client(espClient);
IRrecv irrecv(RECV_PIN);
decode_results results;
IRsend irsend(SEND_PIN);

long lastMsg = 0;
char msg[50];
int intSetup = 0;

//#####################################################################################
//Setup und Zeige Verbindung
void setup_wifi() {
   delay(100);
  // We start by connecting to a WiFi network
    Serial.print("Connecting to ");
    Serial.println(ssid);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) 
    {
      delay(500);
      Serial.print(".");
    }
  randomSeed(micros());
  irsend.begin();
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}
//#####################################################################################

//#####################################################################################
//Senden der IR Daten per LED der Übergebene Code
//kommt per MQTT Message und wird per globaler Variable übergeben
void sendIR(long result)
{
  irsend.sendNEC(result, 32);
  delay(100);
  irsend.sendNEC(result, 32);
  delay(100);
  irsend.sendNEC(result, 32);
  delay(1000);
}
//#####################################################################################

//#####################################################################################
//MQTT Callback wird aufgerufen wenn eine MQTT Message empfangen wird
void callback(char* topic, byte* payload, unsigned int length) 
{
  //lokale Definition der Variablen damit diese leer sind beim Start
  char jsonstr[400];
  StaticJsonBuffer<400> jsonBuffer;
  char Code[20];
  // Allocate the correct amount of memory for the payload copy
  byte* p = (byte*)malloc(length);
  // Copy the payload to the new buffer
  memcpy(p,payload,length);
  //client.publish("outTopic", p, length);
  // Free the memory
  free(p);

  //nur wenn man was empfangen will
  //Json Parse
  for (int i = 0; i < length; i++) 
  {
    jsonstr[i] =(char)payload[i];
  }  
  JsonObject& root = jsonBuffer.parseObject(jsonstr);
  if (!root.success()) 
  {
    Serial.println("parseObject() failed");
    //return;
  }
  //Auswertung empfangener Daten
  String value = root["nvalue"];
  String name1 = root["name"];
  if (name1 =="Code" )
  {
    value.toCharArray(Code,20);
    long result = strtol(Code,NULL, 16);
    client.publish("Fernbedienung/out",Code);
    sendIR(result);     
  }
  else if (name1=="Setup")
  {
    if (value == "ein")
    {
      intSetup =1;
    }
    else
    {
      intSetup=0;
    }
  }
} //end callback
//#####################################################################################

//#####################################################################################
//reconnect MQTT
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) 
  {
    //Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    //if you MQTT broker has clientID,username and password
    //please change following line to    if (client.connect(clientId,userName,passWord))
    if (client.connect(clientId.c_str()))
    {
      //Serial.println("connected");
     //once connected to MQTT broker, subscribe command if any
      client.subscribe(subTopic);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
} //end reconnect()
//#####################################################################################

//#####################################################################################
//Setup
void setup() {
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  irrecv.enableIRIn(); // Start the receiver
   if (client.connect("wrfidclient"))
      {
        client.publish("Fernbedienung/out","hello world");
        //client.subscribe("/domoticz/out/#");
        client.subscribe(subTopic);
        Serial.print("MQTT Verbunden mit subTopic: ");
      } 
}
//#####################################################################################

//#####################################################################################
//Empfange daten von Fernbedienung
void recIR()
{
    long now = millis();
    if (now - lastMsg > 200) {
     lastMsg = now;
    if (irrecv.decode(&results)) 
    {
        if (results.decode_type == NEC) {
        Serial.print("NEC: ");
      } else if (results.decode_type == SONY) {
        Serial.print("SONY: ");
      } else if (results.decode_type == RC5) {
        Serial.print("RC5: ");
      } else if (results.decode_type == RC6) {
        Serial.print("RC6: ");
      } else if (results.decode_type == UNKNOWN) {
        Serial.print("UNKNOWN: ");
      }
      Serial.println(results.value, HEX);
      Serial.println(results.decode_type);
      irrecv.resume(); // Receive the next value

      //Senden des IR Codes per MQTT 
      char message[58];
      sprintf(message, "GET /testID=%lu HTTP/1.0", results.value);
      Serial.println(message);
      //publish sensor data to MQTT broker
      client.publish("Fernbedienung/out", message);
     }
  }
}
//#####################################################################################

//#####################################################################################
//Dauerschleife
void loop() {
  if (!client.connected()) {
    reconnect();
  }
  int i=0;
  client.loop();
  //RecIR wird nur aufgerufen wenn per MQTT Setup auf Ein gesetzt wird
  if (intSetup==1) recIR();
}
//#####################################################################################





