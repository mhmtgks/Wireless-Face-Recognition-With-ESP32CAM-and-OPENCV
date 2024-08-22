#define echopinimiz 13
#define trigpinimiz 12
#define esp 8
#define power 7
int sure;
int mesafe;
void setup() {
pinMode(trigpinimiz, OUTPUT);
pinMode(echopinimiz, INPUT);
pinMode (esp,OUTPUT);
pinMode (power,OUTPUT);
digitalWrite(led,LOW);
Serial.begin(9600);
}
 
void loop() {

digitalWrite(trigpinimiz,HIGH);
//trig pinininden sinyal verdik dışarı
delayMicroseconds(1000) ; //saniyenin milyonda biri
digitalWrite(trigpinimiz,LOW);
sure=pulseIn(echopinimiz,HIGH);
mesafe=(sure/2)/29.1;
Serial.println(mesafe);
delay(250);

//biri yaklaştıysa outputu 1 yaparak espye sinya gidiyor
if (5<mesafe & mesafe<35){
  digitalWrite(esp,HIGH);
  delay(60000);
}
 else {

  digitalWrite(esp,LOW);}
}