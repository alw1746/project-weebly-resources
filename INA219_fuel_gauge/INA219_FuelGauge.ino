/*
Single cell lithium battery fuel gauge based on coulomb counting(1mAh = 3.6 coulombs) with
drift compensated by voltage measurement. Voltage and current readings are provided
by INA219 sensor. If capacity reaches 0, the load is disconnected by P-chan MOSFET
to prevent damage. The mahConsumed value is saved to flash on every 5% change and restored on power up.
Voltage and mahConsumed lookup tables are derived from profiling the battery through INA219_profiler
sketch. The CSV output is processed by generate_lut.py which generates the C++ array initialisation
code for use by the fuel gauge.
*/
#include <Wire.h>
#include <Preferences.h>
#include <INA219_WE.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#define CUTOFF_PIN 12
#define CUTOFF_VOLTAGE 3.5f
#define MAX_VC_DEVIATION 5
#define CHANGE_THRESHOLD 10
#define BATTERY_IR 0.4f      //IR = (Voc - Vload)/load_current_A

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
//NB: this is not strictly true, 0x3C is used for 128x64 displays too. Test OLED to verify.
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

INA219_WE ina219 = INA219_WE();
Preferences preferences;

unsigned long timer2=0;           //event timer
unsigned long period2 = 1000;     //elapsed time(ms)
bool timer2Enabled=false;         //event switch

int cutoffCounter=0,deviationCounter=0;
int lastSavePercent=0;
float totalConsumedMAH = -1.0;       //uninitialised state
float prevConsumedMAH  = -1.0;       //uninitialised state

float vt[101];
float ct[101];

void initVoltageTable() {
    vt[  0] = 4.15;
    vt[  1] = 4.048;
    vt[  2] = 4.034;
    vt[  3] = 4.022;
    vt[  4] = 4.011;
    vt[  5] = 4.001;
    vt[  6] = 3.992;
    vt[  7] = 3.983;
    vt[  8] = 3.976;
    vt[  9] = 3.970;
    vt[ 10] = 3.965;
    vt[ 11] = 3.960;
    vt[ 12] = 3.956;
    vt[ 13] = 3.953;
    vt[ 14] = 3.950;
    vt[ 15] = 3.946;
    vt[ 16] = 3.942;
    vt[ 17] = 3.938;
    vt[ 18] = 3.932;
    vt[ 19] = 3.927;
    vt[ 20] = 3.922;
    vt[ 21] = 3.917;
    vt[ 22] = 3.914;
    vt[ 23] = 3.911;
    vt[ 24] = 3.906;
    vt[ 25] = 3.897;
    vt[ 26] = 3.892;
    vt[ 27] = 3.887;
    vt[ 28] = 3.876;
    vt[ 29] = 3.868;
    vt[ 30] = 3.860;
    vt[ 31] = 3.854;
    vt[ 32] = 3.847;
    vt[ 33] = 3.841;
    vt[ 34] = 3.835;
    vt[ 35] = 3.830;
    vt[ 36] = 3.825;
    vt[ 37] = 3.811;
    vt[ 38] = 3.799;
    vt[ 39] = 3.794;
    vt[ 40] = 3.788;
    vt[ 41] = 3.781;
    vt[ 42] = 3.774;
    vt[ 43] = 3.766;
    vt[ 44] = 3.760;
    vt[ 45] = 3.753;
    vt[ 46] = 3.747;
    vt[ 47] = 3.742;
    vt[ 48] = 3.738;
    vt[ 49] = 3.733;
    vt[ 50] = 3.729;
    vt[ 51] = 3.725;
    vt[ 52] = 3.721;
    vt[ 53] = 3.717;
    vt[ 54] = 3.713;
    vt[ 55] = 3.710;
    vt[ 56] = 3.705;
    vt[ 57] = 3.702;
    vt[ 58] = 3.698;
    vt[ 59] = 3.694;
    vt[ 60] = 3.689;
    vt[ 61] = 3.686;
    vt[ 62] = 3.682;
    vt[ 63] = 3.678;
    vt[ 64] = 3.674;
    vt[ 65] = 3.670;
    vt[ 66] = 3.667;
    vt[ 67] = 3.663;
    vt[ 68] = 3.659;
    vt[ 69] = 3.656;
    vt[ 70] = 3.653;
    vt[ 71] = 3.649;
    vt[ 72] = 3.645;
    vt[ 73] = 3.643;
    vt[ 74] = 3.639;
    vt[ 75] = 3.637;
    vt[ 76] = 3.635;
    vt[ 77] = 3.633;
    vt[ 78] = 3.632;
    vt[ 79] = 3.631;
    vt[ 80] = 3.629;
    vt[ 81] = 3.628;
    vt[ 82] = 3.626;
    vt[ 83] = 3.624;
    vt[ 84] = 3.621;
    vt[ 85] = 3.619;
    vt[ 86] = 3.616;
    vt[ 87] = 3.613;
    vt[ 88] = 3.609;
    vt[ 89] = 3.606;
    vt[ 90] = 3.602;
    vt[ 91] = 3.597;
    vt[ 92] = 3.592;
    vt[ 93] = 3.585;
    vt[ 94] = 3.577;
    vt[ 95] = 3.567;
    vt[ 96] = 3.557;
    vt[ 97] = 3.545;
    vt[ 98] = 3.532;
    vt[ 99] = 3.519;
    vt[100] = 3.500;
}

void initCurrentTable() {
    ct[  0] = 0.0;
    ct[  1] = 12.4;
    ct[  2] = 24.9;
    ct[  3] = 37.3;
    ct[  4] = 49.8;
    ct[  5] = 62.2;
    ct[  6] = 74.6;
    ct[  7] = 87.1;
    ct[  8] = 99.5;
    ct[  9] = 112.0;
    ct[ 10] = 124.4;
    ct[ 11] = 136.8;
    ct[ 12] = 149.3;
    ct[ 13] = 161.7;
    ct[ 14] = 174.2;
    ct[ 15] = 186.6;
    ct[ 16] = 199.1;
    ct[ 17] = 211.5;
    ct[ 18] = 223.9;
    ct[ 19] = 236.4;
    ct[ 20] = 248.8;
    ct[ 21] = 261.3;
    ct[ 22] = 273.7;
    ct[ 23] = 286.1;
    ct[ 24] = 298.6;
    ct[ 25] = 311.0;
    ct[ 26] = 323.5;
    ct[ 27] = 335.9;
    ct[ 28] = 348.3;
    ct[ 29] = 360.8;
    ct[ 30] = 373.2;
    ct[ 31] = 385.7;
    ct[ 32] = 398.1;
    ct[ 33] = 410.5;
    ct[ 34] = 423.0;
    ct[ 35] = 435.4;
    ct[ 36] = 447.9;
    ct[ 37] = 460.3;
    ct[ 38] = 472.8;
    ct[ 39] = 485.2;
    ct[ 40] = 497.6;
    ct[ 41] = 510.1;
    ct[ 42] = 522.5;
    ct[ 43] = 535.0;
    ct[ 44] = 547.4;
    ct[ 45] = 559.8;
    ct[ 46] = 572.3;
    ct[ 47] = 584.7;
    ct[ 48] = 597.2;
    ct[ 49] = 609.6;
    ct[ 50] = 622.0;
    ct[ 51] = 634.5;
    ct[ 52] = 646.9;
    ct[ 53] = 659.4;
    ct[ 54] = 671.8;
    ct[ 55] = 684.2;
    ct[ 56] = 696.7;
    ct[ 57] = 709.1;
    ct[ 58] = 721.6;
    ct[ 59] = 734.0;
    ct[ 60] = 746.5;
    ct[ 61] = 758.9;
    ct[ 62] = 771.3;
    ct[ 63] = 783.8;
    ct[ 64] = 796.2;
    ct[ 65] = 808.7;
    ct[ 66] = 821.1;
    ct[ 67] = 833.5;
    ct[ 68] = 846.0;
    ct[ 69] = 858.4;
    ct[ 70] = 870.9;
    ct[ 71] = 883.3;
    ct[ 72] = 895.7;
    ct[ 73] = 908.2;
    ct[ 74] = 920.6;
    ct[ 75] = 933.1;
    ct[ 76] = 945.5;
    ct[ 77] = 957.9;
    ct[ 78] = 970.4;
    ct[ 79] = 982.8;
    ct[ 80] = 995.3;
    ct[ 81] = 1007.7;
    ct[ 82] = 1020.1;
    ct[ 83] = 1032.6;
    ct[ 84] = 1045.0;
    ct[ 85] = 1057.5;
    ct[ 86] = 1069.9;
    ct[ 87] = 1082.4;
    ct[ 88] = 1094.8;
    ct[ 89] = 1107.2;
    ct[ 90] = 1119.7;
    ct[ 91] = 1132.1;
    ct[ 92] = 1144.6;
    ct[ 93] = 1157.0;
    ct[ 94] = 1169.4;
    ct[ 95] = 1181.9;
    ct[ 96] = 1194.3;
    ct[ 97] = 1206.8;
    ct[ 98] = 1219.2;
    ct[ 99] = 1231.6;
    ct[100] = 1244.1;
}

/*
   vt[0] = 4.15;    //remain capacity=100%
   vt[1] = 4.048;   //remain capacity=99%
   :
   vt[100] = 3.500;   //remain capacity=0%
  binary search voltage for remaining capacity(100-index)%.
*/
int getCapacityPercentVoltage(float voltage) {
    // Clamp limits
    if (voltage >= vt[0]) return 100;
    if (voltage <= vt[100]) return 0;

    int low = 0;
    int high = 100;

    while (low <= high) {
        int mid = low + (high - low) / 2;

        if (vt[mid] == voltage) {
            return 100-mid; // Found exact match
        }
        // Since the array is DESCENDING:
        // If the value at mid is greater than our target, 
        // our target is further right (higher index).
        if (vt[mid] > voltage) {
            low = mid + 1;
        } else {
            high = mid - 1;
        }
    }
    
    // 'low' will hold the index of the closest percentage 
    // rounding towards the lower capacity (safer for battery estimation)
    return 100-low;
}

/*
  ct[0] = 0.0;          //remain capacity=100%
  ct[1] = 12.4;         //remain capacity=99%
    :
  ct[100] = 1244.1;     //remain capacity=0%
  binary search mahConsumed for remaining capacity(100-index)%.
*/
int getCapacityPercentCurrent(float currentMahConsumed) {
    // 1. Clamp out-of-bounds values
    if (currentMahConsumed <= ct[0]) return 100;   // 0 mAh consumed = 100% full
    if (currentMahConsumed >= ct[100]) return 0;   // Max mAh consumed = 0% full

    int low = 0;
    int high = 100;

    while (low <= high) {
        int mid = low + (high - low) / 2;

        if (ct[mid] == currentMahConsumed) {
            // Found exact match. 'mid' represents % consumed.
            return 100 - mid; 
        }
        // Since the array is ASCENDING:
        // If the value at mid is less than our target, 
        // the target must be further to the right (higher index).
        if (ct[mid] < currentMahConsumed) {
            low = mid + 1;
        } else {
            // Target is to the left (lower index).
            high = mid - 1;
        }
    }
    
    // Convert index to % remaining
    // The loop breaks when 'low' crosses 'high'.
    // 'low' points to the first value that is >= currentMahConsumed, 
    // effectively mimicking the behavior of std::lower_bound.
    return 100 - low;
}

void line(int x,int y,const char* msg) {
  int16_t x1,y1;
  uint16_t w,h;
  display.getTextBounds(msg,x,y,&x1,&y1,&w,&h);        //get height
  display.fillRect(x, y, display.width()-x, h,BLACK);  //erase pixels from x to end of line
  display.setCursor(x,y);
  display.print(msg);
  display.display();
}

void loadPrefs(float& mahConsumed) {
  mahConsumed = preferences.getFloat("mahConsumed", -1.0);
  Serial.printf("Prefs mahConsumed:%.1f\r\n",mahConsumed);
}

void clearPrefs() {
  preferences.clear();
}

void savePrefs(float mahConsumed) {
  preferences.putFloat("mahConsumed", mahConsumed);
  Serial.printf("Saved mahConsumed %.1f lastSavePercent %d%%\r\n",mahConsumed,lastSavePercent);
}

void updateOLED(float voltage,int pcv,float totalConsumedMAH,int pcc,bool pwroff) {
  char volt_s[20],bat_s[10],mAh_s[20];

  totalConsumedMAH=fabs(totalConsumedMAH);      //make -0.000 positive
  sprintf(volt_s,"%.2fV %d%%",voltage,pcv);
  line(0,0,volt_s);
  sprintf(mAh_s,"%.1fmAh",totalConsumedMAH);
  line(0,20,mAh_s);
  sprintf(bat_s,"%d%%",pcc);
  line(0,40,bat_s);
  if (pwroff)
    line(50,40,"PWROFF");
}

void updateBatteryLevel(uint32_t period) {
  int pcv=0,pcc=0;

  float current = ina219.getCurrent_mA();
  float voltage = ina219.getBusVoltage_V();

  // Calculate hours passed since last measurement
  float hours = period / 3600000.0;
  totalConsumedMAH += current * hours;
  // --- IR COMPENSATION ---
  // Convert current to Amps for the calculation: V_drop = I(amps) * R(ohms)
  float current_A = current / 1000.0;
  float compensatedVoltage = voltage + (current_A * BATTERY_IR);
  pcv=getCapacityPercentVoltage(compensatedVoltage);            //remaining voltage capacity
  pcc=getCapacityPercentCurrent(totalConsumedMAH);   //remaining mah capacity
  //if (pcv == 100) {                    //is voltage at full capacity%?
  //  pcc=pcv;                           //yes resync to voltage%
  //}
  if (totalConsumedMAH >= ct[100] || voltage <= vt[100]) {
    cutoffCounter++;
    if (cutoffCounter > CHANGE_THRESHOLD) {
      Serial.print("POWER CUTOFF at pcc ");
      Serial.print(pcc);
      Serial.print("%, pcv ");
      Serial.print(voltage,2);
      Serial.println("%");
      updateOLED(voltage,pcv,totalConsumedMAH,pcc,true);
      savePrefs(totalConsumedMAH);
      digitalWrite(CUTOFF_PIN,LOW);    //turn off P-chan MOSFET
      esp_deep_sleep_start();    } 
  } else {
    cutoffCounter=0;
  }
  if ((pcv - pcc) >= MAX_VC_DEVIATION || (pcc - pcv) >= MAX_VC_DEVIATION) {
    deviationCounter++;
    if (deviationCounter > CHANGE_THRESHOLD) {
      Serial.printf("pcc-pcv deviation exceed MAX_VC_DEVIATION, resync pcc %d%% to pcv %d%%\r\n",pcc,pcv);
      pcc=pcv;       //resync to voltage%
      totalConsumedMAH=ct[100-pcc];
      savePrefs(totalConsumedMAH);
      lastSavePercent = pcc;
    }
  } else {
    deviationCounter=0;
  }
  if ((pcc-lastSavePercent) >= 5) {
    savePrefs(totalConsumedMAH);      //save every 5% change to flash
    lastSavePercent = pcc;
  }
  updateOLED(voltage,pcv,totalConsumedMAH,pcc,false);
}

void setup() {
  int pcc,pcv;

  pinMode(CUTOFF_PIN,OUTPUT);
  digitalWrite(CUTOFF_PIN,HIGH);    //turn on P-chan MOSFET
  Serial.begin(115200);
  delay(2000);
  Serial.println("INA219 Fuel Gauge 1.0");
  preferences.begin("FuelGauge", false);
  Wire.begin();
  if (display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    display.clearDisplay();
    display.setTextColor(WHITE);
    display.setRotation(2);     //0=(landscape,pins top),1=(portrait,pins left),2=(land,pins bot)3=(port,pins right)
    display.setTextSize(2);
  }
  else {
    Serial.println(F("SSD1306 allocation failed"));
    while (1) { delay(10); }
  }

  if (!ina219.init()) {
    Serial.println("Failed to find INA219 chip"); 
    while (1) { delay(10); }
  }
  // If you changed the shunt to 1.0 Ohm, you'd calibrate here:
  ina219.setPGain(INA219_PG_40); // choose gain and uncomment for change of default
  ina219.setBusRange(INA219_BRNG_16); // choose range and uncomment for change of default
  ina219.setShuntSizeInOhms(0.1); // used in INA219.
  float voltage = ina219.getBusVoltage_V();     //get voltage

  initVoltageTable();
  initCurrentTable();
  //clearPrefs();
  loadPrefs(totalConsumedMAH);
  if (totalConsumedMAH < 0.0) {   //if totalConsumedMAH is uninitialised
    pcv=getCapacityPercentVoltage(voltage);   //get remain% from voltage
    pcc=pcv;
    totalConsumedMAH=ct[100-pcc];      //get actual mahConsumed from %consumed
    savePrefs(totalConsumedMAH);
    lastSavePercent=pcc;
  }
  prevConsumedMAH = totalConsumedMAH;
  timer2=0;              //start now
  timer2Enabled=true;    //enable battery level check event
}

void loop() {
  uint32_t period;

  period=millis() - timer2;
  if ((period  > period2) && timer2Enabled) {
    timer2 = millis();
    updateBatteryLevel(period);
  }
}