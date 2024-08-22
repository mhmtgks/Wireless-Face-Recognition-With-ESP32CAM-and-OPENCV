#include <WebServer.h>
#include <WiFi.h>
#include <esp32cam.h>
//mail kütüphanesi
#include <ESP_Mail_Client.h>
 
const char* WIFI_SSID = "TurkTelekom_TP4254_2.4GHz";
const char* WIFI_PASS = "7npEXEmJug4X";
 
//smtp configürasyonu
#define SMTP_HOST "smtp.gmail.com"
#define SMTP_PORT 465

//gönderici şifre bilgisi
#define AUTHOR_EMAIL "gönderen@gmail.com"
#define AUTHOR_PASSWORD "*************"


#define RECIPIENT_EMAIL "alıcı@gmail.com"
//smtp objesi tanımladık
SMTPSession smtp;

WiFiServer server(80);
//http için gerekli header
String header;
// çıkış değerlerini ilk değer ataması yapıyoruz
String output12State = "off";
String output14State = "off";
// GPIO pinlerini ayarlıyoruz
const int output12 = 13;
const int output14 = 15;
const int input16 =2;

unsigned long currentTime = millis();

unsigned long previousTime = 0;
const long timeoutTime = 2000;
 
//çözünürlük ayarlıyoruz
static auto loRes = esp32cam::Resolution::find(320, 240);
static auto midRes = esp32cam::Resolution::find(350, 530);
static auto hiRes = esp32cam::Resolution::find(800, 600);

int state=0;
int mesafe=0;
int sure=0; 

//smtp durumunu dönderen fonksiyon
void smtpCallback(SMTP_Status status);

//resmi yakalama fonksiyonu
void serveJpg()
{
  auto frame = esp32cam::capture();
  if (frame == nullptr) {
    Serial.println("CAPTURE FAIL");
    server.send(503, "", "");
    return;
  }
  Serial.printf("CAPTURE OK %dx%d %db\n", frame->getWidth(), frame->getHeight(),
                static_cast<int>(frame->size()));
 
  server.setContentLength(frame->size());
  server.send(200, "image/jpeg");
  WiFiClient client = server.client();
  frame->writeTo(client);
}
// low çözünürlük istenirse kullanılan fonksiyon
void handleJpgLo()
{
  if (!esp32cam::Camera.changeResolution(loRes)) {
    Serial.println("SET-LO-RES FAIL");
  }
  serveJpg();
}

// high çözünürlük istenirse kullanılan fonksiyon
void handleJpgHi()
{
  if (!esp32cam::Camera.changeResolution(hiRes)) {
    Serial.println("SET-HI-RES FAIL");
  }
  serveJpg();
}
// mid çözünürlük istenirse kullanılan fonksiyon
void handleJpgMid()
{
  if (!esp32cam::Camera.changeResolution(midRes)) {
    Serial.println("SET-MID-RES FAIL");
  }
  serveJpg();
}

void sendMail(){
  //smtp clientina bağlanıyoruz
  MailClient.networkReconnect(true);
  smtp.callback(smtpCallback);

  Session_Config config;

  /* smtp nin configleri yapılıyor */
  config.server.host_name = SMTP_HOST;
  config.server.port = SMTP_PORT;
  config.login.email = AUTHOR_EMAIL;
  config.login.password = AUTHOR_PASSWORD;
  config.login.user_domain = "";
// mail için zaman bilgisi alıyoruz
  config.time.ntp_server = F("pool.ntp.org,time.nist.gov");
  config.time.gmt_offset = 3;
  config.time.day_light_offset = 0;
 
  SMTP_Message message;

  
   // mesaj ayarlanıyor
  message.sender.name = F("Güvenlik sistemi");
  message.sender.email = AUTHOR_EMAIL;
  message.subject = F("bilinmeyen yüz saptandı");
  message.addRecipient(F("Mehmet"), RECIPIENT_EMAIL)

  /** Two alternative content versions are sending in this example e.g. plain text and html */
  String htmlMsg = "This message contains attachments: image and text file.";
  message.html.content = htmlMsg.c_str();
  message.html.charSet = "utf-8";
  message.html.transfer_encoding = Content_Transfer_Encoding::enc_qp;

  message.priority = esp_mail_smtp_priority::esp_mail_smtp_priority_normal;
  message.response.notify = esp_mail_smtp_notify_success | esp_mail_smtp_notify_failure | esp_mail_smtp_notify_delay;

  //resim yakalayıp kaydediyoruz
  file.write(esp32cam::capture()->buf, esp32cam::capture()->len); // payload (image), payload length
  Serial.printf("Saved file to path: %s\n", path.c_str());


  /* resim ekliyoruz*/
  SMTP_Attachment att;
  att.file.path = path.c_str();
  att.file.storage_type = esp_mail_file_storage_type_flash;
  att.descr.transfer_encoding = Content_Transfer_Encoding::enc_base64;
  message.addAttachment(att);

  message.resetAttachItem(att);
  att.descr.filename = "text_file.txt";
  att.descr.mime = "text/plain";
  att.file.path = "/text_file.txt";
  att.file.storage_type = esp_mail_file_storage_type_flash;
  att.descr.transfer_encoding = Content_Transfer_Encoding::enc_base64;

  /* Add attachment to the message */
  message.addAttachment(att);

  /* Connect to server with the session config */
  if (!smtp.connect(&config)){
    ESP_MAIL_PRINTF("Connection error, Status Code: %d, Error Code: %d, Reason: %s", smtp.statusCode(), smtp.errorCode(), smtp.errorReason().c_str());
    return;
  }
  // mail gönderim oturumu kapatıyoruz
  if (!MailClient.sendMail(&smtp, &message, true))
    Serial.println("Error sending Email, " + smtp.errorReason());
}
 
 
void  setup(){
  Serial.begin(115200);

    // giriş çıkış pinlerini belirtiyoruz
  pinMode(output12, OUTPUT);
  pinMode(output14, OUTPUT);
  pinMode(input16, INPUT);

  // çıkışları low olarak ayarlıyoruz
  digitalWrite(output12, LOW);
  digitalWrite(output14, LOW);
  Serial.println();
  {
    //esp32 konfigüre ediyoruz
    using namespace esp32cam;
    Config cfg;
    cfg.setPins(pins::AiThinker);
    cfg.setResolution(hiRes);
    cfg.setBufferCount(2);
    cfg.setJpeg(80);
 
    bool ok = Camera.begin(cfg);
    Serial.println(ok ? "CAMERA OK" : "CAMERA FAIL");
  }
  //wifi a bağlama işlemi yapıyoruz
  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
  Serial.print("http://");
  Serial.println(WiFi.localIP());
  Serial.println("  /cam-lo.jpg");
  Serial.println("  /cam-hi.jpg");
  Serial.println("  /cam-mid.jpg");
  //serveri başlatıyoruz
  server.on("/cam-lo.jpg", handleJpgLo);
  server.on("/cam-hi.jpg", handleJpgHi);
  server.on("/cam-mid.jpg", handleJpgMid);
 
  server.begin();
}
 
void loop()
{
  //state 1 ise yani mesafe sensöründen veri gelmiş ise client başlıyor statenin değişim kısmı bu if bloğunun else kısmında
  // sebebi ilk çalışmada buraya girmeden açıkken direkt else bölümüne düşmesi
  if(state==1){
  WiFiClient client = server.available();   // olan clientları dinliyoruzz
  if (client) {                             
    currentTime = millis();
    previousTime = currentTime;
    Serial.println("New Client.");          
    String currentLine = "";                
    while (client.connected() && currentTime - previousTime <= timeoutTime) {  // client bağlı olduğu sürece dönen döngü yazıyoruz
      currentTime = millis();
      if (client.available()) {             // client ayaktaysa okuma yapıyoruz
        char c = client.read();             
        Serial.write(c);                    
        header += c;
        if (c == '\n') {                   
          if (currentLine.length() == 0) {
            //http response statusunu serial monitörde görmek için başarı durumunda http ok olan 200 yazdırıyoruz
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();
            // eğer cevap olumsuzsa yani yüz tanımlanmadıysa çalışan fonksiyon
            if (header.indexOf("GET /12/on") >= 0) {
              Serial.println("GPIO 12 on");
              output12State = "on";
              digitalWrite(output12, HIGH);
            } else if (header.indexOf("GET /14/off") >= 0) {
              Serial.println("GPIO 14 off");
              output14State = digitalRead(input16);
              digitalWrite(output14, LOW);}
            // eğer cevap olumluysa çalışan kod bloğu
              else if (header.indexOf("GET /12/off") >= 0) {
              Serial.println("GPIO 12 off");
              output12State = "off";
              digitalWrite(output12, LOW);
            } else if (header.indexOf("GET /14/on") >= 0) {
              Serial.println("GPIO 14 on");
              output14State = "on";
              digitalWrite(output14, HIGH);
            } 
            }
            //while döngüsünü kırmak için kullanılan break sebibi bir client brute force yapmasın diye
            break;
          } else { 
            currentLine = "";
          }
        } else if (c != '\r') { 
          currentLine += c;      
        }
      }
    }
    // header temizleniyor en baştaki haline dönmek için
    header = "";
    // bağlantı sonlandırılıyor
    client.stop();
    Serial.println("Client disconnected.");
    Serial.println("");
    Serial.println(digitalRead(input16));
  }
  }else{

  // bu kısım eğerki arduniodan gelen ve bizim inputumuz 1 ise yani mesafe sensörü çalıştıysa state yi 1
  // durumuna getiren ve iki lambaya blink yaptıran kısım
      if(digitalRead(input16)){
      Serial.println(digitalRead(input16));
      digitalWrite(output12, HIGH);
      digitalWrite(output14, HIGH);
      delay(500);
      state=1;
      digitalWrite(output12, LOW);
      digitalWrite(output14, LOW);
  }
  }

  server.handleClient();
}