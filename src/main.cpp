#include <Arduino.h>
#include <EEPROM.h>
#include "params.h"
#include "DistBuffer.h"
#include "esp_system.h"
#include "SoftwareSerial.h"
#include "SD.h"

#define SCK   18
#define MISO  19
#define MOSI  23
#define CS    5

#define RX_BARCODE 22

#define EEPROM_SIZE 164

#define RX1 17
#define TX1 16
#define RX2 26
#define TX2 25

#define LED 33
#define LED_R 18
#define LED_G 4
#define LED_B 13

// pocet datovych bajtov zo senzora
#define NUM_BYTES 3

HardwareSerial hwSerial_1(1);
HardwareSerial hwSerial_2(2);
HardwareSerial hwSerial_3(3);

SoftwareSerial swSer1;

File dataFile;  // initialize sd file
int prev_file_indx = 0 ;  // used for file naming
String fileName = "000" ;
char file[10];

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

double serial_read[15];
double pamat_serial_read[15];
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
uint16_t opto_measure_count;
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
DistBuffer<double> memoryBuffer(MEMORY_BUFFER_SIZE);


// vzdialenost z optickeho snimaca
double optical_sensor;
// vzdialenost valca
int32_t dist_sensor;
// vzdialenost valca v impulzoch
int32_t dist_imp_sensor;
// spustenie merania
bool start_measure;
// koniec merania
bool end_measure;
// relativna vzdialenost od posledneho udaju
double relative_dist;
// aktualny namerany priemer valca
double priemer;
// posledna vzdialenost pri ktorej sa ratal priemer
double saved_dist;
// nulovacia vzdialenost v impulzoch
double zero_imp_distance;

// prebieha meranie
bool is_measuring;

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

String surfaceToStr(Surface surface)
{
  if (surface == isNothing) {
    return "nic";
  }
  else {
    return "valec";
  }
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
        float value = actVal.toFloat();

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
        }
        else if (actText == "vKalibruj") {
          KALIB_OPTO = optical_sensor;
          posliTEXT("nastav.t1.txt=", String(KALIB_OPTO, 4));
        }
        else if (actText == "vKalib") {
          KALIB_OPTO = value;
        }
        else if (actText == "vKrok") {
          MEASURE_STEP = value;
        }
        else if (actText == "vDlzka") {
          LENGTH_SCOPE = value;
        }
        else if (actText == "vservis1") {
          SAMPLES_PER_MM = value;
        }
        else if (actText == "vNuluj") {
          zero_imp_distance = dist_imp_sensor;
        }

        serial_read[0] = KALIB_PRIEMER;
        serial_read[1] = KALIB_OPTO;
        serial_read[2] = MEASURE_STEP;
        serial_read[3] = LENGTH_SCOPE;
        serial_read[4] = SAMPLES_PER_MM;

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

//---------------------------------------------------------------------------
void setup(void)
{
  pinMode(LED, OUTPUT); pinMode(LED_R, OUTPUT); pinMode(LED_G, OUTPUT); pinMode(LED_B, OUTPUT);
  digitalWrite(LED, HIGH); digitalWrite(LED_R, HIGH);
  delay(100);
  digitalWrite(LED, LOW); digitalWrite(LED_R, LOW);
  delay(100);
  // terminal
  hwSerial_2.begin (250000, SERIAL_8N1, RX2, TX2);
  // software serial - citacka kodov
  swSer1.begin(38400, SWSERIAL_8N1, RX_BARCODE, RX_BARCODE, false, 256);
  // high speed half duplex, turn off interrupts during tx
  swSer1.enableIntTx(false);

  delay(100);
  digitalWrite(LED, HIGH); digitalWrite(LED_G, HIGH);
  hwSerial_1.begin(921600, SERIAL_8N1, RX1, TX1); // RS422
  digitalWrite(LED, LOW); digitalWrite(LED_G, LOW);
  delay(100);

  digitalWrite(LED, HIGH);
  delay(100);
  // EEPROM
  if (!EEPROM.begin(EEPROM_SIZE)) {
    delay(50);
  }
  delay(100);
  // citanie z EEPROM
  for (int i = 0 ; i <= 12 ; i++) { // data v eeprom od serial_read[0]
    EEPROM.get(i * 8, serial_read[i]); // zlozenie bytov double
    pamat_serial_read[i] = serial_read[i];
  }

  delay(100);
  KALIB_PRIEMER = serial_read[0];
  KALIB_OPTO = serial_read[1];
  MEASURE_STEP = serial_read[2];
  LENGTH_SCOPE = serial_read[3];
  SAMPLES_PER_MM = serial_read[4];

  delay(1000);
  // senzor vzdialenosti
  Serial.begin(115200, SERIAL_8N1); // RS422
  delay(100);
  

  delay(500); // oneskorenie aby presiel zapis na terminal
  // zapis hodnot na terminal
  posliTEXT("nastav.t0.txt=", String(KALIB_PRIEMER, 2));
  posliTEXT("nastav.t1.txt=", String(KALIB_OPTO, 4));
  posliTEXT("nastav.t2.txt=", String(MEASURE_STEP, 2));
  posliTEXT("nastav.t3.txt=", String(LENGTH_SCOPE, 2));
  posliTEXT("servis.t1.txt=", String(SAMPLES_PER_MM, 0));
}


//---------------------------------------------------------------------------
void loop(void)
{
  // posielanie udajov
  if (cas_print > 35 ) {
    posliTEXT("home.t40.txt=", String(optical_sensor, 4));   //vzdialenost
    posliTEXT("home.t13.txt=", ciarovy_kod);
    posliTEXT("home.t1.txt=", String(priemer, 4));
    posliTEXT("home.t2.txt=", String(dist_sensor));
    posliTEXT("kontrola1.t1.txt=", String(opto_count));
    posliTEXT("kontrola1.t2.txt=", String(dist_count));
    cas_print = 0;
  }

  if (cas_print2 > 50 ) {
    posliTEXT("home.t25.txt=", String(den)); // posli den
    posliTEXT("home.t26.txt=", String(mes)); // posli mesiac
    posliTEXT("home.t27.txt=", String(hod)); // posli hodiny
    posliTEXT("home.t28.txt=", String(minuty)); // posli minuty
    posliTEXT("home.t29.txt=", String(sec)); // posli sekundy
    cas_print2 = 0;
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


  if (test_timer >= 1) {
    test_timer = 0;
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
    for (int i = 0 ; i <= 12 ; i++) {
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
      posliTEXT("home.t5.txt=", "Inicializacia karty OK");
      String date = "";
      date += den; // den
      date += "_";
      date += mes; // mesiac
      date += "-";
      date += hod; // hodiny
      date += "_";
      date += minuty; // minuty
      date += "_";
      date += sec; // sekundy
      dataFile = SD.open("/" + date + ".csv" , FILE_WRITE);
      if (dataFile) {
        posliTEXT("home.t5.txt=", "Prebieha zapis na kartu");
        dataFile.println("ciarovy_kod,vzdialenost,priemer");
        String data_column;
        data_column = ciarovy_kod;
        for (int i = 0; i < optoBuffer.numElems; ++i) {
          data_column += ",";
          data_column += String(distBuffer.values[i], 2);
          data_column += ",";
          data_column += String(optoBuffer.values[i], 4);
          dataFile.println(data_column);
          data_column = "";
        }
        dataFile.close();
        posliTEXT("home.t5.txt=", "Zapis na kartu OK");
        posliTEXT("servis.t1.txt=", String(optoBuffer.numElems));
        optoBuffer.clear();
        distBuffer.clear();
      }
      else {
        posliTEXT("home.t5.txt=", "Zapis na kartu ERROR");
      }
      SD.end();
    }
  }


  //---------------------------------------------------------------------------
  // vycitanie ciarovych kodov
  if (swSer1.available()) {
    digitalWrite(LED, true);
    String val = swSer1.readStringUntil('\n');
    ciarovy_kod = val.substring(0, val.length()-1);
    digitalWrite(LED, false);
    posliTEXT("home.t5.txt=", "Ciarovy kod precitany");
  }


  //---------------------------------------------------------------------------
  // senzor vzdialenosti
  if (Serial.available()) {
    char s = ' ';
    String val = "";
    delay(3);
    while (Serial.available() > 0) {
      s = Serial.read();
      val += char(s);
    }
    dist_imp_sensor = val.toInt();
    posliTEXT("servis.t3.txt=", String(dist_imp_sensor));
    if (SAMPLES_PER_MM > 0) {
      dist_sensor = (dist_imp_sensor - zero_imp_distance) / SAMPLES_PER_MM;
    }
    dist_count++;
  }

  // byte_count = Serial.available();
  // if (byte_count == 4) {
  //   byte buff[4];
  //   int i = 0;
  //   for (int i = 0; i < 4; ++i) {
  //     buff[i++] = Serial.read();
  //   }
  //   dist_sensor = (uint32_t)(buff[0]<<24) + (uint32_t)(buff[1]<<16) + (uint32_t)(buff[2]<<8) + (uint32_t)buff[3];
  // }
  // else if (byte_count > 4) {
  //   for (int i = 0; i < byte_count; ++i) {
  //     Serial.read();
  //   }
  // }

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
    optical_sensor = ((data1R - 98232) / 65536) * 10; // snimac 10 mm
    opto_count++;

    // prepocitany priemer valca
    priemer = KALIB_PRIEMER + 2*(KALIB_OPTO - optical_sensor);

    // zaciatok merania
    /*************************************************************************/
    // zaciname stlacenim tlacitka
    if (start_measure) {
      posliTEXT("home.t5.txt=", "Prebieha meranie");
      // pri spusteni merania
      if (is_measuring == false) {
        saved_dist = dist_sensor;
        is_measuring = true;
        opto_count = 0;
        dist_count = 0;
        memoryBuffer.clear();
      }
      // relativna vzdialenost od posledne nameranej hodnoty
      relative_dist = dist_sensor - saved_dist;
      // // prah vzdialenosti pre detekciu
      // if (optical_sensor <= DIST_TRESHOLD) {
      //   if (relative_dist >= (MEASURE_STEP - (LENGTH_SCOPE / 2))) {
      //     memoryBuffer.addValue(priemer);
      //   }
      //   if (relative_dist >= (MEASURE_STEP + (LENGTH_SCOPE / 2))) {
      //     double avg = memoryBuffer.getAverage();
      //     memoryBuffer.clear();
      //     optoBuffer.addValue(avg);
      //     distBuffer.addValue(saved_dist + MEASURE_STEP);
      //     saved_dist += MEASURE_STEP;
      //   }
      // }
      if (optical_sensor <= DIST_TRESHOLD) {
        // pridaj odmerany priemer
        if (relative_dist >= MEASURE_STEP) {
          memoryBuffer.addValue(priemer);
          opto_measure_count++;
        }
        // vyhodnot buffre
        if (opto_measure_count >= LENGTH_SCOPE
            || relative_dist >= MEASURE_STEP*2) {
          opto_measure_count = 0;
          double avg = memoryBuffer.getAverage();
          measures_per_sample = memoryBuffer.numElems;
          memoryBuffer.clear();
          optoBuffer.addValue(avg);
          distBuffer.addValue(saved_dist + MEASURE_STEP);
          saved_dist += MEASURE_STEP;
          relative_dist = dist_sensor - saved_dist;
        }
      }
    }
    // koncime stlacenim tlacitka
    if (end_measure) {
      end_measure = false;
      start_measure = false;
      is_measuring = false;
      zapisSD = true;
      posliTEXT("home.t5.txt=", "Meranie ukoncene");
    }
  }
}
