/*
* Trabajo fin de grado: Sensor inteligente con comunicación inalámbrica para la medida de concentración de CO2 en interiores.
*
* Autor: Yuhao Huang Zheng.
*
*  Librerias usadas en el código:
*  Sensor CO2:                                      https://github.com/WifWaf/MH-Z19
*  Sensor BME280:                                   https://github.com/adafruit/Adafruit_BME280_Library
*  PubSubClient:                                    https://github.com/knolleary/pubsubclient
*  IotWebConf (portal de configuración):            https://github.com/prampec/IotWebConf
*  
*/

//-----------------------------------------------------------------------------------------------------------------

#include <IotWebConf.h>                                  // Libreria para el portal externo de configuraciones
#include <IotWebConfUsing.h>                             
#include <ESP8266WiFi.h>                                 // Libreria WiFi
#include <PubSubClient.h>                                // Libreria para el cliente MQTT
#include <MHZ19.h>                                       // Libreria del sensor de CO2
#include <SoftwareSerial.h>                              // Libraria para permitir comunicación serie en otros pines
#include <Ticker.h>                                      // Libreria para llamar funciones cada cierto tiempo
#include <Wire.h>                                        // Libreria I2C                                       
#include <Adafruit_BME280.h>                             // Libreria del sensor BME280
#include <Adafruit_Sensor.h> 

//-----------------------------------------------------------------------------------------------------------------

#define STRING_LEN 128                                   // Longitud de la cadena
#define CONFIG_VERSION "Tfg-2021-yhz"                    // Nombre de la versión del portal. Puede ser cualquier otro nombre

// Define con los pines correspondiente al ESP12
#define SCL_ESP12 5                                      // GPIO5 del esp12
#define SDA_ESP12 4                                      // GPIO4 del esp12
#define TX_ESP12  12                                     // GPIO12 del esp12
#define RX_ESP12  14                                     // GPIO14 del esp12 

//-----------------------------------------------------------------------------------------------------------------
// Declaración de los Objetos

WiFiClient espClient;
PubSubClient client(espClient);
SoftwareSerial VirtualSerial(RX_ESP12, TX_ESP12);        // Comunicación serie con los pines del ESP12, RX = GPIO14  TX = GPIO12
Ticker ticker_co2;
Ticker ticker_bme;
DNSServer dnsServer;
WebServer server(80);
MHZ19 sensor_CO2;
Adafruit_BME280 sensor_BME;

//-----------------------------------------------------------------------------------------------------------------

// Crear Access Point inicial. Se puede cambiar estos parámetros en el portal de configuración
const char AcessPoint[] = "ESP8266AP";
const char AP_pass[] = "TFG2021YHZ";

// Parámetros personalizados
char mqtt_server[STRING_LEN];
char mqtt_port[STRING_LEN];
char mqtt_user[STRING_LEN];
char mqtt_password[STRING_LEN];
char mqtt_Id[STRING_LEN];
char mqtt_prefix[STRING_LEN];                           // Prefijo usado en el proyecto tfg/2021/ introducido desde el portal

bool needMqttConnect = false;

// Variables para las lecturas de los sensores
unsigned int lectura_CO2;
float lectura_temp, lectura_hum;

// Variables para almacenar la conversión de entero, float a cadena
char co2String[8];
char tempString[8];
char humString[8];

// Sufijos (suffix) de los temas (topics)
char* sufijo_Control_co2 = "control/sensor/co2";
char* sufijo_Datos_co2 = "datos/sensor/co2";
char* sufijo_Control_bme = "control/sensor/bme";
char* sufijo_temp_bme = "datos/sensor/bme/temperatura";
char* sufijo_hum_bme = "datos/sensor/bme/humedad";

char* topic_CO2_1;                                     // Variable para almacenar topic de control del sensor CO2
char* topic_CO2_2;                                     // Variable para almacenar topic de datos del sensor CO2

char* topic_bme_1;                                     // Variable para almacenar topic de control del sensor BME280
char* topic_bme_2;                                     // Variable para almacenar topic de la temperatura (datos)
char* topic_bme_3;                                     // Variable para almacenar topic de la humedad (datos)

char tmp1[STRING_LEN];                                 // Variable temporal para almacenar el prefijo (prefix) introducido desde el portal y construir los topics
char tmp2[STRING_LEN];                        
char tmp3[STRING_LEN]; 
char tmp4[STRING_LEN]; 
char tmp5[STRING_LEN];

IotWebConf iotWebConf(AcessPoint, &dnsServer, &server, AP_pass, CONFIG_VERSION);

// Parámetros personalizados
IotWebConfParameterGroup group1 = IotWebConfParameterGroup("group1", "Parámetros MQTT");
IotWebConfTextParameter mqttServer = IotWebConfTextParameter("Servidor MQTT", "mqttServer", mqtt_server, STRING_LEN);
IotWebConfTextParameter mqttPort = IotWebConfTextParameter("Puerto MQTT", "mqttPort", mqtt_port, STRING_LEN);
IotWebConfTextParameter mqttId = IotWebConfTextParameter ("MQTT ID", "mqttId", mqtt_Id, STRING_LEN);
IotWebConfTextParameter mqttUser = IotWebConfTextParameter("Usuario MQTT", "mqttUser", mqtt_user, STRING_LEN);
IotWebConfPasswordParameter mqttPass = IotWebConfPasswordParameter("Contraseña MQTT", "mqttPass", mqtt_password, STRING_LEN);
IotWebConfTextParameter mqttPrefix = IotWebConfTextParameter("Prefix", "mqttPrefix", mqtt_prefix, STRING_LEN);

//-----------------------------------------------------------------------------------------------------------------
// Función que gestiona la página inicial del portal de configuración.

void handleRoot(){

  // -- Let IotWebConf test and handle captive portal requests.
  if (iotWebConf.handleCaptivePortal())
  {
    // -- Captive portal request were already served.
    return;
  }
  String s = "<!DOCTYPE html><html lang=\"en\"><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1, user-scalable=no\"/>";
  s += "<title>Home page</title></head><body>";
  s += "<h1>Bienvenido al portal de configuraci&oacute;n</h1>";
  s += "<h2>Trabajo fin de grado. Autor: Yuhao Huang Zheng.</h2>";
  s += "<h3>&Uacute;ltimas configuraciones MQTT: </h3>";
  s += "<ul>";
  s += "<li>Servidor MQTT: ";
  s += mqtt_server;
  s += "<li>Puerto MQTT: ";
  s += mqtt_port;
  s += "<li>ID dispositivo: ";
  s += mqtt_Id;
  s += "</ul>";
  s += "Ir a <a href='config'>configure page</a> para cambiar la configuraci&oacute;n.";
  s += "</body></html>\n";

  server.send(200, "text/html", s);
}

//-----------------------------------------------------------------------------------------------------------------

void configSaved(){

  Serial.println("Configuration was updated.");
}

//-----------------------------------------------------------------------------------------------------------------

bool formValidator(iotwebconf::WebRequestWrapper* webRequestWrapper){

  Serial.println("Validating form.");
  bool valid = true;

  return valid;
}

//-----------------------------------------------------------------------------------------------------------------

void wifiConnected(){
  
  needMqttConnect = true;
  
}

//-----------------------------------------------------------------------------------------------------------------
// Función que realiza la conexión MQTT. El ID del cliente es un parámetro configurable desde el portal más un número hexadecimal aleatorio
// Para la conexión MQTT es necesario los parámetros de ID, usuario y contraseña
// Una vez realizado la conexión se construye los topics según los prefijos y sufijos correspodientes. El prefijo es configurable desde el portal
// Y se procede a la suscripción a los topics de control
// En caso de fallar la conexión MQTT retorna los valores de error y reintenta la conexión MQTT

bool connectMqtt(){
  
    // Realizar la conexión MQTT
    while (!client.connected()) {
    Serial.println("Attempting MQTT connection...");
    String MQTT_ID = (String)mqtt_Id;                                                   // Convertir char a string el parámetro ID
    MQTT_ID += String(random(0xffff), HEX);                                             // ID del cliente = ID_introducido_desde_el_portal + número HEX aleatorio
    
    
    if (client.connect(MQTT_ID.c_str(),mqtt_user, mqtt_password) ) {                    // Pasar parámetros MQTT: Id, usuario, contraseña
      Serial.println("Conectado con el servidor MQTT");
      
      //Una vez conectado... Construir los temas (topics) con el prefijo y sufijo correspondiente
      strcpy(tmp1,mqtt_prefix);                                                         // Copiar el prefijo (prefix) en una variable temporal
      strcpy(tmp2,mqtt_prefix);                                  
      strcpy(tmp3,mqtt_prefix);
      strcpy(tmp4,mqtt_prefix);
      strcpy(tmp5,mqtt_prefix);
      
      // Construcción de los temas (topics)
      topic_CO2_1 = strcat(tmp1, sufijo_Control_co2);                                   // Se construye el topic: tfg/2021/control/sensor/co2 
      topic_CO2_2 = strcat(tmp2, sufijo_Datos_co2);                                     // Se construye el topic: tfg/2021/datos/sensor/co2
      topic_bme_1 = strcat(tmp3, sufijo_Control_bme);                                   // Se construye el topic: tfg/2021/control/sensor/bme
      topic_bme_2 = strcat(tmp4, sufijo_temp_bme);                                      // Se construye el topic: tfg/2021/datos/sensor/bme/temperatura
      topic_bme_3 = strcat(tmp5, sufijo_hum_bme);                                       // Se construye el topic: tfg/2021/datos/sensor/bme/humedad

      //Suscribirse a los temas (topics) de control
      client.subscribe(topic_CO2_1);                                                    // Suscrito a tfg/2021/control/sensor/co2
      client.subscribe(topic_bme_1);                                                    // Suscrito a tfg/2021/control/sensor/bme

    //En caso de fallar la conexión MQTT retorna valores de error e intentar de nuevo la conexión 
    }else{
      Serial.print("Fallido, rc = ");
      Serial.print(client.state());                                                     // Valores de retornos en https://pubsubclient.knolleary.net/api.html#state
      Serial.println(" Intentar de nuevo en 2s");
      delay(2000);
    }
   
  }
   
  // Si se ha conectado devolver true
  return true;

}

//-----------------------------------------------------------------------------------------------------------------
//Función que agrega los parámetros personalizados al portal de configuración

void addConfig(){
  
  group1.addItem(&mqttServer);
  group1.addItem(&mqttPort);
  group1.addItem(&mqttId);
  group1.addItem(&mqttUser);
  group1.addItem(&mqttPass);
  group1.addItem(&mqttPrefix);

  iotWebConf.addParameterGroup(&group1);
  
  iotWebConf.setConfigSavedCallback(&configSaved);
  iotWebConf.setFormValidator(&formValidator);
  iotWebConf.setWifiConnectionCallback(&wifiConnected);
  
  // Inicializar las configuraciones
  bool validConfig = iotWebConf.init();
  if (!validConfig){
    mqtt_server[0] = '\0';
    mqtt_port[0] = '\0';
    mqtt_user[0] = '\0';
    mqtt_password[0] = '\0';
    mqtt_Id[0] = '\0';
    mqtt_prefix[0] = '\0';
  }

  // Set up required URL handlers on the web server.
  server.on("/", handleRoot);
  server.on("/config", []{ iotWebConf.handleConfig(); });
  server.onNotFound([](){ iotWebConf.handleNotFound(); });

}

//-----------------------------------------------------------------------------------------------------------------
// Función que lee el sensor de CO2. Para poder publicar el valor del sensor en el topic hay que convertirlo a cadena

void leer_co2(){

  lectura_CO2 = sensor_CO2.getCO2();
  sprintf(co2String, "%d", lectura_CO2);                // Convertir entero a cadena
  client.publish(topic_CO2_2, co2String);               
}

//-----------------------------------------------------------------------------------------------------------------
// Función que lee el sensor BME280, temperatura y humedad. Para poder publicar los valores del sensor hay que convertirlo a cadena

void leer_bme(){
  lectura_temp = sensor_BME.readTemperature();
  dtostrf(lectura_temp, 1, 2, tempString);             // Convertir float a cadena
  client.publish(topic_bme_2, tempString);

  lectura_hum = sensor_BME.readHumidity();
  dtostrf(lectura_hum, 1, 2, humString);
  client.publish(topic_bme_3, humString);
}

//-----------------------------------------------------------------------------------------------------------------
// Función que gestiona los mensajes entrantes de los topics suscritos
// Para el control de los sensores de CO2 y BME280 se utiliza las propiedades attach y detach de los Tickers

void callback(char* topic, byte* payload, unsigned int length){
  
  Serial.print("Mensaje llegado en el topic: ");
  Serial.println(topic);
  for (int i = 0; i < length; i++) {
      Serial.print((char)payload[i]);
    }
    Serial.println();
    
  // Buscar dentro del topic la primera ocurrencia  
  if (strstr(topic, topic_CO2_1)){
    
    // Switch ON. Leer sensor cada 2 segundo...
    if ((char)payload[0] == '1'){ticker_co2.attach(2,leer_co2);}
    
    // Switch OFF. Desactivar lectura. Limpiar registro de CO2
    else {   
      ticker_co2.detach();
      client.publish(topic_CO2_2, "---");
    }
  }

  // Buscar dentro del topic la primera ocurrencia
  if(strstr(topic, topic_bme_1)){
    
     // Switch ON. Leer sensor cada 2 segundo...
    if ((char)payload[0] == '1'){ticker_bme.attach(2,leer_bme);}
      
    // Switch OFF. Desactivar lectura. Limpiar registro de la temperatura y humedad                
    else {   
      ticker_bme.detach();
      client.publish(topic_bme_2, "---");                 
      client.publish(topic_bme_3, "---");                 
    }

    
  }
 
}

//-----------------------------------------------------------------------------------------------------------------
// Función setup dónde se gestiona la inicialización de las comunicaciones entre dispositivos
// Y la gestión del servidor MQTT

void setup(){

  Serial.begin(115200);
  VirtualSerial.begin(9600);                                          // Comunicación serie entre micro y sensor a 9600 baudios
  sensor_CO2.begin(VirtualSerial);
  addConfig();                                                        // Llamada a la función addConfig
  
  Wire.begin(SDA_ESP12, SCL_ESP12);                                   // Inicializar los pines 4 y 5 como SDA SCL
  
  if(!sensor_BME.begin() ){                                           // Comprobación de la conexión con el sensor BME280
    Serial.println("Sensor BME280 no encontrado.");                 
    while(1);
  }
  
  // Servidor MQTT. Convertir el puerto de cadena a entero
  client.setServer(mqtt_server, atoi(mqtt_port));
  client.setCallback(callback);

  
}

//-----------------------------------------------------------------------------------------------------------------
// Función loop dónde se gestiona la reconexión de MQTT

void loop(){

  
  iotWebConf.doLoop();
  client.loop();
  
  if (needMqttConnect){
  
    if (connectMqtt()){
    
      needMqttConnect = false;
    }
  }
  else if ((iotWebConf.getState() == IOTWEBCONF_STATE_ONLINE) && (!client.connected())) {
  
    Serial.println("MQTT reconnect");
    connectMqtt();
  }
  
  
}

//-----------------------------------------------------------------------------------------------------------------
