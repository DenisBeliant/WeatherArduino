#define LUM A0
#define TEMP A1
#define WIND 2

#define BLUE 5
#define GREEN 6
#define RED 7

#include <SPI.h>
#include <Ethernet.h>
#include <Blynk.h>
#include <BlynkSimpleEthernet.h>

// Enter a MAC address and IP address for your controller below.
// The IP address will be dependent on your local network:
byte mac[] = {
  0xA8, 0x61, 0x0A, 0xAE, 0x68, 0x3B
};

// Initialize the Ethernet server library
// with the IP address and port you want to use
// (port 80 is default for HTTP):

EthernetServer server(80);
EthernetClient client;

char auth[] = "mdPyjyhdtPk5T-rVl38N5jQz1jFHJguH";

unsigned long int tPrec, tRise, tPer = 0;

bool canRead = true;
bool blinkLed = true;

int wind;
int temp;
int weather;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  pinMode(LUM, INPUT);
  pinMode(TEMP, INPUT);
  pinMode(WIND, INPUT);

  pinMode(RED, OUTPUT);
  pinMode(GREEN, OUTPUT);
  pinMode(BLUE, OUTPUT);



  attachInterrupt(digitalPinToInterrupt(WIND), inter, RISING);

  Ethernet.begin(mac);

  // start the server
  server.begin();
  Blynk.begin(auth);
  Serial.println(Ethernet.localIP());

}

void loop() {

  Blynk.run();

  if (canRead) {

    if (tRise - tPrec > 0) tPer = tRise - tPrec;

    tPrec = tRise;

    wind = findSpeed(tPer);
    temp = getTemp(analogRead(TEMP));
    weather = getWeather(analogRead(LUM));

    Serial.print(wind);
    Serial.print(':');
    Serial.print(temp);
    Serial.print(':');
    Serial.print(weather);
    Serial.println();

  } else {

    Serial.print("Maintenance");

    digitalWrite(RED, blinkLed);
    digitalWrite(GREEN, 0);
    digitalWrite(BLUE, 0);

    if (millis() / 100 % 20 == 0) blinkLed = !blinkLed;

    Serial.println();
  }

  if (Serial.read() == 'm') canRead = !canRead;
  laToile(wind, temp, weather);
}

void inter() {
  tRise = millis();
}


float findSpeed(long int tick) {

  return abs(2400 / tick);

}

float getTemp(int tmp) {
  float volt = (tmp) * 5.0;
  volt /= 1024;

  return (volt - 0.5) * 100;
}

int getWeather(int val) {
  digitalWrite(RED, 0);
  digitalWrite(GREEN, 0);
  digitalWrite(BLUE, 0);

  if (val > 200) {
    digitalWrite(RED, 1);
  }
  else if (val > 80) {
    digitalWrite(GREEN, 1);
  }
  else {
    digitalWrite(BLUE, 1);
  }

  return val;

}

void laToile(int wind, int temp, int weather) {
  client = server.available();

  if (client) {
    // an http request ends with a blank line
    boolean currentLineIsBlank = true;
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        // if you've gotten to the end of the line (received a newline
        // character) and the line is blank, the http request has ended,
        // so you can send a reply
        if (c == '\n' && currentLineIsBlank) {

          // send a standard http response header
          client.println(F("HTTP/1.1 200 OK"));
          client.println(F("Content-Type: text/html"));
          client.println(F("Connection: close"));  // the connection will be closed after completion of the response
          client.println(F("Refresh: 5"));  // refresh the page automatically every 5 sec
          client.println();
          client.println(F("<!DOCTYPE html>"));
          client.println(F("<html lang='en'>"));
          client.println(F("<head>"));
          client.println(F("<meta charset='UTF-8'>"));
          client.println(F("<link rel='stylesheet' href='https://cdn.jsdelivr.net/npm/bootstrap@4.5.3/dist/css/bootstrap.min.css' integrity='sha384-TX8t27EcRE3e/ihU7zmQxVncDAy5uIKz4rEkgIXeMed4M0jlfIDPvg6uqKI2xXr2' crossorigin='anonymous'>"));
          client.println(F("<title>Météo en direct :</title>"));
          client.println(F("</head>"));
          client.println(F("<body style='background-color: rgb(222, 255, 253);'>"));
          client.println(F("<h1 style='margin: auto; text-align: center; margin-bottom: 50px;'>Météo :</h1>"));

          if (canRead) {

            client.println(F("<div class='card alert-info' style='width: 60%; margin: auto; margin-bottom: 10px; text-align: center;'><div class='card-body'><div class='card-title'>Temperature :</div><div class='progress'><div class='progress-bar progress-bar-striped bg-danger'"));
            client.print(genHTML(temp, 40));
            client.println(F(" C</div></div></div></div>"));
            client.println(F("<div class='card alert-info' style='width: 60%; margin: auto; margin-bottom: 10px; text-align: center;'><div class='card-body'><div class='card-title'>Vent :</div><div class='progress'><div class='progress-bar progress-bar-striped bg-danger'"));
            client.print(genHTML(wind, 50));
            client.println(F(" km/h</div></div></div></div>"));
            client.println(F("<div class='card alert-info' style='width: 60%; margin: auto; margin-bottom: 10px; text-align: center;'><div class='card-body'><div class='card-title'>"));
            client.print(F("<img src='"));
            client.print(weather > 200 ? "https://pngimg.com/uploads/sun/sun_PNG13427.png" : weather > 50 ? "https://www.transparentpng.com/thumb/clouds/xapA4x-clouds-sky-png-picture.png" : "https://beeimg.com/images/h41731038092.png");
            client.print("' width='200 px' height=' 200px' />");
            client.println(F("</div>"));

          } else client.print(F("<h2 style='color: red;'>Maintenance en cours..</h2>"));

          client.println(F("</div>"));
          client.println(F("</body>"));
          client.println(F("</html>"));
          break;
        }
        if (c == '\n') {
          // you're starting a new line
          currentLineIsBlank = true;
        } else if (c != '\r') {
          // you've gotten a character on the current line
          currentLineIsBlank = false;
        }
      }
    }
    // give the web browser time to receive the data
    delay(1);
    // close the connection:
    client.stop();
  }
}

String genHTML(int val, int max) {

  char page[150];


  sprintf(page, "aria-valuenow='%i' role='progressbar' aria-valuemin='0' aria-valuemax='%i' style='width: %i%c'> %i", val, max, (val * 100) / max, '%', val);

  return page;
}
