const int GPS_POWER = 36;
String buffer = String(100);

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  Serial1.begin(38400,SERIAL_8N1);
//  Serial.println("OK");
//  delay(1000); 
//  pinMode(GPS_POWER, OUTPUT);
//  digitalWrite(GPS_POWER, HIGH);
}

void loop() {
  if (Serial1.available()) {
    buffer = Serial1.readStringUntil('\r\n');
    Serial.println(buffer);
//    if(buffer.startsWith("$GPGGA,")){
//      int t_index = buffer.indexOf(',',7);
//      String t_String = buffer.substring(7,t_index);
//      int hh = t_String.substring(0,2).toInt();
//      int mm = t_String.substring(2,4).toInt();
//      int ss = t_String.substring(4,6).toInt();
//      Serial.print(t_String);Serial.print("\t");
//      if (hh+8 > 24) 
//        hh = hh + 8 - 24;
//      else
//        hh += 8;
//      Serial.print(hh);Serial.print("-");
//      Serial.print(mm);Serial.print("-");
//      Serial.print(ss);Serial.println("");
//    }
  }
  
}
