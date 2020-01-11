/*
remix from HarryFun's great Morphing Digital Clock idea https://github.com/hwiguna/HariFun_166_Morphing_Clock
follow the great tutorial there and eventually use this code as alternative

provided 'AS IS', use at your own risk
 * mirel.t.lazar@gmail.com
 * 
 * 
We also need ESP8266 support
Close Manage Libraries, but stay in Arduino IDE
Go to File > Preferences
Click the icon to the right of Additional Board Manager URLs
Paste this URL on a separate line (sequence does not matter).
http://arduino.esp8266.com/stable/package_esp8266com_index.json
Click Ok to get out of Preferences
Navigate to: Tools > Board xyz > Board Manager...
Search for 8266
Install esp8266 by ESP8266 Community.

in case of errors related to NTP or time:
To get past these errors you need to install these library's:
--not used: NTPClient by Fabrice Weinberg https://github.com/arduino-libraries/NTPClient
Time 1.5 by Michael Margolis https://github.com/PaulStoffregen/Time
NtpClientLib 3.0.2-beta by Germán Martín https://github.com/gmag11/NtpClient
PxMatrix 1.6.0 by Dominic Buchstaler https://github.com/2dom/PxMatrix
*/

#include <TimeLib.h>
#include <NtpClientLib.h>
#include <ESP8266WiFi.h>

#define double_buffer
#include <PxMatrix.h>

//#define USE_ICONS
//#define USE_WEATHER_ANI
#define USE_FIREWORKS

#include "FS.h"

#ifdef ESP8266
#include <Ticker.h>
Ticker display_ticker;
#define P_LAT 16
#define P_A 5
#define P_B 4
#define P_C 15
#define P_D 12
#define P_E 0
#define P_OE 2
#endif

WiFiServer httpsvr (80); //Initialize the server on Port 80

// Pins for LED MATRIX
PxMATRIX display(64, 32, P_LAT, P_OE, P_A, P_B, P_C, P_D, P_E);
#include "TinyFont.h"
const byte row0 = 2+0*10;
const byte row1 = 2+1*10;
const byte row2 = 2+2*10;

//=== SEGMENTS ===
int cin = 25; //color intensity
#include "Digit.h"
Digit digit0(&display, 0, 63 - 1 - 9*1, 8, display.color565(0, 0, 255));
Digit digit1(&display, 0, 63 - 1 - 9*2, 8, display.color565(0, 0, 255));
Digit digit2(&display, 0, 63 - 4 - 9*3, 8, display.color565(0, 0, 255));
Digit digit3(&display, 0, 63 - 4 - 9*4, 8, display.color565(0, 0, 255));
Digit digit4(&display, 0, 63 - 7 - 9*5, 8, display.color565(0, 0, 255));
Digit digit5(&display, 0, 63 - 7 - 9*6, 8, display.color565(0, 0, 255));

/*
 * the following parameters need to be defined in params.h
 * 
//wifi network
char wifi_ssid[] = "";
//wifi password
char wifi_pass[] = "";
//timezone
char timezone[5] = "1";
//use 24h time format
char military[3] = "Y";     // 24 hour mode? Y/N
//use metric data
char u_metric[3] = "Y";     // use metric for units? Y/N
//date format
char date_fmt[7] = "D.M.Y"; // date format: D.M.Y or M.D.Y or M.D or D.M or D/M/Y.. looking for trouble
//open weather map api key 
String apiKey   = ""; //e.g a hex string like "abcdef0123456789abcdef0123456789"
//the city you want the weather for 
String location = "Muenchen,DE"; //e.g. "Paris,FR"
 * 
 */
#include "params.h"

#ifdef ESP8266
// ISR for display refresh
void display_updater ()
{
  //display.displayTestPattern(70);
  display.display (70);
}
#endif

void getWeather ();

byte hh;
byte mm;
byte ss;
byte ntpsync = 1;
const char ntpsvr[]   = "pool.ntp.org";
//settings
#define NVARS 15
#define LVARS 12
char c_vars [NVARS][LVARS];
typedef enum e_vars {
  EV_SSID = 0,
  EV_SSID2 = 0,
  EV_PASS,
  EV_PASS2,
  EV_TZ,
  EV_24H,
  EV_METRIC,
  EV_DATEFMT,
  EV_OWMK,
  EV_OWMK2,
  EV_OWMK3,
  EV_GEOLOC,
  EV_GEOLOC2,
  EV_GEOLOC3,
  EV_DST,
  EV_MAX
};

bool toBool (String s)
{
  return s.equals ("true");
}

int vars_read ()
{
  File varf = SPIFFS.open ("/vars.cfg", "r");
  if (!varf)
  {
    Serial.println ("Failed to open config file");
    return 0;
  }
  //read vars
  for (int i = 0; i < NVARS; i++)
    for (int j = 0; j < LVARS; j++)
      c_vars[i][j] = (char)varf.read ();
  //
  for (int i = 0; i < NVARS; i++)
  {
    Serial.print ("var ");
    Serial.print (i);
    Serial.print (": ");
    Serial.println (c_vars[i]);
  }
  //
  varf.close ();
  return 1;
}

int vars_write ()
{
  File varf = SPIFFS.open ("/vars.cfg", "w");
  if (!varf)
  {
    Serial.println ("Failed to open config file");
    return 0;
  }
  //read vars
  for (int i = 0; i < NVARS; i++)
    for (int j = 0; j < LVARS; j++)
      if (varf.write (c_vars[i][j]) != 1)
        Serial.println ("error writing var");
  //
  varf.close ();
  return 1;
}
//
void setup ()
{	
	Serial.begin (115200);
  while (!Serial)
    delay (500); //delay for Leonardo
  //display setup
  display.begin (16);
#ifdef ESP8266
  display_ticker.attach (0.002, display_updater);
#endif
  //
  Serial.println ("");
  Serial.print ("Connecting");
  TFDrawText (&display, String ("   CONNECTING   "), 0, 13, display.color565(0, 0, 255));
  //connect to wifi network
  WiFi.begin (wifi_ssid, wifi_pass);
  while (WiFi.status () != WL_CONNECTED)
  {
    delay (500);
    Serial.print(".");
  }
  Serial.println ("success!");
  Serial.print ("IP Address is: ");
  Serial.println (WiFi.localIP ());  //
  TFDrawText (&display, String("     ONLINE     "), 0, 13, display.color565(0, 0, 255));
  //
  if (SPIFFS.begin ())
  {
    Serial.println ("SPIFFS Initialize....ok");
    if (!vars_read ())
    {
      //init vars
      strcpy (c_vars[EV_TZ], timezone);
      strcpy (c_vars[EV_24H], military);
      strcpy (c_vars[EV_METRIC], u_metric);
      strcpy (c_vars[EV_DATEFMT], date_fmt);
      strcpy (c_vars[EV_DST], "false");
    }
  }
  else
  {
    Serial.println ("SPIFFS Initialization...failed");
  }  //
  Serial.print ("timezone=");
  Serial.println (c_vars[EV_TZ]);
  Serial.print ("military=");
  Serial.println (c_vars[EV_24H]);
  Serial.print ("metric=");
  Serial.println (c_vars[EV_METRIC]);
  Serial.print ("date-format=");
  Serial.println (c_vars[EV_DATEFMT]);
  Serial.print ("dst=");
  Serial.println (c_vars[EV_DST]);
  //start NTP
  NTP.begin (ntpsvr, String (c_vars[EV_TZ]).toInt(), toBool (String (c_vars[EV_DST])));
  NTP.setInterval (10);//force rapid sync in 10sec
  //
	NTP.onNTPSyncEvent ([](NTPSyncEvent_t ntpEvent) 
	{
		if (ntpEvent) 
		{
			Serial.print ("Time Sync error: ");
			if (ntpEvent == noResponse)
				Serial.println ("NTP server not reachable");
			else if (ntpEvent == invalidAddress)
				Serial.println ("Invalid NTP server address");
		}
		else 
		{
			Serial.print ("Got NTP time: ");
			Serial.println (NTP.getTimeDateString (NTP.getLastNTPSync ()));
      ntpsync = 1;
		}
	});
  //
  httpsvr.begin (); // Start the HTTP Server
  //delay (1500);
  getWeather ();
  //prep screen for clock display
  display.fillScreen (0);
  int cc_gry = display.color565 (128, 128, 128);
  //reset digits color
  digit0.SetColor (cc_gry);
  digit1.SetColor (cc_gry);
  digit2.SetColor (cc_gry);
  digit3.SetColor (cc_gry);
  digit4.SetColor (cc_gry);
  digit5.SetColor (cc_gry);
  digit1.DrawColon (cc_gry);
  digit3.DrawColon (cc_gry);
  //
  Serial.print ("display color range [");
  Serial.print (display.color565 (0, 0, 0));
  Serial.print (" .. ");
  Serial.print (display.color565 (255, 255, 255));
  Serial.println ("]");
}

const char server[]   = "api.openweathermap.org";
WiFiClient client;
int tempMin = -10000;
int tempMax = -10000;
int tempM = -10000;
int presM = -10000;
int humiM = -10000;
int condM = -1;  //-1 - undefined, 0 - unk, 1 - sunny, 2 - cloudy, 3 - overcast, 4 - rainy, 5 - thunders, 6 - snow
String condS = "";
void getWeather ()
{
  if (!apiKey.length ())
  {
    Serial.println ("w:missing API KEY for weather data, skipping"); 
    return;
  }
  Serial.print ("i:connecting to weather server.. "); 
  // if you get a connection, report back via serial: 
  if (client.connect (server, 80))
  { 
    Serial.println ("connected."); 
    // Make a HTTP request: 
    client.print ("GET /data/2.5/weather?"); 
    client.print ("q="+location); 
    client.print ("&appid="+apiKey); 
    client.print ("&cnt=1"); 
    (*u_metric=='Y')?client.println ("&units=metric"):client.println ("&units=imperial");
    client.println ("Host: api.openweathermap.org"); 
    client.println ("Connection: close");
    client.println (); 
  } 
  else 
  { 
    Serial.println ("w:unable to connect");
    return;
  } 
  delay (1000);
  String sval = "";
  int bT, bT2;
  //do your best
  String line = client.readStringUntil ('\n');
  if (!line.length ())
    Serial.println ("w:unable to retrieve weather data");
  else
  {
    Serial.print ("weather:"); 
    Serial.println (line); 
    //weather conditions - "main":"Clear",
    bT = line.indexOf ("\"main\":\"");
    if (bT > 0)
    {
      bT2 = line.indexOf ("\",\"", bT + 8);
      sval = line.substring (bT + 8, bT2);
      Serial.print ("cond ");
      Serial.println (sval);
      //0 - unk, 1 - sunny, 2 - cloudy, 3 - overcast, 4 - rainy, 5 - thunders, 6 - snow
      condM = 0;
      if (sval.equals("Clear"))
        condM = 1;
      else if (sval.equals("Clouds"))
        condM = 2;
      else if (sval.equals("Overcast"))
        condM = 3;
      else if (sval.equals("Rain"))
        condM = 4;
      else if (sval.equals("Drizzle"))
        condM = 4;
      else if (sval.equals("Thunderstorm"))
        condM = 5;
      else if (sval.equals("Snow"))
        condM = 6;
      //
      condS = sval;
      Serial.print ("condM ");
      Serial.println (condM);
    }
    //tempM
    bT = line.indexOf ("\"temp\":");
    if (bT > 0)
    {
      bT2 = line.indexOf (",\"", bT + 7);
      sval = line.substring (bT + 7, bT2);
      Serial.print ("temp: ");
      Serial.println (sval);
      tempM = sval.toInt ();
    }
    else
      Serial.println ("temp NOT found!");
    //tempMin
    bT = line.indexOf ("\"temp_min\":");
    if (bT > 0)
    {
      bT2 = line.indexOf (",\"", bT + 11);
      sval = line.substring (bT + 11, bT2);
      Serial.print ("temp min: ");
      Serial.println (sval);
      tempMin = sval.toInt ();
    }
    else
      Serial.println ("temp_min NOT found!");
    //tempMax
    bT = line.indexOf ("\"temp_max\":");
    if (bT > 0)
    {
      bT2 = line.indexOf ("},", bT + 11);
      sval = line.substring (bT + 11, bT2);
      Serial.print ("temp max: ");
      Serial.println (sval);
      tempMax = sval.toInt ();
    }
    else
      Serial.println ("temp_max NOT found!");
    //pressM
    bT = line.indexOf ("\"pressure\":");
    if (bT > 0)
    {
      bT2 = line.indexOf (",\"", bT + 11);
      sval = line.substring (bT + 11, bT2);
      Serial.print ("press ");
      Serial.println (sval);
      presM = sval.toInt();
    }
    else
      Serial.println ("pressure NOT found!");
    //humiM
    bT = line.indexOf ("\"humidity\":");
    if (bT > 0)
    {
      bT2 = line.indexOf (",\"", bT + 11);
      sval = line.substring (bT + 11, bT2);
      Serial.print ("humi ");
      Serial.println (sval);
      humiM = sval.toInt();
    }
    else
      Serial.println ("humidity NOT found!");
  }//connected
}

#ifdef USE_ICONS
#include "TinyIcons.h"
//icons 10x5: 10 cols, 5 rows
int moony_ico [50] = {
  //3 nuances: 0x18c3 < 0x3186 < 0x4a49
  0x0000, 0x4a49, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x18c3,
  0x0000, 0x0000, 0x0000, 0x4a49, 0x3186, 0x3186, 0x18c3, 0x0000, 0x0000, 0x0000,
  0x0000, 0x0000, 0x4a49, 0x4a49, 0x3186, 0x3186, 0x3186, 0x18c3, 0x0000, 0x0000,
  0x0000, 0x0000, 0x4a49, 0x3186, 0x3186, 0x3186, 0x3186, 0x18c3, 0x0000, 0x0000,
};

#ifdef USE_WEATHER_ANI
int moony1_ico [50] = {
  //3 nuances: 0x18c3 < 0x3186 < 0x4a49
  0x0000, 0x18c3, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x4a49,
  0x0000, 0x0000, 0x0000, 0x4a49, 0x3186, 0x3186, 0x18c3, 0x0000, 0x0000, 0x0000,
  0x0000, 0x0000, 0x4a49, 0x4a49, 0x3186, 0x3186, 0x3186, 0x18c3, 0x0000, 0x0000,
  0x0000, 0x0000, 0x4a49, 0x3186, 0x3186, 0x3186, 0x3186, 0x18c3, 0x0000, 0x0000,
};

int moony2_ico [50] = {
  //3 nuances: 0x18c3 < 0x3186 < 0x4a49
  0x0000, 0x3186, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x3186,
  0x0000, 0x0000, 0x0000, 0x4a49, 0x3186, 0x3186, 0x18c3, 0x0000, 0x0000, 0x0000,
  0x0000, 0x0000, 0x4a49, 0x4a49, 0x3186, 0x3186, 0x3186, 0x18c3, 0x0000, 0x0000,
  0x0000, 0x0000, 0x4a49, 0x3186, 0x3186, 0x3186, 0x3186, 0x18c3, 0x0000, 0x0000,
};
#endif

int sunny_ico [50] = {
  0x0000, 0x0000, 0x0000, 0xffe0, 0x0000, 0x0000, 0xffe0, 0x0000, 0x0000, 0x0000,
  0x0000, 0xffe0, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xffe0, 0x0000,
  0x0000, 0x0000, 0x0000, 0xffe0, 0xffe0, 0xffe0, 0xffe0, 0x0000, 0x0000, 0x0000,
  0xffe0, 0x0000, 0xffe0, 0xffe0, 0xffe0, 0xffe0, 0xffe0, 0xffe0, 0x0000, 0xffe0,
  0x0000, 0x0000, 0xffe0, 0xffe0, 0xffe0, 0xffe0, 0xffe0, 0xffe0, 0x0000, 0x0000,
};

#ifdef USE_WEATHER_ANI
int sunny1_ico [50] = {
  0x0000, 0x0000, 0x0000, 0xffe0, 0x0000, 0x0000, 0xffff, 0x0000, 0x0000, 0x0000,
  0x0000, 0xffff, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xffe0, 0x0000,
  0x0000, 0x0000, 0x0000, 0xffe0, 0xffe0, 0xffe0, 0xffe0, 0x0000, 0x0000, 0x0000,
  0xffe0, 0x0000, 0xffe0, 0xffe0, 0xffe0, 0xffe0, 0xffe0, 0xffe0, 0x0000, 0xffff,
  0x0000, 0x0000, 0xffe0, 0xffe0, 0xffe0, 0xffe0, 0xffe0, 0xffe0, 0x0000, 0x0000,
};

int sunny2_ico [50] = {
  0x0000, 0x0000, 0x0000, 0xffff, 0x0000, 0x0000, 0xffe0, 0x0000, 0x0000, 0x0000,
  0x0000, 0xffe0, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xffff, 0x0000,
  0x0000, 0x0000, 0x0000, 0xffe0, 0xffe0, 0xffe0, 0xffe0, 0x0000, 0x0000, 0x0000,
  0xffff, 0x0000, 0xffe0, 0xffe0, 0xffe0, 0xffe0, 0xffe0, 0xffe0, 0x0000, 0xffe0,
  0x0000, 0x0000, 0xffe0, 0xffe0, 0xffe0, 0xffe0, 0xffe0, 0xffe0, 0x0000, 0x0000,
};
#endif

int cloudy_ico [50] = {
  0x0000, 0x0000, 0x0000, 0xffe0, 0x0000, 0x0000, 0xffe0, 0x0000, 0x0000, 0x0000,
  0x0000, 0xffe0, 0x0000, 0x0000, 0x0000, 0x0000, 0xc618, 0xc618, 0xffe0, 0x0000,
  0x0000, 0x0000, 0x0000, 0xffe0, 0xffe0, 0xc618, 0xc618, 0xc618, 0xc618, 0x0000,
  0xffe0, 0x0000, 0xffe0, 0xffe0, 0xffe0, 0xc618, 0xc618, 0xc618, 0xc618, 0xc618,
  0x0000, 0x0000, 0xffe0, 0xffe0, 0xffe0, 0xffe0, 0xc618, 0xc618, 0xc618, 0xc618,
};

#ifdef USE_WEATHER_ANI
int cloudy1_ico [50] = {
  0x0000, 0x0000, 0x0000, 0xffff, 0x0000, 0x0000, 0xffe0, 0x0000, 0x0000, 0x0000,
  0x0000, 0xffe0, 0x0000, 0x0000, 0x0000, 0x0000, 0xc618, 0xc618, 0xffff, 0x0000,
  0x0000, 0x0000, 0x0000, 0xffe0, 0xffe0, 0xc618, 0xc618, 0xc618, 0xc618, 0x0000,
  0xffff, 0x0000, 0xffe0, 0xffe0, 0xffe0, 0xc618, 0xc618, 0xc618, 0xc618, 0xc618,
  0x0000, 0x0000, 0xffe0, 0xffe0, 0xffe0, 0xffe0, 0xc618, 0xc618, 0xc618, 0xc618,
};

int cloudy2_ico [50] = {
  0x0000, 0x0000, 0x0000, 0xffe0, 0x0000, 0x0000, 0xffff, 0x0000, 0x0000, 0x0000,
  0x0000, 0xffff, 0x0000, 0x0000, 0x0000, 0x0000, 0xc618, 0xc618, 0xffe0, 0x0000,
  0x0000, 0x0000, 0x0000, 0xffe0, 0xffe0, 0xc618, 0xc618, 0xc618, 0xc618, 0x0000,
  0xffe0, 0x0000, 0xffe0, 0xffe0, 0xffe0, 0xc618, 0xc618, 0xc618, 0xc618, 0xc618,
  0x0000, 0x0000, 0xffe0, 0xffe0, 0xffe0, 0xffe0, 0xc618, 0xc618, 0xc618, 0xc618,
};
#endif

int ovrcst_ico [50] = {
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xc618, 0xc618, 0x0000, 0x0000,
  0x0000, 0x0000, 0xc618, 0xc618, 0x0000, 0xc618, 0xc618, 0xc618, 0xc618, 0x0000,
  0x0000, 0xc618, 0xffff, 0xffff, 0xc618, 0xffff, 0xffff, 0xffff, 0xc618, 0x0000,
  0x0000, 0xc618, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xc618, 0x0000,
  0x0000, 0x0000, 0xc618, 0xc618, 0xc618, 0xc618, 0xc618, 0xc618, 0x0000, 0x0000,
};

#ifdef USE_WEATHER_ANI
int ovrcst1_ico [50] = {
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xc618, 0xc618, 0x0000, 0x0000,
  0x0000, 0x0000, 0xc618, 0xc618, 0x0000, 0xc618, 0xc618, 0xc618, 0xc618, 0x0000,
  0x0000, 0xc618, 0xc618, 0xc618, 0xc618, 0xffff, 0xffff, 0xffff, 0xc618, 0x0000,
  0x0000, 0xc618, 0xc618, 0xc618, 0xffff, 0xffff, 0xffff, 0xffff, 0xc618, 0x0000,
  0x0000, 0x0000, 0xc618, 0xc618, 0xc618, 0xc618, 0xc618, 0xc618, 0x0000, 0x0000,
};

int ovrcst2_ico [50] = {
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xc618, 0xc618, 0x0000, 0x0000,
  0x0000, 0x0000, 0xc618, 0xc618, 0x0000, 0xc618, 0xc618, 0xc618, 0xc618, 0x0000,
  0x0000, 0xc618, 0xffff, 0xffff, 0xc618, 0xc618, 0xc618, 0xc618, 0xc618, 0x0000,
  0x0000, 0xc618, 0xffff, 0xffff, 0xffff, 0xc618, 0xc618, 0xc618, 0xc618, 0x0000,
  0x0000, 0x0000, 0xc618, 0xc618, 0xc618, 0xc618, 0xc618, 0xc618, 0x0000, 0x0000,
};
#endif

int thndr_ico [50] = {
  0x041f, 0xc618, 0x041f, 0xc618, 0xc618, 0xc618, 0x041f, 0xc618, 0xc618, 0x041f,
  0xc618, 0xc618, 0xc618, 0xc618, 0x041f, 0xc618, 0xc618, 0x041f, 0xc618, 0xc618,
  0xc618, 0x041f, 0xc618, 0xc618, 0xc618, 0x041f, 0xc618, 0xc618, 0xc618, 0xc618,
  0xc618, 0xc618, 0xc618, 0x041f, 0xc618, 0xc618, 0xc618, 0xc618, 0xc618, 0x041f,
  0xc618, 0x041f, 0xc618, 0xc618, 0xc618, 0xc618, 0x041f, 0xc618, 0x041f, 0xc618,
};

int rain_ico [50] = {
  0x041f, 0x0000, 0x041f, 0x0000, 0x0000, 0x0000, 0x041f, 0x0000, 0x0000, 0x041f,
  0x0000, 0x0000, 0x0000, 0x0000, 0x041f, 0x0000, 0x0000, 0x041f, 0x0000, 0x0000,
  0x0000, 0x041f, 0x0000, 0x0000, 0x0000, 0x041f, 0x0000, 0x0000, 0x0000, 0x0000,
  0x0000, 0x0000, 0x0000, 0x041f, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x041f,
  0x0000, 0x041f, 0x0000, 0x0000, 0x0000, 0x0000, 0x041f, 0x0000, 0x041f, 0x0000,
};

#ifdef USE_WEATHER_ANI
int rain1_ico [50] = {
  0x0000, 0x041f, 0x0000, 0x0000, 0x0000, 0x0000, 0x041f, 0x0000, 0x041f, 0x0000,
  0x041f, 0x0000, 0x041f, 0x0000, 0x0000, 0x0000, 0x041f, 0x0000, 0x0000, 0x041f,
  0x0000, 0x0000, 0x0000, 0x0000, 0x041f, 0x0000, 0x0000, 0x041f, 0x0000, 0x0000,
  0x0000, 0x041f, 0x0000, 0x0000, 0x0000, 0x041f, 0x0000, 0x0000, 0x0000, 0x0000,
  0x0000, 0x0000, 0x0000, 0x041f, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x041f,
};

int rain2_ico [50] = {
  0x0000, 0x0000, 0x0000, 0x041f, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x041f,
  0x0000, 0x041f, 0x0000, 0x0000, 0x0000, 0x0000, 0x041f, 0x0000, 0x041f, 0x0000,
  0x041f, 0x0000, 0x041f, 0x0000, 0x0000, 0x0000, 0x041f, 0x0000, 0x0000, 0x041f,
  0x0000, 0x0000, 0x0000, 0x0000, 0x041f, 0x0000, 0x0000, 0x041f, 0x0000, 0x0000,
  0x0000, 0x041f, 0x0000, 0x0000, 0x0000, 0x041f, 0x0000, 0x0000, 0x0000, 0x0000,
};

int rain3_ico [50] = {
  0x0000, 0x041f, 0x0000, 0x0000, 0x0000, 0x041f, 0x0000, 0x0000, 0x0000, 0x0000,
  0x0000, 0x0000, 0x0000, 0x041f, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x041f,
  0x0000, 0x041f, 0x0000, 0x0000, 0x0000, 0x0000, 0x041f, 0x0000, 0x041f, 0x0000,
  0x041f, 0x0000, 0x041f, 0x0000, 0x0000, 0x0000, 0x041f, 0x0000, 0x0000, 0x041f,
  0x0000, 0x0000, 0x0000, 0x0000, 0x041f, 0x0000, 0x0000, 0x041f, 0x0000, 0x0000,
};

int rain4_ico [50] = {
  0x0000, 0x0000, 0x0000, 0x0000, 0x041f, 0x0000, 0x0000, 0x041f, 0x0000, 0x0000,
  0x0000, 0x041f, 0x0000, 0x0000, 0x0000, 0x041f, 0x0000, 0x0000, 0x0000, 0x0000,
  0x0000, 0x0000, 0x0000, 0x041f, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x041f,
  0x0000, 0x041f, 0x0000, 0x0000, 0x0000, 0x0000, 0x041f, 0x0000, 0x041f, 0x0000,
  0x041f, 0x0000, 0x041f, 0x0000, 0x0000, 0x0000, 0x041f, 0x0000, 0x0000, 0x041f,
};
#endif

int snow_ico [50] = {
  0xc618, 0x0000, 0xc618, 0x0000, 0x0000, 0x0000, 0xc618, 0x0000, 0x0000, 0xc618,
  0x0000, 0x0000, 0x0000, 0x0000, 0xc618, 0x0000, 0x0000, 0xc618, 0x0000, 0x0000,
  0x0000, 0xc618, 0x0000, 0x0000, 0x0000, 0xc618, 0x0000, 0x0000, 0x0000, 0x0000,
  0x0000, 0x0000, 0x0000, 0xc618, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xc618,
  0x0000, 0xc618, 0x0000, 0x0000, 0x0000, 0x0000, 0xc618, 0x0000, 0xc618, 0x0000,
};

#ifdef USE_WEATHER_ANI
int snow1_ico [50] = {
  0x0000, 0xc618, 0x0000, 0x0000, 0x0000, 0x0000, 0xc618, 0x0000, 0xc618, 0x0000,
  0xc618, 0x0000, 0xc618, 0x0000, 0x0000, 0x0000, 0xc618, 0x0000, 0x0000, 0xc618,
  0x0000, 0x0000, 0x0000, 0x0000, 0xc618, 0x0000, 0x0000, 0xc618, 0x0000, 0x0000,
  0x0000, 0xc618, 0x0000, 0x0000, 0x0000, 0xc618, 0x0000, 0x0000, 0x0000, 0x0000,
  0x0000, 0x0000, 0x0000, 0xc618, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xc618,
};

int snow2_ico [50] = {
  0x0000, 0x0000, 0x0000, 0xc618, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xc618,
  0x0000, 0xc618, 0x0000, 0x0000, 0x0000, 0x0000, 0xc618, 0x0000, 0xc618, 0x0000,
  0xc618, 0x0000, 0xc618, 0x0000, 0x0000, 0x0000, 0xc618, 0x0000, 0x0000, 0xc618,
  0x0000, 0x0000, 0x0000, 0x0000, 0xc618, 0x0000, 0x0000, 0xc618, 0x0000, 0x0000,
  0x0000, 0xc618, 0x0000, 0x0000, 0x0000, 0xc618, 0x0000, 0x0000, 0x0000, 0x0000,
};

int snow3_ico [50] = {
  0x0000, 0xc618, 0x0000, 0x0000, 0x0000, 0xc618, 0x0000, 0x0000, 0x0000, 0x0000,
  0x0000, 0x0000, 0x0000, 0xc618, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xc618,
  0x0000, 0xc618, 0x0000, 0x0000, 0x0000, 0x0000, 0xc618, 0x0000, 0xc618, 0x0000,
  0xc618, 0x0000, 0xc618, 0x0000, 0x0000, 0x0000, 0xc618, 0x0000, 0x0000, 0xc618,
  0x0000, 0x0000, 0x0000, 0x0000, 0xc618, 0x0000, 0x0000, 0xc618, 0x0000, 0x0000,
};

int snow4_ico [50] = {
  0x0000, 0x0000, 0x0000, 0x0000, 0xc618, 0x0000, 0x0000, 0xc618, 0x0000, 0x0000,
  0x0000, 0xc618, 0x0000, 0x0000, 0x0000, 0xc618, 0x0000, 0x0000, 0x0000, 0x0000,
  0x0000, 0x0000, 0x0000, 0xc618, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xc618,
  0x0000, 0xc618, 0x0000, 0x0000, 0x0000, 0x0000, 0xc618, 0x0000, 0xc618, 0x0000,
  0xc618, 0x0000, 0xc618, 0x0000, 0x0000, 0x0000, 0xc618, 0x0000, 0x0000, 0xc618,
};
#endif

#ifdef USE_WEATHER_ANI
int *suny_ani[] = {sunny_ico, sunny1_ico, sunny2_ico, sunny1_ico, sunny2_ico};
int *clod_ani[] = {cloudy_ico, cloudy1_ico, cloudy2_ico, cloudy1_ico, cloudy2_ico};
int *ovct_ani[] = {ovrcst_ico, ovrcst1_ico, ovrcst2_ico, ovrcst1_ico, ovrcst2_ico};
int *rain_ani[] = {rain_ico, rain1_ico, rain2_ico, rain3_ico, rain4_ico};
int *thun_ani[] = {thndr_ico, rain1_ico, rain2_ico, rain3_ico, rain4_ico};
int *snow_ani[] = {snow_ico, snow1_ico, snow2_ico, snow3_ico, snow4_ico};
int *mony_ani[] = {moony_ico, moony1_ico, moony2_ico, moony1_ico, moony2_ico};
#else
int *suny_ani[] = {sunny_ico, sunny_ico, sunny_ico, sunny_ico, sunny_ico};
int *clod_ani[] = {cloudy_ico, cloudy_ico, cloudy_ico, cloudy_ico, cloudy_ico};
int *ovct_ani[] = {ovrcst_ico, ovrcst_ico, ovrcst_ico, ovrcst_ico, ovrcst_ico};
int *rain_ani[] = {rain_ico, rain_ico, rain_ico, rain_ico, rain_ico};
int *thun_ani[] = {thndr_ico, rain_ico, rain_ico, rain_ico, rain_ico};
int *snow_ani[] = {snow_ico, snow_ico, snow_ico, snow_ico, snow_ico};
int *mony_ani[] = {moony_ico, moony_ico, moony_ico, moony_ico, moony_ico};
#endif
/*
 * 
int ovrcst_ico [50] = {
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xc618, 0x0000, 0x0000, 0x0000, 0x0000,
  0x0000, 0x0000, 0x0000, 0xc618, 0xc618, 0xffff, 0xc618, 0x0000, 0x0000, 0x0000,
  0x0000, 0xc618, 0xffff, 0x0000, 0xffff, 0xffff, 0xffff, 0xffff, 0xc618, 0x0000,
  0xc618, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xc618,
  0x0000, 0xc618, 0xc618, 0xc618, 0xc618, 0xc618, 0xc618, 0xc618, 0xc618, 0x0000,
};
int ovrcst_ico [50] = {
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xc618, 0xc618, 0x0000, 0x0000,
  0x0000, 0x0000, 0xc618, 0xc618, 0x0000, 0xc618, 0xc618, 0xc618, 0xc618, 0x0000,
  0x0000, 0xc618, 0xffff, 0xffff, 0xc618, 0xffff, 0xffff, 0xffff, 0xc618, 0x0000,
  0x0000, 0xc618, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xc618, 0x0000,
  0x0000, 0x0000, 0xc618, 0xc618, 0xc618, 0xc618, 0xc618, 0xc618, 0x0000, 0x0000,
};
int cloudy_ico [50] = {
  0x0000, 0x0000, 0x0000, 0xffe0, 0x0000, 0x0000, 0xffe0, 0xc618, 0x0000, 0x0000,
  0x0000, 0xffe0, 0xc618, 0xc618, 0x0000, 0xc618, 0xc618, 0xc618, 0xffe0, 0x0000,
  0x0000, 0xc618, 0xc618, 0xc618, 0xffe0, 0xc618, 0xc618, 0xc618, 0xc618, 0x0000,
  0x0000, 0xc618, 0xc618, 0xc618, 0xc618, 0xc618, 0xc618, 0xc618, 0xc618, 0xffe0,
  0x0000, 0x0000, 0xc618, 0xc618, 0xc618, 0xc618, 0xc618, 0xc618, 0x0000, 0x0000,
};
 */
#endif

int xo = 1, yo = 26;
char use_ani = 0;
char daytime = 1;
void draw_weather_conditions ()
{
  //0 - unk, 1 - sunny, 2 - cloudy, 3 - overcast, 4 - rainy, 5 - thunders, 6 - snow
  Serial.print ("weather conditions ");
  Serial.println (condM);
  //cleanup previous cond
  xo = 3*TF_COLS; yo = 1;
#ifdef USE_ICONS
  if (condM == 0 && daytime)
  {
    Serial.print ("!weather condition icon unknown, show: ");
    Serial.println (condS);
    int cc_dgr = display.color565 (30, 30, 30);
    //draw the first 5 letters from the unknown weather condition
    String lstr = condS.substring (0, (condS.length () > 5?5:condS.length ()));
    lstr.toUpperCase ();
    TFDrawText (&display, lstr, xo, yo, cc_dgr);
  }
  else
  {
    TFDrawText (&display, String("     "), xo, yo, 0);
  }
  //
  xo = 4*TF_COLS; yo = 1;
  switch (condM)
  {
    case 0://unk
      break;
    case 1://sunny
      if (!daytime)
        DrawIcon (&display, moony_ico, xo, yo, 10, 5);
      else
        DrawIcon (&display, sunny_ico, xo, yo, 10, 5);
      //DrawIcon (&display, cloudy_ico, xo, yo, 10, 5);
      //DrawIcon (&display, ovrcst_ico, xo, yo, 10, 5);
      //DrawIcon (&display, rain_ico, xo, yo, 10, 5);
      use_ani = 1;
      break;
    case 2://cloudy
      DrawIcon (&display, cloudy_ico, xo, yo, 10, 5);
      use_ani = 1;
      break;
    case 3://overcast
      DrawIcon (&display, ovrcst_ico, xo, yo, 10, 5);
      use_ani = 1;
      break;
    case 4://rainy
      DrawIcon (&display, rain_ico, xo, yo, 10, 5);
      use_ani = 1;
      break;
    case 5://thunders
      DrawIcon (&display, thndr_ico, xo, yo, 10, 5);
      use_ani = 1;
      break;
    case 6://snow
      DrawIcon (&display, snow_ico, xo, yo, 10, 5);
      use_ani = 1;
      break;
  }
#else
  xo = 3*TF_COLS; yo = 1;
  Serial.print ("!weather condition icon unknown, show: ");
  Serial.println (condS);
  int cc_dgr = display.color565 (30, 30, 30);
  //draw the first 5 letters from the unknown weather condition
  String lstr = condS.substring (0, (condS.length () > 5?5:condS.length ()));
  lstr.toUpperCase ();
  TFDrawText (&display, lstr, xo, yo, cc_dgr);
#endif
}

void draw_weather ()
{
  int cc_wht = display.color565 (cin, cin, cin);
  int cc_red = display.color565 (cin, 0, 0);
  int cc_grn = display.color565 (0, cin, 0);
  int cc_blu = display.color565 (0, 0, cin);
  //int cc_ylw = display.color565 (cin, cin, 0);
  //int cc_gry = display.color565 (128, 128, 128);
  int cc_dgr = display.color565 (30, 30, 30);
  Serial.println ("showing the weather");
  xo = 0; yo = 1;
  TFDrawText (&display, String("                   "), xo, yo, cc_dgr);
  if (tempM == -10000 || humiM == -10000 || presM == -10000)
  {
    //TFDrawText (&display, String("NO WEATHER DATA"), xo, yo, cc_dgr);
    Serial.println ("!no weather data available");
  }
  else
  {
    //weather below the clock
    //-temperature
    int lcc = cc_red;
    if (*u_metric == 'Y')
    {
      //C
      if (tempM < 26)
        lcc = cc_grn;
      if (tempM < 18)
        lcc = cc_blu;
      if (tempM < 6)
        lcc = cc_wht;
    }
    else
    {
      //F
      if (tempM < 79)
        lcc = cc_grn;
      if (tempM < 64)
        lcc = cc_blu;
      if (tempM < 43)
        lcc = cc_wht;
    }
    //
    String lstr = String (tempM) + String((*u_metric=='Y')?"C":"F");
    Serial.print ("temperature: ");
    Serial.println (lstr);
    TFDrawText (&display, lstr, xo, yo, lcc);
    //weather conditions
    //-humidity
    lcc = cc_red;
    if (humiM < 65)
      lcc = cc_grn;
    if (humiM < 35)
      lcc = cc_blu;
    if (humiM < 15)
      lcc = cc_wht;
    lstr = String (humiM) + "%";
    xo = 8*TF_COLS;
    TFDrawText (&display, lstr, xo, yo, lcc);
    //-pressure
    lstr = String (presM);
    xo = 12*TF_COLS;
    if (presM < 1000)
        xo = 13*TF_COLS;
    TFDrawText (&display, lstr, xo, yo, cc_blu);
    //draw temp min/max
    if (tempMin > -10000)
    {
      xo = 0*TF_COLS; yo = 26;
      TFDrawText (&display, "   ", xo, yo, 0);
      lstr = String (tempMin);// + String((*u_metric=='Y')?"C":"F");
      //blue if negative
      int ct = cc_blu;
      if (tempMin < 0)
      {
        ct = cc_dgr;
        lstr = String (-tempMin);// + String((*u_metric=='Y')?"C":"F");
      }
      Serial.print ("temp min: ");
      Serial.println (lstr);
      TFDrawText (&display, lstr, xo, yo, ct);
    }
    if (tempMax > -10000)
    {
      TFDrawText (&display, "   ", 13*TF_COLS, yo, 0);
      //move the text to the right or left as needed
      xo = 14*TF_COLS; yo = 26;
      if (tempMax < 10)
        xo = 15*TF_COLS;
      if (tempMax > 99)
        xo = 13*TF_COLS;
      lstr = String (tempMax);// + String((*u_metric=='Y')?"C":"F");
      //blue if negative
      int ct = cc_blu;
      if (tempMax < 0)
      {
        ct = cc_dgr;
        lstr = String (-tempMax);// + String((*u_metric=='Y')?"C":"F");
      }
      Serial.print ("temp max: ");
      Serial.println (lstr);
      TFDrawText (&display, lstr, xo, yo, ct);
    }
    //weather conditions
    draw_weather_conditions ();
  }
}

void draw_love ()
{
  Serial.println ("showing some love");
  use_ani = 0;
  //love*you,boo
  yo = 1;
  int cc = random (255, 65535);
  xo  = 0; TFDrawChar (&display, 'L', xo, yo, cc); cc = random (255, 65535);
  xo += 4; TFDrawChar (&display, 'O', xo, yo, cc); cc = random (255, 65535);
  xo += 4; TFDrawChar (&display, 'V', xo, yo, cc); cc = random (255, 65535);
  xo += 4; TFDrawChar (&display, 'E', xo, yo, cc); cc = random (255, 65535);
  xo += 4; TFDrawChar (&display, ' ', xo, yo, cc); cc = random (255, 65535);
  xo += 4; TFDrawChar (&display, 'h', xo, yo, display.color565 (255, 0, 0));
  xo += 4; TFDrawChar (&display, 'i', xo, yo, display.color565 (255, 0, 0));
  xo += 4; TFDrawChar (&display, ' ', xo, yo, cc); cc = random (255, 65535);
  xo += 4; TFDrawChar (&display, 'Y', xo, yo, cc); cc = random (255, 65535);
  xo += 4; TFDrawChar (&display, 'O', xo, yo, cc); cc = random (255, 65535);
  xo += 4; TFDrawChar (&display, 'U', xo, yo, cc); cc = random (255, 65535);
  xo += 4; TFDrawChar (&display, ',', xo, yo, cc); cc = random (255, 65535);
  xo += 4; TFDrawChar (&display, ' ', xo, yo, cc); cc = random (255, 65535);
  xo += 4; TFDrawChar (&display, 'B', xo, yo, cc); cc = random (255, 65535);
  xo += 4; TFDrawChar (&display, 'O', xo, yo, cc); cc = random (255, 65535);
  xo += 4; TFDrawChar (&display, 'O', xo, yo, cc); cc = random (255, 65535);
}
//
void draw_date ()
{
  int cc_grn = display.color565 (0, cin, 0);
  Serial.println ("showing the date");
  //for (int i = 0 ; i < 12; i++)
    //TFDrawChar (&display, '0' + i%10, xo + i * 5, yo, display.color565 (0, 255, 0));
  //date below the clock
  long tnow = now ();
  String lstr = "";
  for (int i = 0; i < 5; i += 2)
  {
    switch (date_fmt[i])
    {
      case 'D':
        lstr += (day(tnow) < 10 ? "0" + String(day(tnow)) : String(day(tnow)));
        if (i < 4)
          lstr += date_fmt[i + 1];
        break;
      case 'M':
        lstr += (month(tnow) < 10 ? "0" + String(month(tnow)) : String(month(tnow)));
        if (i < 4)
          lstr += date_fmt[i + 1];
        break;
      case 'Y':
        lstr += String(year(tnow));
        if (i < 4)
          lstr += date_fmt[i + 1];
        break;
    }
  }
  //
  if (lstr.length ())
  {
    //
    xo = 3*TF_COLS; yo = 26;
    TFDrawText (&display, lstr, xo, yo, cc_grn);
  }
}

void draw_animations (int stp)
{
#ifdef USE_ICONS
//weather icon animation
  int xo = 4*TF_COLS; 
  int yo = 1;
  //0 - unk, 1 - sunny, 2 - cloudy, 3 - overcast, 4 - rainy, 5 - thunders, 6 - snow
  if (use_ani)
  {
    int *af = NULL;
    //weather/night icon
    if (!daytime)
      af = mony_ani[stp%5];
    else
    {
      switch (condM)
      {
        case 1:
            af = suny_ani[stp%5];
          break;
        case 2:
            af = clod_ani[stp%5];
          break;
        case 3:
            af = ovct_ani[stp%5];
          break;
        case 4:
            af = rain_ani[stp%5];
          break;
        case 5:
            af = thun_ani[stp%5];
          break;
        case 6:
            af = snow_ani[stp%5];
          break;
      }
    }
    //draw animation
    if (af)
      DrawIcon (&display, af, xo, yo, 10, 5);
  }
#endif
}


#ifdef USE_FIREWORKS
//fireworks
// adapted to Arduino pxMatrix
// from https://r3dux.org/2010/10/how-to-create-a-simple-fireworks-effect-in-opengl-and-sdl/
// Define our initial screen width, height, and colour depth
int SCREEN_WIDTH  = 64;
int SCREEN_HEIGHT = 32;

const int FIREWORKS = 6;           // Number of fireworks
const int FIREWORK_PARTICLES = 8;  // Number of particles per firework

class Firework
{
  public:
    float x[FIREWORK_PARTICLES];
    float y[FIREWORK_PARTICLES];
    char lx[FIREWORK_PARTICLES], ly[FIREWORK_PARTICLES];
    float xSpeed[FIREWORK_PARTICLES];
    float ySpeed[FIREWORK_PARTICLES];

    char red;
    char blue;
    char green;
    char alpha;

    int framesUntilLaunch;

    char particleSize;
    boolean hasExploded;

    Firework(); // Constructor declaration
    void initialise();
    void move();
    void explode();
};

const float GRAVITY = 0.05f;
const float baselineSpeed = -1.0f;
const float maxSpeed = -2.0f;

// Constructor implementation
Firework::Firework()
{
  initialise();
  for (int loop = 0; loop < FIREWORK_PARTICLES; loop++)
  {
    lx[loop] = 0;
    ly[loop] = SCREEN_HEIGHT + 1; // Push the particle location down off the bottom of the screen
  }
}

void Firework::initialise()
{
    // Pick an initial x location and  random x/y speeds
    float xLoc = (rand() % SCREEN_WIDTH);
    float xSpeedVal = baselineSpeed + (rand() % (int)maxSpeed);
    float ySpeedVal = baselineSpeed + (rand() % (int)maxSpeed);

    // Set initial x/y location and speeds
    for (int loop = 0; loop < FIREWORK_PARTICLES; loop++)
    {
        x[loop] = xLoc;
        y[loop] = SCREEN_HEIGHT + 1; // Push the particle location down off the bottom of the screen
        xSpeed[loop] = xSpeedVal;
        ySpeed[loop] = ySpeedVal;
        //don't reset these otherwise particles won't be removed
        //lx[loop] = 0;
        //ly[loop] = SCREEN_HEIGHT + 1; // Push the particle location down off the bottom of the screen
    }

    // Assign a random colour and full alpha (i.e. particle is completely opaque)
    red   = (rand() % 255);/// (float)RAND_MAX);
    green = (rand() % 255); /// (float)RAND_MAX);
    blue  = (rand() % 255); /// (float)RAND_MAX);
    alpha = 50;//max particle frames

    // Firework will launch after a random amount of frames between 0 and 400
    framesUntilLaunch = ((int)rand() % (SCREEN_HEIGHT/2));

    // Size of the particle (as thrown to glPointSize) - range is 1.0f to 4.0f
    particleSize = 1.0f + ((float)rand() / (float)RAND_MAX) * 3.0f;

    // Flag to keep trackof whether the firework has exploded or not
    hasExploded = false;

    //cout << "Initialised a firework." << endl;
}

void Firework::move()
{
    for (int loop = 0; loop < FIREWORK_PARTICLES; loop++)
    {
        // Once the firework is ready to launch start moving the particles
        if (framesUntilLaunch <= 0)
        {
            //draw black on last known position
            //display.drawPixel (x[loop], y[loop], cc_blk);
            lx[loop] = x[loop];
            ly[loop] = y[loop];
            //
            x[loop] += xSpeed[loop];

            y[loop] += ySpeed[loop];

            ySpeed[loop] += GRAVITY;
        }
    }
    framesUntilLaunch--;

    // Once a fireworks speed turns positive (i.e. at top of arc) - blow it up!
    if (ySpeed[0] > 0.0f)
    {
        for (int loop2 = 0; loop2 < FIREWORK_PARTICLES; loop2++)
        {
            // Set a random x and y speed beteen -4 and + 4
            xSpeed[loop2] = -4 + (rand() / (float)RAND_MAX) * 8;
            ySpeed[loop2] = -4 + (rand() / (float)RAND_MAX) * 8;
        }

        //cout << "Boom!" << endl;
        hasExploded = true;
    }
}

void Firework::explode()
{
    for (int loop = 0; loop < FIREWORK_PARTICLES; loop++)
    {
        // Dampen the horizontal speed by 1% per frame
        xSpeed[loop] *= 0.99;

        //draw black on last known position
        //display.drawPixel (x[loop], y[loop], cc_blk);
        lx[loop] = x[loop];
        ly[loop] = y[loop];
        //
        // Move the particle
        x[loop] += xSpeed[loop];
        y[loop] += ySpeed[loop];

        // Apply gravity to the particle's speed
        ySpeed[loop] += GRAVITY;
    }

    // Fade out the particles (alpha is stored per firework, not per particle)
    if (alpha > 0)
    {
        alpha -= 1;
    }
    else // Once the alpha hits zero reset the firework
    {
        initialise();
    }
}

// Create our array of fireworks
Firework fw[FIREWORKS];

void fireworks_loop (int frm)
{
  int cc_frw;
  //display.fillScreen (0);
  // Draw fireworks
  //cout << "Firework count is: " << Firework::fireworkCount << endl;
  for (int loop = 0; loop < FIREWORKS; loop++)
  {
      for (int particleLoop = 0; particleLoop < FIREWORK_PARTICLES; particleLoop++)
      {
  
          // Set colour to yellow on way up, then whatever colour firework should be when exploded
          if (fw[loop].hasExploded == false)
          {
              //glColor4f(1.0f, 1.0f, 0.0f, 1.0f);
              cc_frw = display.color565 (255, 255, 0);
          }
          else
          {
              //glColor4f(fw[loop].red, fw[loop].green, fw[loop].blue, fw[loop].alpha);
              //glVertex2f(fw[loop].x[particleLoop], fw[loop].y[particleLoop]);
              cc_frw = display.color565 (fw[loop].red, fw[loop].green, fw[loop].blue);
          }

          // Draw the point
          //glVertex2f(fw[loop].x[particleLoop], fw[loop].y[particleLoop]);
          display.drawPixel (fw[loop].x[particleLoop], fw[loop].y[particleLoop], cc_frw);
          display.drawPixel (fw[loop].lx[particleLoop], fw[loop].ly[particleLoop], 0);
      }
      // Move the firework appropriately depending on its explosion state
      if (fw[loop].hasExploded == false)
      {
          fw[loop].move();
      }
      else
      {
          fw[loop].explode();
      }
      //
      //delay (10);
  }
}
//-
#endif //define USE_FIREWORKS

byte prevhh = 0;
byte prevmm = 0;
byte prevss = 0;
long tnow;
#define FIREWORKS_DISPLAY 10//sec
#define FIREWORKS_LOOP    50//ms
WiFiClient httpcli;

//handle web server requests
void web_server ()
{
  httpcli = httpsvr.available ();
  if (httpcli) 
  {
    char svf = 0;
    //Read what the browser has sent into a String class and print the request to the monitor
    String httprq = httpcli.readString ();
    //Looking under the hood
    Serial.println (httprq);
    int pidx = -1;
    //
    String httprsp = "HTTP/1.1 200 OK\r\n";
    httprsp += "Content-type: text/html\r\n\r\n";
    httprsp += "<!DOCTYPE HTML>\r\n<html>\r\n";
    //check requests
    if ((pidx = httprq.indexOf ("GET /datetime/")) != -1)
    {
      int pidx2 = httprq.indexOf (" ", pidx + 14);
      if (pidx2 != -1)
      {
        String datetime = httprq.substring (pidx + 14, pidx2);
        //display.setBrightness (bri.toInt ());
        int yy = datetime.substring(0, 4).toInt();
        int MM = datetime.substring(4, 6).toInt();
        int dd = datetime.substring(6, 8).toInt();
        int hh = datetime.substring(8, 10).toInt();
        int mm = datetime.substring(10, 12).toInt();
        int ss = 0;
        if (datetime.length() == 14)
        {
          ss = datetime.substring(12, 14).toInt();
        }
        //void setTime(int hr,int min,int sec,int dy, int mnth, int yr)
        setTime(hh, mm, ss, dd, MM, yy);
        ntpsync = 1;
        Serial.print (">date and time: ");
        Serial.print (datetime);
        Serial.print (" vs ");
        Serial.print (yy);
        Serial.print (".");
        Serial.print (MM);
        Serial.print (".");
        Serial.print (dd);
        Serial.print (" ");
        Serial.print (hh);
        Serial.print (":");
        Serial.print (mm);
        Serial.print (":");
        Serial.print (ss);
        Serial.println ("");
      }
    }
    else if (httprq.indexOf ("GET /daylight/on ") != -1)
    {
      strcpy (c_vars[EV_DST], "true");
      NTP.begin (ntpsvr, String (c_vars[EV_TZ]).toInt (), toBool(String (c_vars[EV_DST])));
      httprsp += "<strong>daylight: on</strong><br>";
      Serial.println ("daylight ON");
      svf = 1;
    }
    else if (httprq.indexOf ("GET /daylight/off ") != -1)
    {
      strcpy (c_vars[EV_DST], "false");
      NTP.begin (ntpsvr, String (c_vars[EV_TZ]).toInt (), toBool(String (c_vars[EV_DST])));
      httprsp += "<strong>daylight: off</strong><br>";
      Serial.println ("daylight OFF");
      svf = 1;
    }
    else if ((pidx = httprq.indexOf ("GET /brightness/")) != -1)
    {
      int pidx2 = httprq.indexOf (" ", pidx + 16);
      if (pidx2 != -1)
      {
        String bri = httprq.substring (pidx + 16, pidx2);
        display.setBrightness (bri.toInt ());
        Serial.print (">brightness: ");
        Serial.println (bri);
      }
    }
    else if ((pidx = httprq.indexOf ("GET /timezone/")) != -1)
    {
      int pidx2 = httprq.indexOf (" ", pidx + 14);
      if (pidx2 != -1)
      {
        String tz = httprq.substring (pidx + 14, pidx2);
        //strcpy (timezone, tz.c_str ());
        strcpy (c_vars[EV_TZ], tz.c_str ());
        NTP.begin (ntpsvr, String (c_vars[EV_TZ]).toInt (), toBool(String (c_vars[EV_DST])));
        httprsp += "<strong>timezone:" + tz + "</strong><br>";
        Serial.print ("timezone: ");
        Serial.println (c_vars[EV_TZ]);
        svf = 1;
      }
      else
      {
        httprsp += "<strong>!invalid timezone!</strong><br>";
        Serial.print ("invalid timezone");
      }
    }
    //
    httprsp += "<br><br>use the following configuration links<br>";
    httprsp += "<a href='/daylight/on'>daylight on</a><br>";
    httprsp += "<a href='/daylight/off'>daylight off</a><br>";
    httprsp += "<a href='/timezone/0'>timezone 0</a><br>";
    httprsp += "<a href='/timezone/1'>timezone 1</a><br>";
    httprsp += "<a href='/timezone/2'>timezone 2</a><br>";
    httprsp += "use /timezone/x for timezone 'x'<br>";
    httprsp += "<a href='/brightness/10'>brightness 10</a><br>";
    httprsp += "<a href='/brightness/50'>brightness 50</a><br>";
    httprsp += "<a href='/brightness/100'>brightness 100</a><br>";
    httprsp += "<a href='/brightness/200'>brightness 200</a><br>";
    httprsp += "use /brightness/x for display brightness 'x' from 0 (darkest) to 255 (brightest)<br>";
    httprsp += "<br><br>";
    httprsp += "current configuration<br>";
    httprsp += "daylight: " + String (c_vars[EV_DST]) + "<br>";
    httprsp += "timezone: " + String (c_vars[EV_TZ]) + "<br>";
    httprsp += "<br><a href='/'>home</a><br>";
    httprsp += "<br>" \
      "<script language='javascript'>" \
      "var today = new Date();" \
      "var hh = today.getHours();" \
      "var mm = today.getMinutes();" \
      "if(hh<10)hh='0'+hh;" \
      "if(mm<59)mm=1+mm;" \
      "if(mm<10)mm='0'+mm;" \
      "var dd = today.getDate();" \
      "var MM = today.getMonth()+1;" \
      "if(dd<10)dd='0'+dd;" \
      "if(MM<10)MM='0'+MM;" \
      "var yyyy = today.getFullYear();" \
      "document.write('set date and time to <a href=/datetime/'+yyyy+MM+dd+hh+mm+'>'+yyyy+'.'+MM+'.'+dd+' '+hh+':'+mm+':00 (next minute)</a><br>');" \
      "document.write('using current date and time '+today);" \
      "</script>";
    httprsp += "</html>\r\n";
    httpcli.flush (); //clear previous info in the stream
    httpcli.print (httprsp); // Send the response to the client
    delay (1);
    //save settings?
    if (svf)
    {
      if (vars_write () > 0)
        Serial.println ("variables stored");
      else
        Serial.println ("variables storing failed");
    }
    Serial.println ("Client disonnected");
  }
}

void loop ()
{
	static int i = 0;
	static int last = 0;
  static int cm;
  //handle web server requests
  web_server ();
  //time changes every miliseconds, we only want to draw when digits actually change
  tnow = now ();
  //
  hh = hour (tnow);   //NTP.getHour ();
  mm = minute (tnow); //NTP.getMinute ();
  ss = second (tnow); //NTP.getSecond ();
  //animations?
  cm = millis ();
  //
#ifdef USE_FIREWORKS
  //fireworks on 1st of Jan 00:00, for 55 seconds
  if (1 || (month (tnow) == 1 && day (tnow) == 1))
  {
    if ((hh == 0 && mm == 0) && (ss > 0 && ss < FIREWORKS_DISPLAY))
    {
      if ((cm - last) > FIREWORKS_LOOP)
      {
        //Serial.println(millis() - last);
        last = cm;
        i++;
        fireworks_loop (i);
      }
      ntpsync = 1;
      return;
    }
  }
#endif //define USE_FIREWORKS

  //weather animations
  if ((cm - last) > 150)
  {
    //Serial.println(millis() - last);
    last = cm;
    i++;
    //
    draw_animations (i);
    //
  }
  //
  if (ntpsync)
  {
    ntpsync = 0;
    //
    prevss = ss;
    prevmm = mm;
    prevhh = hh;
    //brightness control: dimmed during the night(25), bright during the day(150)
    if (hh >= 20 && cin == 150)
    {
      cin = 25;
      Serial.println ("night mode brightness");
      daytime = 0;
    }
    if (hh < 8 && cin == 150)
    {
      cin = 25;
      Serial.println ("night mode brightness");
      daytime = 0;
    }
    //during the day, bright
    if (hh >= 8 && hh < 20 && cin == 25)
    {
      cin = 150;
      Serial.println ("day mode brightness");
      daytime = 1;
    }
    //we had a sync so draw without morphing
    int cc_gry = display.color565 (128, 128, 128);
    int cc_dgr = display.color565 (30, 30, 30);
    //dark blue is little visible on a dimmed screen
    //int cc_blu = display.color565 (0, 0, cin);
    int cc_col = cc_gry;
    //
    if (cin == 25)
      cc_col = cc_dgr;
    //reset digits color
    digit0.SetColor (cc_col);
    digit1.SetColor (cc_col);
    digit2.SetColor (cc_col);
    digit3.SetColor (cc_col);
    digit4.SetColor (cc_col);
    digit5.SetColor (cc_col);
    //clear screen
    display.fillScreen (0);
    //date and weather
    draw_weather ();
    draw_date ();
    //
    digit1.DrawColon (cc_col);
    digit3.DrawColon (cc_col);
    //military time?
    if (hh > 12 && military[0] == 'N')
      hh -= 12;
    //
    digit0.Draw (ss % 10);
    digit1.Draw (ss / 10);
    digit2.Draw (mm % 10);
    digit3.Draw (mm / 10);
    digit4.Draw (hh % 10);
    digit5.Draw (hh / 10);
  }
  else
  {
    //seconds
    if (ss != prevss) 
    {
      int s0 = ss % 10;
      int s1 = ss / 10;
      if (s0 != digit0.Value ()) digit0.Morph (s0);
      if (s1 != digit1.Value ()) digit1.Morph (s1);
      //ntpClient.PrintTime();
      prevss = ss;
      //refresh weather every 5mins at 30sec in the minute
      if (ss == 30 && ((mm % 5) == 0))
        getWeather ();
    }
    //minutes
    if (mm != prevmm)
    {
      int m0 = mm % 10;
      int m1 = mm / 10;
      if (m0 != digit2.Value ()) digit2.Morph (m0);
      if (m1 != digit3.Value ()) digit3.Morph (m1);
      prevmm = mm;
      //
#define SHOW_SOME_LOVE
#ifdef SHOW_SOME_LOVE
      if (mm == 0)
        draw_love ();
      else
#endif
        draw_weather ();
    }
    //hours
    if (hh != prevhh) 
    {
      prevhh = hh;
      //
      draw_date ();
      //brightness control: dimmed during the night(25), bright during the day(150)
      if (hh == 20 || hh == 8)
      {
        ntpsync = 1;
        //bri change is taken care of due to the sync
      }
      //military time?
      if (hh > 12 && military[0] == 'N')
        hh -= 12;
      //
      int h0 = hh % 10;
      int h1 = hh / 10;
      if (h0 != digit4.Value ()) digit4.Morph (h0);
      if (h1 != digit5.Value ()) digit5.Morph (h1);
    }//hh changed
  }
  //set NTP sync interval as needed
  if (NTP.getInterval() < 3600 && year(now()) > 1970)
  {
    //reset the sync interval if we're already in sync
    NTP.setInterval (3600 * 24);//re-sync once a day
  }
  //
	//delay (0);
}
