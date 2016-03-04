/*
 * Demo of ThingSpeak use (post to channel and tweet)
 *
 * To use:
 *    Configure the Simblee with the RGB LED Shield 
 * 
 *    Create a ThingSpeak account (https://thingspeak.com/)
 *    Link to Twitter and get a Tweet Key (https://thingspeak.com/apps/thingtweets)
 *    Update the tweetKey variable below to include the ThingSpeek Twitter API Key 
 *    
 *    Open the Simblee app on a mobile device.  While connected to the Simblee
 *    either the "Tweet" on the Mobile device or Button B on the Simblee will 
 *    send a tweet.
 *
 *    See: https://github.com/bsiever/SimbleeForMobile-ThingSpeak-Tweet
 */ 

#include "SimbleeForMobile.h"
#include "SimbleeForMobileClient.h"

// ***** Data / Global Variables
const char *tweetKey = "XXXXXXXXXXXXXXXX";  // TODO: Update this with your key!
const char *apiKey =   "XXXXXXXXXXXXXXXX";  // TODO: Update this with your key!

// SimbleeForMobile related data
SimbleeForMobileClient client;
uint8_t uiButtonId;                     // Button in User Interface on Mobile device

// Physical button data
const int physicalButtonPin = 5;        // Physical button pin

// ***** Forward declarations of functions
void thingTweet(char *message);
void thingSpeak(char *data[], int numfields);
void updatePhysicalButtonState();
void tweet(); 

// ***** Arduino Standard startup / loop
void setup()
{
  Serial.begin(9600);
  printf("Starting...\n");
  SimbleeForMobile.advertisementData = "Tweet";
  SimbleeForMobile.deviceName = "Tweeter";
  SimbleeForMobile.begin();  
  pinMode(physicalButtonPin, INPUT);
}

void loop()
{
  updatePhysicalButtonState();
  // process must be called in the loop for SimbleeForMobile
  SimbleeForMobile.process();
}

// ***** SimbleeForMobile standard functions
void ui()
{
  color_t darkgray = rgb(85,85,85);
  int width = SimbleeForMobile.screenWidth;
  int height = SimbleeForMobile.screenHeight;
    
  SimbleeForMobile.beginScreen(darkgray);

  uiButtonId = SimbleeForMobile.drawButton(10, height/2, width-20, "Tweet", BLUE, TEXT_TYPE);
  
  SimbleeForMobile.setEvents(uiButtonId, EVENT_PRESS);
  SimbleeForMobile.endScreen();
}

void ui_event(event_t &event)
{
  if(event.id == uiButtonId) {
    // Trigger a tweet if the button is touched
    tweet();  
  }
}


// Send a specific tweet
void tweet() {
  static int count=0;
  count++;
  char buffer[140];
  sprintf(buffer,"Hello World %d",count);
  thingTweet(buffer);
}


// Button Callback --- called if the physical button is pressed
void physicalButtonPressed() {
  tweet();  // Trigger a tweet 
}

// Debounce the physical button
void updatePhysicalButtonState() {
  static int state = 0;
  static int onStart = 0;
  static int offStart = 0;

  int currentState = digitalRead(physicalButtonPin);
  
  switch(state) {
    case 0: // Waiting for ON
      if(currentState) {
        state = 1;
        onStart = millis();
      }
      Simblee_pinWake(physicalButtonPin, HIGH);
    break;

    case 1: // Waiting for on time
      if(millis() - onStart > 50) 
        state = 2;
    break;

    case 2: // Check it
      //printf("state 2\n");
      if(currentState) { 
          // if still on 
         physicalButtonPressed();
         state = 3; // Wait for release
         Simblee_pinWake(physicalButtonPin, LOW);
      } else { // Reset
        state = 0; // Wait for on
        Simblee_pinWake(physicalButtonPin, HIGH);
      }
      break;
      
    
    case 3: // Waiting for off time
      if(!currentState) {
         offStart = millis();
         state = 4;
      }
    break;

    case 4: // Waiting for off
    default:
      if((millis()-offStart) >  200 ) {
        if(!currentState) {
          state=0;
        }
      }
    break;
  }
}

// ***** ThingSpeak data delivery functions & Data



/*
 * thingSpeak:  data is an array of character strings to fill in for field1, field2, ...
 *              numfields is the number of items in data
 *              
 *              ThingSpeak can only be updated every 15s
 */
void thingSpeak(char *data[], int numfields) {
/* These strings are for the ThingSpeak Channel API (posting data to a channel) */
    const char *postString = // ~ 155 characters 
        "POST /update HTTP/1.1\r\n"
        "Host: api.thingspeak.com\r\n"
        "Connection: close\r\n"
        "X-THINGSPEAKAPIKEY: %s\r\n"
        "Content-Type: application/x-www-form-urlencoded\r\n"
        "Content-Length: %d\r\n"
        "\r\n"
        "%s\r\n";
  char buffer[250];
  char fields[250];
  if(!client.connected()) {
    uint8_t connectStatus = client.connect("thingspeak.com", 80);
    if(connectStatus == 1) {
      fields[0] = 0;
      int offset = 0;
      for(int i=0;i<numfields;i++) {
        offset += sprintf(fields+offset, "field%d=%s%s",i+1, data[i], i<numfields-1?"&":"");  
      }
      sprintf(buffer, postString, apiKey, offset+2, fields);
      client.write((uint8_t*)buffer, strlen(buffer));
      client.stop();
    }
  }  
}

/*
 * thingTweet:  message is the message to tweet (<=120 characters)
 */
void thingTweet(char *message) {
  const char *tweetString = "POST /apps/thingtweet/1/statuses/update HTTP/1.1\r\n"
      "Host: api.thingspeak.com\r\n"
      "Connection: close\r\n"
      "Content-Type: application/x-www-form-urlencoded\r\n"
      "Content-Length: %d\r\n"
      "\r\n"
      "%s\r\n";
  const char *tweetData = "api_key=%s&status=%s";  // 16+len(API)+len(status) = 16+16+140 = 172
  char tweetLine[166]; // 10 (key) + 16 (setup text) + 140 (tweet)
  char buffer2[325]; // >=155+166
  if(!client.connected()) {
    uint8_t connectStatus = client.connect("api.thingspeak.com", 80);
    if (connectStatus==1) {
      // Build the tweet line
      sprintf(tweetLine, tweetData, tweetKey, message);
      // Build the complete POST request
      sprintf(buffer2, tweetString, strlen(tweetLine), tweetLine);
      // Post it
      client.write((uint8_t*)buffer2, strlen(buffer2));
      // Stop / Close the socket
      client.stop();   
    }
  }   
}

