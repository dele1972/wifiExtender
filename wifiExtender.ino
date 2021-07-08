// NAPT example released to public domain
// Based on `ExampleRangeExtender-NAPT` example

#include <Dele72.h>           // provides some 'secret' constants @TODO: you can delete this line

const uint16_t deviceID = 3;                                  // Device/Placement ID (0 to +65,535)
long rssi;                     // signal strength of current wlan connection in dBm


#if LWIP_FEATURES && !LWIP_IPV6

#define HAVE_NETDUMP 0

#ifndef STASSID
#define STASSID WlanConstants::SSID_DEVA
#define STAPSK  WlanConstants::WIFIPASSWORD
#endif

#ifndef APSSID
#define APSSID WlanConstants::SSID_DEVADEV
#define APPSK  WlanConstants::WIFIPASSWORD
#endif

#include <ESP8266WiFi.h>      // to create a WiFi Client (https://arduino-esp8266.readthedocs.io/en/latest/esp8266wifi/readme.html)
#include <WiFiClientSecure.h> // see expl.: https://github.com/espressif/arduino-esp32/blob/master/libraries/WiFiClientSecure/examples/WiFiClientSecure/WiFiClientSecure.ino
//WiFiClientSecure client;       // ssl supporting client (ex. from https://arduino-esp8266.readthedocs.io/en/2.5.2/esp8266wifi/client-secure-examples.html)
BearSSL::WiFiClientSecure client;

/* HOST Connection Settings */
const char* host                           = HostConstants::DATA_HOST;
const char* url                            = HostConstants::URL_DATAHOME;
unsigned short port                        = HostConstants::SSL_PORT;                      // Port to start with SSL (https) connection
//unsigned short port                        = 443;                         // Port to start with SSL (https) connection
//#define MYROOT_CA ROOT_CA
unsigned char MYROOT_CA[sizeof(ROOT_CA)-1] = {};                          // this will be filled later with my root certificate (ssl) and consumes much less heap than #define!
const char *ssid = STASSID;
const char *pass = STAPSK;



#include <lwip/napt.h>        // Network Address and Port Translation (https://github.com/esp8266/Arduino/blob/master/tools/sdk/lwip2/include/lwip/napt.h)
#include <lwip/dns.h>
#include <dhcpserver.h>

// #define NAPT 1000             // set max 1.000 (IP) entries in the NAPT table
// #define NAPT_PORT 10          // set max 10 portmap entries in the NAPT table
#define NAPT IP_NAPT_MAX             // set max 512 (IP) entries in the NAPT table
#define NAPT_PORT IP_PORTMAP_MAX          // set max 32 portmap entries in the NAPT table

#if HAVE_NETDUMP
  #include <NetDump.h>
  void dump(int netif_idx, const char* data, size_t len, int out, int success) {
    (void)success;
    Serial.print(out ? F("out ") : F(" in "));
    Serial.printf("%d ", netif_idx);
  
    // optional filter example: if (netDump_is_ARP(data))
    {
      netDump(Serial, data, len);
      //netDumpHex(Serial, data, len);
    }
  }
#endif

void setup() {
  Serial.begin(115200);

  Serial.printf("Heap before copy CA: %d\n", ESP.getFreeHeap());
  copyCertificate();
  Serial.printf("Heap after copy CA: %d\n\n\n", ESP.getFreeHeap());


  Serial.printf("Heap before get Time: %d\n", ESP.getFreeHeap());
  getCurrentTime();
  Serial.printf("Heap before get Time: %d\n\n\n", ESP.getFreeHeap());
  
  Serial.printf("\n\nNAPT Range extender\n");
  Serial.printf("Heap on start: %d\n", ESP.getFreeHeap());

#if HAVE_NETDUMP
  phy_capture = dump;
#endif


  Serial.printf("Heap before WiFi Station connect: %d\n", ESP.getFreeHeap());
  connectWifiStation();
  Serial.printf("Heap after WiFi Station connect: %d\n\n\n", ESP.getFreeHeap());


  // give DNS servers to AP side
  dhcps_set_dns(0, WiFi.dnsIP(0));
  dhcps_set_dns(1, WiFi.dnsIP(1));


  Serial.printf("Heap before start access point: %d\n", ESP.getFreeHeap());
  startAccessPoint();
  Serial.printf("Heap after start access point: %d\n\n\n", ESP.getFreeHeap());

  
  // Webserver connection disabled
  Serial.printf("Heap before Webserver connection: %d\n", ESP.getFreeHeap());
  connectWebserver();
  Serial.printf("Heap after Webserver connection: %d\n\n\n", ESP.getFreeHeap());

  
  //Serial.printf("Heap before start NAPT routing: %d\n", ESP.getFreeHeap());
  //startNaptRouting();
  //Serial.printf("Heap after start NAPT routing: %d\n", ESP.getFreeHeap());
}

#else

void setup() {
  Serial.begin(115200);
  Serial.printf("\n\nNAPT not supported in this configuration\n");
}

#endif

void loop() {
  //connectWebserver();

/*
  if (client.connected()) {
    sendDataToHost();
  }
  
*/
  /* if the server disconnected, stop the client */

  if (!client.connected()) {
      Serial.println();
      Serial.println("Server disconnected");
      client.stop();
  }

  //yield();

  /* shut down to deepsleep mode (NodeMCU: GPIO16[labeledD0] connect to RST)) */
/*
  Serial.println("\n\n   *** Shut Down *** \n\n");
  */
  delay(5 * 60000);                 // execute once every 5 minutes, don't flood remote service
  /*delay(1 * 60000);                 // execute once every minute, don't flood remote service
  // ESP.deepSleep(5 * 60 * 1000000);  // deepsleep for 5 Minutes
*/
}

void getCurrentTime () {
  /* Get current time */
  // Synchronize time useing SNTP. This is necessary to verify that
  // the TLS certificates offered by the server are currently valid.
  Serial.print("Setting time using SNTP");
  configTime(8 * 3600, 0, "0.de.pool.ntp.org", "3.de.pool.ntp.org");  // https://www.pool.ntp.org/zone/de
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


void connectWifiStation() {
  
  // first, connect to station (STA) so we can get a proper local DNS server
  WiFi.mode(WIFI_STA);
  WiFi.begin(STASSID, STAPSK);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(500);
  }
  // @TODO - copy this output to 'wissen-sammler'
  Serial.printf("\nSTA: %s (dns: %s / %s)\n",
                WiFi.localIP().toString().c_str(),
                WiFi.dnsIP(0).toString().c_str(),
                WiFi.dnsIP(1).toString().c_str());
  // Measure Signal Strength (RSSI) of Wi-Fi connection
 rssi = WiFi.RSSI();

}


void startAccessPoint() {
  
  // start AP
  WiFi.softAPConfig(  // enable AP, with android-compatible google domain
    IPAddress(172, 217, 28, 254),
    IPAddress(172, 217, 28, 254),
    IPAddress(255, 255, 255, 0));
  WiFi.softAP(APSSID, APPSK);
  Serial.printf("AP: %s\n", WiFi.softAPIP().toString().c_str());
}


void startNaptRouting() {

  // start Network Address Port Translation (NAPT)
  Serial.printf("Heap before: %d\n", ESP.getFreeHeap());
  // Heap before: 12504

  // @TODO: ip_napt_init() will return -1 if connectWebserver() is called before :(
  err_t ret = ip_napt_init(NAPT, NAPT_PORT);
  Serial.printf("ip_napt_init(%d,%d): ret=%d (OK=%d)\n", NAPT, NAPT_PORT, (int)ret, (int)ERR_OK);
  // ip_napt_init(1000,10): ret=-1 (OK=0)

  
  if (ret == ERR_OK) {
    ret = ip_napt_enable_no(SOFTAP_IF, 1);
    Serial.printf("ip_napt_enable_no(SOFTAP_IF): ret=%d (OK=%d)\n", (int)ret, (int)ERR_OK);
    if (ret == ERR_OK) {
      Serial.printf("WiFi Network '%s' with same password is now NATed behind '%s'\n", APSSID, STASSID);
    }
  }
  Serial.printf("Heap after napt init: %d\n", ESP.getFreeHeap());
  // Heap after napt init: 12504
  if (ret != ERR_OK) {
    Serial.printf("NAPT initialization failed\n");
    // NAPT initialization failed
  }

}

void copyCertificate() {
  
  /* copy Dele72.ROOT_CA to local MYROOT_CA */
  for (int i = 0; i < sizeof(ROOT_CA); i++) {
    MYROOT_CA[i] = ROOT_CA[i]; //copies UserInput in reverse to TempInput
    // printf("%c", MYROOT_CA[i]);
  }
  MYROOT_CA[sizeof(ROOT_CA)] = '\0'; // adds NULL character at end  
  //MYROOT_CA[sizeof(MYROOT_CA)] = '\0'; // adds NULL character at end  

  }

void connectWebserver() {

  /*
  // print copied certificate to Serial Monitor
  for (int i = 0; i < sizeof(MYROOT_CA); i++) {
    printf("%c", MYROOT_CA[i]);
  }
  */
  // copyCertificate();


  /* Connect to Server */
  Serial.println(String("\n\nStarting connection to server...") + host);
  //Serial.print("\n\n\nCount of MYROOT_CA: "); Serial.println(sizeof(MYROOT_CA)-1);
  //for (int i = 0; i < sizeof(MYROOT_CA); i++) {
  //  printf("%c", MYROOT_CA[i]);
  //}

  
  // client.setInsecure();  // https://github.com/esp8266/Arduino/issues/4826#issuecomment-491813938 BUT THEN THE CONNECTION BECOMES INSECURE!
  // client.setFingerprint(fingerprint);  // The use of validating by Fingerprint isn't recommended because the fingerprint will be renewed more often than the certificate and then the fingerprint has to be updated
  client.setCACert(MYROOT_CA, sizeof(MYROOT_CA)-1); // https://stackoverflow.com/a/56203388
  Serial.println("\n\nCONNECTING...");
int returnVal = 0;
  returnVal = client.connect(host, port);
  Serial.print("\n\n... returnVal: "); Serial.println(returnVal);
  //if (!client.connect(host, port)) {
  if (!returnVal) {
    
    Serial.println("connection failed");
    
    // if connection is failed try again in 5 Seconds
    delay(5000);
    return;
  }

  Serial.println("Connected to server!");


  /* Verify Server */

  // Verify validity of server's certificate
  if (client.verifyCertChain(host)) {

    Serial.println("Server certificate verified");
  } else {

    Serial.println("ERROR: certificate verification failed!");
    return;
  }
  /*
  if (client.verify(fingerprint, host)) {
  
    Serial.println("certificate matches");

  } else {

    Serial.println("certificate doesn't match");

  }
  */

}


void sendDataToHost() {
  
  /* Set Data to send */

  // Create manually a POST request (like https://developer.mozilla.org/en-US/docs/Web/HTTP/Methods/POST)
  // Instead the ESP8266HTTPClient.h could be used
  
  //    char* localip = (String) WiFi.localIP();
  //        + "&ip="            + (String) WiFi.localIP()
  String DataToSend = "DEVICE="           + (String) deviceID
                    + "&SSID="            + (String) STASSID
                    + "&RSSI="            + (String) rssi
                    + "&HUMIDITY="        + (String) STASSID
                    + "&TEMPERATURE="     + (String) rssi
                    + "&PRESSURE=bla"
                    + "&APPROXALTITUDE=bla"
                    + "&VOLTAGESOLAR=bla"
                    + "&VOLTAGEACCU=bla";

  /* create HTTP POST request */
  
  client.println(String("POST ") + url + " HTTP/1.1");
  client.println(String("Host: ") + host);                 
  client.println("Content-Type: application/x-www-form-urlencoded");
  client.println("User-Agent: ESP8266/1.0");
  client.println(String("Content-Length: ") + DataToSend.length());
  client.println("Connection: close"); 
  client.println(""); 
  client.print(DataToSend);
  
  Serial.println("\n");
  Serial.println("My data string im POSTing looks like this: ");
  Serial.println(DataToSend);
  Serial.println("And it is this many bytes: ");
  Serial.println(DataToSend.length());       
  Serial.println("\n");


  /* wait for Server Response */
  /*
    while (client.connected()) {
      String line = client.readStringUntil('\n');
      Serial.println(line);
      if (line == "\r") {
        Serial.println("headers received");
        break;
      }
    }
  */

  Serial.print("Waiting for response:\r\n");
  while (!client.available()){
      Serial.print("z");
      delay(50); //
  }  

  
  /* read and check Server Response */
  /*
    String line = client.readStringUntil('\n');
    if (line.startsWith("{\"state\":\"success\"")) {
      Serial.println("esp8266/Arduino CI successfull!");
    } else {
      Serial.println("esp8266/Arduino CI has failed");
    Serial.println(line);
    }
  */
  // if data is available then receive and print to Terminal
  while (client.available()) {
      char c = client.read();
      // Serial.print("+");
      Serial.write(c);
  }
}
