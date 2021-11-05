/**
 * Meranie priemeru valca
 * 
 * pocet impulzov v Z osi: 213.236
 */

#include <Arduino.h>
#include <EEPROM.h>
#include <EthernetENC.h>
#include <esp_task_wdt.h>
#include "params.h"
#include "DistBuffer.h"
#include "esp_system.h"
#include "SoftwareSerial.h"
#include "SD.h"
#include "RunningMedian.h"

#define SCK   18
#define MISO  19
#define MOSI  23
#define CS    5

#define RX_BARCODE 32
#define TX_BARCODE 27

#define ETH_CS 2

#define EEPROM_SIZE 164

#define RX1 17
#define TX1 16
#define RX2 26
#define TX2 25

#define LED 33

// 21, 22 - IIC

#define START_BUTTON 35
#define STOP_BUTTON 34

#define LED_START 15
#define LED_STOP 4

// pocet datovych bajtov zo senzora
#define NUM_BYTES 3

// watchdog timeout na restart
#define WDT_TIMEOUT 5

HardwareSerial hwSerial_1(1);
HardwareSerial hwSerial_2(2);
HardwareSerial hwSerial_3(3);

SoftwareSerial swSer1;

File dataFile;  // initialize sd file
int prev_file_indx = 0 ;  // used for file naming
String fileName = "000" ;
char file[10];

/* Ethernet */
// Enter a MAC address for your controller below.
// Newer Ethernet shields have a MAC address printed on a sticker on the shield
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };

// if you don't want to use DNS (and reduce your sketch size)
// use the numeric IP instead of the name for the server:
//IPAddress server(74,125,232,128);  // numeric IP for Google (no DNS)
String server;    // name address for Google (using DNS)
int server_ip[4];
int klient_ip[4];
int port = 8080;

// Initialize the Ethernet client library
// with the IP address and port of the server
// that you want to connect to (port 80 is default for HTTP):
EthernetClient client;

// Variables to measure the speed
unsigned long beginMicros, endMicros;
unsigned long byteCount = 0;
bool printWebData = true;  // set to false for better speed measurement

const int data_size = 1000;
String string_data = "";
double number;
double numbers[data_size];
bool connected;
bool eth_connected;
bool zapisEthernet;

uint16_t citac;
uint32_t timerDsec;
uint16_t cas_led;
uint32_t timer1;
uint32_t test_timer;

uint16_t cas_print;
uint16_t cas_print2;

uint8_t  L_Byte1;
uint8_t  M_Byte1;
uint8_t  H_Byte1;
uint32_t hodnota1;
uint16_t cislo_word1;
uint8_t dataRS422_1;
uint16_t pocet_sprav1;
double data1R;

const int serial_size = 15;
double serial_read[serial_size];
double pamat_serial_read[serial_size];
uint16_t data1;
uint16_t data2;
uint16_t data3;
uint16_t data4;
uint16_t pocet_spravHMI;
uint16_t data_HMI[20];
uint16_t farba;
uint16_t pamatfarba;
uint16_t cas_alarm;
uint16_t alarm_sig;

uint16_t opto_count;
uint16_t dist_count;
uint16_t measures_per_sample;

// posielane z terminalu
uint8_t mes;
uint8_t den;
uint8_t hod;
uint8_t minuty;
uint8_t sec;

// parsing textu z terminalu
enum State { isStart, isEscape, isText, isValue };
State actState = isStart;
String actText;
String actVal;

String ciarovy_kod;

char incomingByte;
bool valuesReady;
bool zapisEEPROM;

bool zapisSD;
bool initSD;

// buffre
DistBuffer<double> optoBuffer(ROLLER_BUFFER_SIZE);
DistBuffer<double> distBuffer(ROLLER_BUFFER_SIZE);
// DistBuffer<double> memoryBuffer(MEMORY_BUFFER_SIZE);
RunningMedian memoryBuffer = RunningMedian(MEMORY_BUFFER_SIZE);


// pocet hodnot zobrazenych na displeji
const int showCount = 10;

// vzdialenost z optickeho snimaca
double optical_sensor;
// vzdialenost valca
double dist_sensor;
// vzdialenost valca v impulzoch
int32_t dist_imp_sensor;
// spustenie merania
bool start_measure;
// koniec merania
bool end_measure;
// aktualny namerany priemer valca
double priemer;
// namerana dlzka valca
double dlzka;
// dlzka pri krokoch
double next_step;
// vzdialenost pri zaciatku valca
double start_dist;
// nulovacia vzdialenost v impulzoch
double zero_imp_distance;

// prebieha meranie
bool is_measuring;
// prebieha kalibracia
bool is_calibrating;

int byte_count;


//---------------------------------------------------------------------------
// posli text do terminalu
void posliTEXT(String text1, String text2) {
  hwSerial_2.print(text1);
  hwSerial_2.write(0x22);
  hwSerial_2.print(text2);
  hwSerial_2.write(0x22);
  hwSerial_2.write(0xff);
  hwSerial_2.write(0xff);
  hwSerial_2.write(0xff);
}

// posli data do terminalu
void posliDATA(String text1, uint16_t data) {
  hwSerial_2.print(text1);
  hwSerial_2.print(data);
  hwSerial_2.write(0xff);
  hwSerial_2.write(0xff);
  hwSerial_2.write(0xff);
}

String getDateStr() {
  String date = "";
  date += den; // den
  date += "-";
  date += mes; // mesiac
  date += "_";
  date += hod; // hodiny
  date += "-";
  date += minuty; // minuty
  date += "-";
  date += sec; // sekundy
  return date;
}


// priradenie hodnoty k prijatemu textu z terminalu
void parseValues(char val) {
  switch (actState) {
    case isStart:  if (val == 'v') {
        actText += val;
        actState = isText;
      }
      else if (val == '\xFF') {
        actState = isEscape;
      }
      break;

    case isEscape: if (val != '\xFF') {
        actText += val;
        actState = isText;
      }
      break;

    case isText:   if (val != '\x22') {
        actText += val;
      }
      else {
        actState = isValue;
      }
      break;

    case isValue:  if (val != '\x22') {
        actVal += val;
      }
      else {
        double value = actVal.toDouble();
        bool change_ip = false;

        if (actText == "vM") {
          mes = value;
        }
        else if (actText == "vD") {
          den = value;
        }
        else if (actText == "vH") {
          hod = value;
        }
        else if (actText == "vm") {
          minuty = value;
        }
        else if (actText == "vs") {
          sec = value;
        }
        else if (actText == "vStart") {
          start_measure = true;
        }
        else if (actText == "vStop") {
          if (start_measure) {
            end_measure = true;
          }
        }
        else if (actText == "vPriemer") {
          KALIB_PRIEMER = value;
          serial_read[14] = KALIB_PRIEMER;
        }
        else if (actText == "vKalibruj") {
          KALIB_OPTO = optical_sensor;
          serial_read[1] = KALIB_OPTO;
          posliTEXT("nastav.t1.txt=", String(KALIB_OPTO, 4));
        }
        else if (actText == "vKalib") {
          KALIB_OPTO = value;
        }
        else if (actText == "vKrok") {
          MEASURE_STEP = value;
          serial_read[2] = MEASURE_STEP;
        }
        else if (actText == "vDlzka") {
          LENGTH_SCOPE = value;
          serial_read[3] = LENGTH_SCOPE;
        }
        else if (actText == "vservis1") {
          SAMPLES_PER_MM = value;
          serial_read[4] = SAMPLES_PER_MM;
        }
        else if (actText == "vNuluj") {
          zero_imp_distance = dist_imp_sensor;
        }
        else if (actText == "vethernet1") {
          server_ip[0] = value;
          serial_read[5] = server_ip[0];
          change_ip = true;
        }
        else if (actText == "vethernet2") {
          server_ip[1] = value;
          serial_read[6] = server_ip[1];
          change_ip = true;
        }
        else if (actText == "vethernet3") {
          server_ip[2] = value;
          serial_read[7] = server_ip[2];
          change_ip = true;
        }
        else if (actText == "vethernet4") {
          server_ip[3] = value;
          serial_read[8] = server_ip[3];
          change_ip = true;
        }
        else if (actText == "vethernet5") {
          port = value;
          serial_read[9] = port;
        }
        else if (actText == "vethernet6") {
          klient_ip[0] = value;
          serial_read[10] = klient_ip[0];
        }
        else if (actText == "vethernet7") {
          klient_ip[1] = value;
          serial_read[11] = klient_ip[1];
        }
        else if (actText == "vethernet8") {
          klient_ip[2] = value;
          serial_read[12] = klient_ip[2];
        }
        else if (actText == "vethernet9") {
          klient_ip[3] = value;
          serial_read[13] = klient_ip[3];
        }

        if (change_ip) {
          server = String(server_ip[0]) + "." + String(server_ip[1])
            + "." + String(server_ip[2]) + "." + String(server_ip[3]);
        }

        zapisEEPROM = true;
        actText = "";
        actVal = "";
        actState = isStart;
      }
      break;

    default:
      break;
  }
}

int connectServer() {
  if (client.connect(server.c_str(), port)) {
    posliTEXT("home.t5.txt=", "Connected to server");
    delay(1000);
    return 1;
  } else {
    // if you didn't get a connection to the server:
    posliTEXT("home.t5.txt=", "Connection failed" );
    // reconnect after Ethernet unplugged
    if (Ethernet.linkStatus() == LinkOFF) {
      posliTEXT("home.t5.txt=", "Ethernet nepripojeny");
    }
  }
  return 0;
}

double evaluateMemoryBuffer() {
  int bufferCount = memoryBuffer.getCount();
  double avg;
  if (NUM_MEDIANS_RATE > 0) {
    // pocet medianov, z ktorych sa zrobi priemer
    int avgCount = (double)bufferCount / NUM_MEDIANS_RATE;
    if (avgCount > 0) {
      avg = memoryBuffer.getAverage(avgCount);
    }
    else {
      avg = memoryBuffer.getAverage();
    }
  }
  else {
    avg = memoryBuffer.getAverage();
  }
  return avg;
}

//---------------------------------------------------------------------------
void setup(void)
{
  pinMode(START_BUTTON, INPUT);
  pinMode(STOP_BUTTON, INPUT);
  pinMode(LED_START, OUTPUT);
  pinMode(LED_STOP, OUTPUT);
  pinMode(LED, OUTPUT);
  digitalWrite(LED, HIGH);
  delay(100);
  digitalWrite(LED, LOW);
  delay(100);
  // terminal
  hwSerial_2.begin (250000, SERIAL_8N1, RX2, TX2);
  // software serial - citacka kodov
  swSer1.begin(38400, SWSERIAL_8N1, RX_BARCODE, TX_BARCODE, false, 256);
  // high speed half duplex, turn off interrupts during tx
  swSer1.enableIntTx(false);
  digitalWrite(LED, HIGH);
  delay(100);
  // opticky snimac
  hwSerial_1.begin(921600, SERIAL_8N1, RX1, TX1); // RS422
  digitalWrite(LED, LOW);
  digitalWrite(LED, HIGH);
  delay(100);
  // EEPROM
  if (!EEPROM.begin(EEPROM_SIZE)) {
    delay(50);
  }
  digitalWrite(LED, LOW);
  delay(100);
  // citanie z EEPROM
  for (int i = 0 ; i < serial_size ; i++) { // data v eeprom od serial_read[0]
    EEPROM.get(i * 8, serial_read[i]); // zlozenie bytov double
    pamat_serial_read[i] = serial_read[i];
  }
  digitalWrite(LED, HIGH);

  delay(100);
  KALIB_PRIEMER = serial_read[14];
  KALIB_OPTO = serial_read[1];
  MEASURE_STEP = serial_read[2];
  LENGTH_SCOPE = serial_read[3];
  SAMPLES_PER_MM = serial_read[4];
  server_ip[0] = serial_read[5];
  server_ip[1] = serial_read[6];
  server_ip[2] = serial_read[7];
  server_ip[3] = serial_read[8];
  server = String(server_ip[0]) + "." + String(server_ip[1])
    + "." + String(server_ip[2]) + "." + String(server_ip[3]);
  port = serial_read[9];
  klient_ip[0] = serial_read[10];
  klient_ip[1] = serial_read[11];
  klient_ip[2] = serial_read[12];
  klient_ip[3] = serial_read[13];

  delay(100);
  Ethernet.init(ETH_CS);
  // Set the static IP address
  IPAddress ip(klient_ip[0], klient_ip[1], klient_ip[2], klient_ip[3]);
  Ethernet.begin(mac, ip);
  delay(1000);
  // senzor vzdialenosti
  Serial.begin(115200, SERIAL_8N1); // RS422
  digitalWrite(LED, LOW);
  delay(100);
  digitalWrite(LED, HIGH);

  delay(500); // oneskorenie aby presiel zapis na terminal
  // zapis hodnot na terminal
  posliTEXT("nastav.t0.txt=", String(KALIB_PRIEMER, 2));
  posliTEXT("nastav.t1.txt=", String(KALIB_OPTO, 4));
  posliTEXT("nastav.t2.txt=", String(MEASURE_STEP, 2));
  posliTEXT("nastav.t3.txt=", String(LENGTH_SCOPE, 2));
  posliTEXT("servis.t0.txt=", String(SAMPLES_PER_MM, 4));
  posliTEXT("ethernet.t1.txt=", String(server_ip[0]));
  posliTEXT("ethernet.t2.txt=", String(server_ip[1]));
  posliTEXT("ethernet.t3.txt=", String(server_ip[2]));
  posliTEXT("ethernet.t4.txt=", String(server_ip[3]));
  posliTEXT("ethernet.t5.txt=", String(port));
  posliTEXT("ethernet.t6.txt=", String(klient_ip[0]));
  posliTEXT("ethernet.t7.txt=", String(klient_ip[1]));
  posliTEXT("ethernet.t8.txt=", String(klient_ip[2]));
  posliTEXT("ethernet.t9.txt=", String(klient_ip[3]));

  //----------------   watch dog -------------------------------------
  // esp_task_wdt_init(WDT_TIMEOUT, true); //enable panic so ESP32 restarts
  // esp_task_wdt_add(NULL); //add current thread to WDT watch
}


//---------------------------------------------------------------------------
void loop(void)
{
  // Ethernet.maintain(); // pri DHCP
  // esp_task_wdt_reset();

  // posielanie udajov
  if (cas_print > 10) {
    cas_print = 0;
    posliTEXT("home.t40.txt=", String(optical_sensor, 3));   //vzdialenost
    posliTEXT("home.t13.txt=", ciarovy_kod);
    posliTEXT("home.t1.txt=", String(priemer, 3));
    posliTEXT("home.t2.txt=", String(dist_sensor, 2));
    posliTEXT("kontrola1.t1.txt=", String(opto_count));
    posliTEXT("kontrola1.t2.txt=", String(dist_count));
  }

  /* casovace */
  if (abs(millis() - timer1) >= 10) {
    timer1 = millis();
    citac++;
    cas_print++;
    cas_print2++;
    cas_led++;
    cas_alarm++;
  }

  //---------------------------------------------------------------------------
  // pocitanie realneho casu
  if (abs(millis() - timerDsec) >= 1000) {
    test_timer++;
    sec++;
    timerDsec = millis();
  }
  if (sec > 59) {
    minuty++;
    sec = 0;
  }
  if (minuty > 59) {
    hod++;
    minuty = 0;
  }


  //---------------------------------------------------------------------------
  // seriovy UART prenos, citanie z terminalu
  while (hwSerial_2.available() > 0) {
    char ch = ' ';
    ch = hwSerial_2.read();
    parseValues(ch);
  }

  //---------------------------------------------------------------------------
  // zapis do EEPROM ak pride udaj z terminalu
  if (zapisEEPROM == true) {
    for (int i = 0 ; i < serial_size ; i++) {
      if (pamat_serial_read[i] != serial_read[i]) {
        EEPROM.put(i * 8, serial_read[i]);
        EEPROM.commit();
        pamat_serial_read[i] = serial_read[i];
      }
    }
    zapisEEPROM = false;
  }

  //---------------------------------------------------------------------------
  // zapis na SD kartu
  if (zapisSD) {
    zapisSD = false;
    // spi.begin(SCK, MISO, MOSI, CS);
    // if (!SD.begin(CS,spi,80000000)) {
    if (!SD.begin(CS)) {
      posliTEXT("home.t5.txt=", "Karta nie je vlozena");
    }
    else {
      String date = getDateStr();
      posliTEXT("home.t5.txt=", "Inicializacia karty OK");
      dataFile = SD.open("/" + date + ".csv" , FILE_WRITE);
      if (dataFile) {
        posliTEXT("home.t5.txt=", "Prebieha zapis na kartu");
        dataFile.println("ciarovy_kod,vzdialenost,priemer");
        String data_row;
        data_row = ciarovy_kod;
        for (int i = 0; i < optoBuffer.numElems; ++i) {
          data_row += ",";
          data_row += String(distBuffer.values[i], 2);
          data_row += ",";
          data_row += String(optoBuffer.values[i], 4);
          dataFile.println(data_row);
          data_row = "";
        }
        if (!ciarovy_kod.isEmpty() && optoBuffer.numElems == 0) {
          dataFile.println(data_row);
        }
        dataFile.close();
        posliTEXT("home.t5.txt=", "Zapis na kartu OK");
      }
      else {
        posliTEXT("home.t5.txt=", "Zapis na kartu ERROR");
      }
      SD.end();
    }
  }


  //---------------------------------------------------------------------------
  // Ethernet
  if (zapisEthernet) {
    test_timer = 0;
    zapisEthernet = false;
    connected = connectServer();

    if (client.connected()) {
      // Make a HTTP request:
      client.println("POST /data HTTP/1.1");
      client.println("Host: " + server + ":" + String(port));
      client.println("User-Agent: Arduino");
      client.println("Content-Type: text/plain");
      client.println("Transfer-Encoding: chunked");
      // client.print("Content-Length: ");
      // client.println(data_row.length());
      client.println("Connection: close");
      client.println();

      // send chunked data
      String data_row = "ciarovy_kod,vzdialenost,priemer\n";
      data_row += ciarovy_kod;
      for (int i = 0; i < optoBuffer.numElems; ++i) {
        data_row += ",";
        data_row += String(distBuffer.values[i], 2);
        data_row += ",";
        data_row += String(optoBuffer.values[i], 4);
        data_row += '\n';
        // send one row
        client.println(String(data_row.length(), HEX));
        client.println(data_row);
        data_row = "";
      }
      if (!ciarovy_kod.isEmpty() && optoBuffer.numElems == 0) {
        data_row += '\n';
      }
      data_row += getDateStr();
      // send last data row
      client.println(String(data_row.length(), HEX));
      client.println(data_row);
      // send ending row
      client.println(String(0, HEX));
      client.println();

      // disconnect, flush the client
      while (client.connected()) {
        client.flush();
        client.stop();
        delay(1000);
      }
    }
    digitalWrite(LED_STOP, LOW);
  }

  // check if ethernet is inplugged
  if (Ethernet.linkStatus() == LinkOFF) {
    eth_connected = false;
  }

  // if there are incoming bytes available
  // from the server, read them and print them:
  int len = client.available();
  if (len > 0) {
    byte buffer[80];
    if (len > 80) len = 80;
    client.read(buffer, len);
  }

  //---------------------------------------------------------------------------
  // vycitanie ciarovych kodov
  if (swSer1.available()) {
    digitalWrite(LED, LOW);
    String val = swSer1.readStringUntil('\n');
    ciarovy_kod = val.substring(0, val.length()-1);
    digitalWrite(LED, HIGH);
    posliTEXT("home.t5.txt=", "Ciarovy kod precitany");
  }

  //---------------------------------------------------------------------------
  // senzor vzdialenosti
  if (Serial.available()) {
    String val = Serial.readStringUntil('\n');
    dist_imp_sensor = val.toInt();
    posliTEXT("servis.t3.txt=", String(dist_imp_sensor));
    if (SAMPLES_PER_MM > 0) {
      dist_sensor = (dist_imp_sensor - zero_imp_distance) / SAMPLES_PER_MM;
    }
    dist_count++;
  }

  //---------------------------------------------------------------------------
  // tlacitka a ledky
  if (digitalRead(START_BUTTON) == LOW) {
    start_measure = true;
    digitalWrite(LED_STOP, LOW);
    digitalWrite(LED_START, HIGH);
  }
  if (start_measure && digitalRead(STOP_BUTTON) == LOW) {
    end_measure = true;
    digitalWrite(LED_START, LOW);
    digitalWrite(LED_STOP, HIGH);
  }


  /*************************************************************************/
  // opticky senzor vysky
  if (hwSerial_1.available() >= NUM_BYTES) {
    pocet_sprav1 = hwSerial_1.available();

    for (int i = 0 ; i < pocet_sprav1 ; i++) {
      dataRS422_1 = hwSerial_1.read();
      bitWrite(cislo_word1, 0, bitRead(dataRS422_1, 6));
      bitWrite(cislo_word1, 1, bitRead(dataRS422_1, 7));
      if (cislo_word1 == 0) {
        L_Byte1 = dataRS422_1;
      }
      if (cislo_word1 == 1) {
        M_Byte1 = dataRS422_1;
      }
      if (cislo_word1 > 1) {
        H_Byte1 = dataRS422_1;
      }
    }

    // skladanie bajtov
    bitWrite(hodnota1, 0,  bitRead(L_Byte1, 0));
    bitWrite(hodnota1, 1,  bitRead(L_Byte1, 1));
    bitWrite(hodnota1, 2,  bitRead(L_Byte1, 2));
    bitWrite(hodnota1, 3,  bitRead(L_Byte1, 3));
    bitWrite(hodnota1, 4,  bitRead(L_Byte1, 4));
    bitWrite(hodnota1, 5,  bitRead(L_Byte1, 5));
    bitWrite(hodnota1, 6,  bitRead(M_Byte1, 0));
    bitWrite(hodnota1, 7,  bitRead(M_Byte1, 1));
    bitWrite(hodnota1, 8,  bitRead(M_Byte1, 2));
    bitWrite(hodnota1, 9,  bitRead(M_Byte1, 3));
    bitWrite(hodnota1, 10, bitRead(M_Byte1, 4));
    bitWrite(hodnota1, 11, bitRead(M_Byte1, 5));
    bitWrite(hodnota1, 12, bitRead(H_Byte1, 0));
    bitWrite(hodnota1, 13, bitRead(H_Byte1, 1));
    bitWrite(hodnota1, 14, bitRead(H_Byte1, 2));
    bitWrite(hodnota1, 15, bitRead(H_Byte1, 3));
    bitWrite(hodnota1, 16, bitRead(H_Byte1, 4));
    bitWrite(hodnota1, 17, bitRead(H_Byte1, 5));

    data1R = hodnota1;
    optical_sensor = ((data1R - 98232) / 65536) * 25; // snimac 25 mm
    opto_count++;

    // prepocitany priemer valca
    priemer = KALIB_PRIEMER + 2*(KALIB_OPTO - optical_sensor);
  }

  // zaciatok merania
  /*************************************************************************/
  // zaciname stlacenim tlacitka
  if (start_measure) {
    posliTEXT("home.t5.txt=", "Prebieha meranie");

    // prah vzdialenosti pre detekciu
    if (optical_sensor <= DIST_TRESHOLD) {
      // pri spusteni merania
      if (is_measuring == false) {
        is_measuring = true;
        if (KALIB_DLZKA > 0) {
          is_calibrating = true;
        }
        start_dist = dist_sensor;
        next_step = MEASURE_STEP;
        opto_count = 0;
        dist_count = 0;
        optoBuffer.clear();
        distBuffer.clear();
        memoryBuffer.clear();
      }
      // aktualna dlzka valca
      dlzka = dist_sensor - start_dist;
      // pridaj odmerany priemer valca
      memoryBuffer.add(priemer);
      measures_per_sample++;

      // vyhodnot buffer a uloz si spriemerovanu hodnotu medzi zaznamy
      if (dlzka >= next_step) {
        double avg = evaluateMemoryBuffer();
        next_step += MEASURE_STEP;
        measures_per_sample = 0;
        optoBuffer.add(avg);
        distBuffer.add(next_step);
      }
      // vyhodnot buffer ked kalibrujeme
      if (is_calibrating && dlzka >= KALIB_DLZKA) {
        double avg = evaluateMemoryBuffer();
        KALIB_OPTO = avg;
        is_calibrating = false;
        // uprav vsetky predosle hodnoty
        if (optoBuffer.numElems > 0) {
          for (int i = 0; i < optoBuffer.numElems; ++i) {
            optoBuffer.values[i] += KALIB_OPTO * 2;
          }
        }
      }
      if (is_calibrating == false) {
          memoryBuffer.clear();
      }
    }
  }

  // koncime stlacenim tlacitka
  if (end_measure) {
    /* len na testovanie */
    // double dist = 0, opto = 0;
    // for (int i = 0; i < ROLLER_BUFFER_SIZE; ++i) {
    //   optoBuffer.add(opto);
    //   distBuffer.add(dist);
    //   opto += 0.01;
    //   dist += 0.1;
    // }

    end_measure = false;
    start_measure = false;
    is_measuring = false;
    zapisSD = true;
    zapisEthernet = true;

    int showStep = optoBuffer.numElems / showCount;
    int actStep = 0, distTerminalId = 50, optoTerminalId = 60;
    for (int i = 0; i < showCount; ++i) {
      posliTEXT("home.t"+String(distTerminalId)+".txt=", String(distBuffer.values[actStep], 0));
      posliTEXT("home.t"+String(optoTerminalId)+".txt=", String(optoBuffer.values[actStep], 4));
      distTerminalId++; optoTerminalId++;
      actStep += showStep;
    }
    posliTEXT("home.t70.txt=", String(dlzka, 2));
    posliTEXT("home.t5.txt=", "Meranie ukoncene");
  }
}
