
#include <Wire.h>
#include "RTClib.h"

RTC_DS1307 rtc;
DateTime current;
DateTime last;
const uint8_t timezone = 8;

// which analog pin to connect
#define THERMISTORPIN A0         
// resistance at 25 degrees C
const uint16_t THERMISTORNOMINAL = 10000;    
// temp. for nominal resistance (almost always 25 C)
const uint16_t TEMPERATURENOMINAL = 25; 
// how many samples to take and average, more takes longer
// but is more 'smooth'
const uint16_t NUMSAMPLES = 5;
// The beta coefficient of the thermistor (usually 3000-4000)
const uint16_t BCOEFFICIENT = 3435;
// the value of the 'other' resistor
const uint16_t SERIESRESISTOR = 10000; 

uint16_t tSamples[NUMSAMPLES];
int tCount = 0;
int temperature = 88;

const int LED_ON = LOW;
const int LED_OFF= HIGH;

const int digits[10][7] = 
{{ LED_ON,  LED_ON,  LED_ON,  LED_ON,  LED_ON,  LED_ON, LED_OFF},  // 0
 {LED_OFF,  LED_ON,  LED_ON, LED_OFF, LED_OFF, LED_OFF, LED_OFF},  // 1
 { LED_ON,  LED_ON, LED_OFF,  LED_ON,  LED_ON, LED_OFF,  LED_ON},  // 2
 { LED_ON,  LED_ON,  LED_ON,  LED_ON, LED_OFF, LED_OFF,  LED_ON},  // 3
 {LED_OFF,  LED_ON,  LED_ON, LED_OFF, LED_OFF,  LED_ON,  LED_ON},  // 4
 { LED_ON, LED_OFF,  LED_ON,  LED_ON, LED_OFF,  LED_ON,  LED_ON},  // 5
 { LED_ON, LED_OFF,  LED_ON,  LED_ON,  LED_ON,  LED_ON,  LED_ON},  // 6
 { LED_ON,  LED_ON,  LED_ON, LED_OFF, LED_OFF, LED_OFF, LED_OFF},  // 7
 { LED_ON,  LED_ON,  LED_ON,  LED_ON,  LED_ON,  LED_ON,  LED_ON},  // 8
 { LED_ON,  LED_ON,  LED_ON,  LED_ON, LED_OFF,  LED_ON,  LED_ON}}; // 9

 const int letter[4][7] = 
{{ LED_ON,  LED_ON,  LED_ON,  LED_ON,  LED_ON,  LED_ON, LED_OFF},  // G
 {LED_OFF,  LED_ON,  LED_ON, LED_OFF, LED_OFF, LED_OFF, LED_OFF},  // P
 { LED_ON,  LED_ON,  LED_ON,  LED_ON, LED_OFF, LED_OFF,  LED_ON},  // E
 {LED_OFF, LED_OFF, LED_OFF, LED_OFF, LED_OFF, LED_OFF,  LED_ON}}; // -
 
 const int digits_none[7] = {LED_OFF, LED_OFF, LED_OFF, LED_OFF, LED_OFF, LED_OFF, LED_OFF};
// first digit of day is none, 1, 2, 3, with DOWN row
const int digits_day_1x2x3x[3][7] = {{LED_OFF,  LED_ON, LED_OFF,  LED_ON, LED_OFF, LED_OFF, LED_OFF}, // 1
									 { LED_ON,  LED_ON, LED_OFF, LED_OFF,  LED_ON, LED_OFF, LED_OFF}, // 2
									 { LED_ON,  LED_ON, LED_OFF,  LED_ON, LED_OFF, LED_OFF, LED_OFF}}; // 3

const int cols[5] = {44, 43, 42, 41, 40}; // board: 17, 18, 19, 20, 22
//                         A   B   C   D   E   F   G
const int ledpin[2][7] = {{23, 22, 27, 28, 26, 24, 25},  // up row, board: 28, 29, 24, 23, 25, 27, 26
						  {31, 32, 35, 34, 33, 30, 29}}; // down row, board: 12, 11, 8, 9, 10, 13, 15
const int ledpin_xh[7] =  {23, 32, 35, 34, 33, 30, 29};  // for hour less than 10, such as x0, x1, ~ x8, x9

const int ledpin_dx[7] =  {31, 22, 27, 28, 26, 24, 25};  // for day bigger than 9, such as 1x, 2x, 3x
// first digit of month is none or 1, with DOWN row
const int ledpin_month_1x = 34;
// two leds for second blink
const int ledpin_seconds = 33;
// negtive sign for temperature.
const int ledpin_negtive = 25;

const long interval = 500; // seconds blink every half second
unsigned long previousMillis = 0;
int seconds = ON;

// yellow 5v, green gnd, orange tx, white rx 
//const int GPS_POWER = 36; // brown, directly connect to 5v.
//const int GPS_OFF = LOW;
//const int GPS_ON = HIGH;
//const int GPS_EN = 37;    // purple, for Bluetooth only.
bool gps_status = false;
int gps_led_status = LED_ON;

unsigned long last_gps_check_time = 0;
String gpsString;

void setup() {
	int i, j;
  // init all col
	for(i=0; i<5; i++){
		pinMode(cols[i], OUTPUT);
		digitalWrite(cols[i], LED_OFF);
	}
  // init all row
	for(j=0; j<2; j++){
		for(i=0; i<7; i++){
			pinMode(ledpin[j][i], OUTPUT);
			digitalWrite(ledpin[j][i], LED_OFF);
		}
	}
  // init GPS
	gpsString.reserve(1024);
	gpsString = "";
	Serial1.begin(38400,SERIAL_8N1);

  Serial.begin(9600);
  // init RTC
	if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
		while (1);
	}

	if (! rtc.isrunning()) {
    Serial.println("RTC is NOT running!");
	// following line sets the RTC to the date & time this sketch was compiled
		rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
	// This line sets the RTC with an explicit date & time, for example to set
	// January 21, 2014 at 3am you would call:
	// rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
	}

  // init analog input for temperature.
	analogReference(DEFAULT);

//  rtc.adjust(DateTime(2000+17, 10, 17, 8, 0, 0));
  // get current time
	last = rtc.now();
	last_gps_check_time = millis();
	previousMillis = millis();

	readGPS();
}

void loop() {
	uint8_t i;
	unsigned long currentMillis = millis();

	if ((unsigned long)(currentMillis - previousMillis) >= interval) {
	// save the last time you blinked the LED
		previousMillis = currentMillis;
		if (seconds == LED_ON) {
			seconds = LED_OFF;
		} else {
			seconds = LED_ON;
		}

		if(tCount < NUMSAMPLES) {
			tSamples[tCount] = analogRead(THERMISTORPIN);
			tCount ++;
		}else{
	  // average all the samples out
			float average = 0;
			for (i=0; i< NUMSAMPLES; i++) {
				average += tSamples[i];
			}
			average /= NUMSAMPLES;
//        Serial.println(average, DEC);
			temperature = int(tReading2Degree(average) + 0.5);
			tCount = 0;
		}

	// get gps data every one houre.
		if((unsigned long)(currentMillis-last_gps_check_time) >= 3600000L) {
//      if(current.second()==0 && abs(currentMillis-last_gps_check_time) > 30000) {
			readGPS();
		}

		if(gps_status) {
			gps_led_status = LED_ON;
		}else{
			if(gps_led_status == LED_ON)
				gps_led_status = LED_OFF;
			else
				gps_led_status = LED_ON;
		}

//      current = rtc.now();
		
	}
  current = DateTime(last.unixtime() + ((unsigned long)(currentMillis-last_gps_check_time))/1000);
	showDateTime(current.month(), current.day(), current.hour(), current.minute(), seconds, current.dayOfTheWeek(), temperature, gps_led_status);

}

/*
  M: month
  D: day
  h: hour
  m: minute, int
  s: second, ON or OFF
  w: week day
  t: temperature
*/
void showDateTime(int M, int D, int h, int m, int s, int w, int t, int gps){
	int j;
	int delay_t = 1;

	if(w == 0){ w = 8;}

	int tLow = abs(t) % 10;
	for(j=0; j<7; j++){
	// low digit of temp
		if(t < -9 && s == LED_OFF) {
			digitalWrite(ledpin[0][j], digits_none[j]);
			digitalWrite(ledpin_negtive, LED_ON);
		}else{
			digitalWrite(ledpin[0][j], digits[tLow][j]);
		}
	// week day
		digitalWrite(ledpin[1][j], digits[w][j]);
	}
	digitalWrite(cols[0], LED_ON);
	delay(delay_t);
	digitalWrite(cols[0], LED_OFF);

	int mLow = m % 10;
	int tHigh = abs(t) / 10;
	for(j=0; j<7; j++){
	// high digit of temp
		if(t>0){
			if(tHigh > 0) {
				digitalWrite(ledpin[0][j], digits[tHigh][j]);
			}else{
				digitalWrite(ledpin[0][j], digits_none[j]);
			}
		}else if(t < 0) {
			if(tHigh > 0) {
				if(s == LED_OFF) {
					digitalWrite(ledpin[0][j], digits_none[j]);
					digitalWrite(ledpin_negtive, LED_ON);
				}else{
					digitalWrite(ledpin[0][j], digits[tHigh][j]);
				}
			}else{
				digitalWrite(ledpin[0][j], digits_none[j]);
				digitalWrite(ledpin_negtive, LED_ON);
			}
		}else{
			digitalWrite(ledpin[0][j], digits_none[j]);
		}

	// low digit of minutes
		digitalWrite(ledpin[1][j], digits[mLow][j]);
	}
	digitalWrite(cols[1], LED_ON);
	delay(delay_t);
	digitalWrite(cols[1], LED_OFF);

	int dLow = D % 10;
	int mHigh = m / 10;
	for(j=0; j<7; j++){
	// low digit of day
		digitalWrite(ledpin[0][j], digits[dLow][j]);
	// high digit of minutes
		digitalWrite(ledpin[1][j], digits[mHigh][j]);
	}
	digitalWrite(cols[2], LED_ON);
	delay(delay_t);
	digitalWrite(cols[2], LED_OFF);

	int dHigh = D / 10;
	int hLow = h % 10;
	for(j=0; j<7; j++){
	// high digit of day
		if(dHigh>0){
			digitalWrite(ledpin_dx[j], digits[dHigh][j]);
		}else{
			digitalWrite(ledpin_dx[j], digits_none[j]);
		}
	// low digit of hours
		digitalWrite(ledpin_xh[j], digits[hLow][j]);
	}
	digitalWrite(ledpin[0][6], gps);  // world calendar for gps status
	digitalWrite(ledpin[0][5], LED_OFF);  // chinese calendar is off
	digitalWrite(cols[3], LED_ON);
	delay(delay_t);
	digitalWrite(cols[3], LED_OFF);

	int MLow = M % 10;
	int hHigh = h / 10;
	for(j=0; j<7; j++){
	// low digit of month
		digitalWrite(ledpin[0][j], digits[MLow][j]);
	// high digit of month
		if(M/10 > 0){
			digitalWrite(ledpin_month_1x, LED_ON);
		}else{
			digitalWrite(ledpin_month_1x, LED_OFF);
		}
	// high digit of hours
		if(hHigh>0){
			digitalWrite(ledpin[1][j], digits[hHigh][j]);
		}else{
			digitalWrite(ledpin[1][j], digits_none[j]);
		}
	}
	digitalWrite(ledpin_seconds, s);
	digitalWrite(ledpin[1][6], LED_OFF);  // user alarm is off
	digitalWrite(ledpin[1][5], LED_OFF);  // every hour alarm is off
	digitalWrite(cols[4], LED_ON);
	delay(delay_t);
	digitalWrite(cols[4], LED_OFF);

}

float tReading2Degree(float reading) {
	float resistor = SERIESRESISTOR * (1023 - reading) / reading;
	//  Serial.print(resistor);
	//  Serial.print("\t");
	float steinhart;
	steinhart = resistor / THERMISTORNOMINAL;     // (R/Ro)
	steinhart = log(steinhart);                  // ln(R/Ro)
	steinhart /= BCOEFFICIENT;                   // 1/B * ln(R/Ro)
	steinhart += 1.0 / (TEMPERATURENOMINAL + 273.15); // + (1/To)
	steinhart = 1.0 / steinhart;                 // Invert
	steinhart -= 273.15;                         // convert to C
	//  Serial.println(steinhart);
	return steinhart;
}

void readGPS() {

	unsigned long t; 
	unsigned long timeout = 3000;  // 3 seconds timeout
	// clean Serial buffer.
	while (Serial1.available()) {
		Serial1.read();
	}
	gps_status = false;
	gpsString = "";
	t = millis();
	while(millis() - t < timeout) {
		if (Serial1.available()) {
			gpsString += (char)Serial1.read();
			if(gpsString.endsWith("\r\n")) {
	//        Serial.print(gpsString);
				if(gpsString.startsWith("$GPRMC,")) {
					gps_status = getGPSTime();
				  if(gps_status) {// success 
						last_gps_check_time = millis();
						return;
					}
				}
				gpsString.remove(0);
			}
		}
	}

	// failed
	last = DateTime(last.unixtime() + ((unsigned long)(millis() - last_gps_check_time))/1000);  
	last_gps_check_time = millis();
}

bool getGPSTime() {
	int i;

//  Serial.println(gpsString);

	if(!checksum(gpsString))
		return false;

//  if(gpsString.startsWith("$GPRMC,")){
	int comma_index = gpsString.indexOf(',');
	int end_index = gpsString.indexOf(',',comma_index+1); // Time
	String t_String = gpsString.substring(comma_index+1, end_index);
	
	//Navigation receiver warning A = OK, V = invalid
	if(gpsString.charAt(end_index+1) == 'V') return false;
	
//    last_gps_check_time = millis();
	comma_index = 0;
	for(i=0; i< 9; i++){
		comma_index = gpsString.indexOf(',',comma_index+1); 
	}
	end_index = gpsString.indexOf(',',comma_index+1); // Date
	String d_String = gpsString.substring(comma_index+1, end_index);
	
//        Serial.print(t_String);Serial.print("\t");Serial.println(d_String);
	int dd = d_String.substring(0,2).toInt();
	int MM = d_String.substring(2,4).toInt();
	int yy = d_String.substring(4,6).toInt();
	
	int hh = t_String.substring(0,2).toInt();
	int mm = t_String.substring(2,4).toInt();
	int ss = t_String.substring(4,6).toInt();

	if (hh+timezone >= 24) {
		hh = hh + timezone - 24;
		dd ++;
		if(dd == 29 && MM == 2) {
			if(!isLeapYear(yy)){
				dd = 1;
				MM ++;
			}
		}else if(dd == 30 && MM == 2) {
			dd = 1;
			MM ++;
		}else if(dd == 31){
			if(MM == 4 || MM == 6 || MM == 9 || MM == 11){
				dd = 1;
				MM ++;
			}
		}else if(dd == 32){
			dd = 1;
			MM ++;
		}

	}else{
		hh += timezone;
	}

//          Serial.print(yy);Serial.print("-");
//          Serial.print(MM);Serial.print("-");
//          Serial.print(dd);Serial.print(" ");
//          Serial.print(hh);Serial.print(":");
//          Serial.print(mm);Serial.print(":");
//          Serial.println(ss);
	rtc.adjust(DateTime(2000+yy, MM, dd, hh, mm, ss));
	last = DateTime(2000+yy, MM, dd, hh, mm, ss);
	return true;

}

bool checksum(String gprmc) {
	int indx = gprmc.indexOf('*');
	if(indx == -1) return false;

	int i;
	int c = 0;
	for(i=1;i<indx;i++) {
		c ^= gprmc.charAt(i);
	}
//  Serial.println(c);
	if(c == strtol(gprmc.substring(indx+1).c_str(), NULL, 16))
		return true;
	else
		return false;
}

bool isLeapYear(int y){
	if(y%100 == 0){
		if(y%400 == 0)
			return true;
	}else{
		if(y%4 == 0)
			return true;
	}
	return false;
}
