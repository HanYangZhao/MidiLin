

/*
* Arduino Ribbon Synth MIDI controller
* ------------------------------------
* ©2015 Dean Miller
* Modified by hyz
*/

#include <EEPROM.h>
#include <Wire.h>
#include <Adafruit_NeoPixel.h>
#include <QuickStats.h>
#ifdef __AVR__
  #include <avr/power.h>
#endif

#define PIN_LED   13
#define NEO_PIN   21
#define N_PIXELS  29
#define TRANSPOSE_UP 4
#define TRANSPOSE_DOWN 2
#define S1_VCC    8
#define VOLUME    A8
#define MODAL     A1
#define S0        A2
#define M0        A3
#define S1        A4
#define M1        A5


#define T0        A0
#define T1        A0
#define T2        A0
#define T3        A0

#define JSX       A6
#define JSY       A7

#define JSSEL     7

#define THRESH    600
#define N_STR     2
#define N_FRET    25
#define N_KEYS    16
#define S_PAD     3
#define T_PAD     300

#define MOD_THRESHOLD 50

//---Midi CC----
#define VOLUME_CC 7
#define MOD_CC 1
#define MIDI_CHANNEL 0
#define VOLCA_VOLUME_CC 11
#define VOLCA_MOD_CC 46
#define VOLCA_MIDI_CHANNEL 10
#define MUTE_CC 123 

long noteDebounceTime = 0;
int noteDebounceDelay = 25;

long lastDebounceTime = 0;
int debounceDelay = 200;

long ledDebounceTime = 0;
int ledDebounceDelay = 20;

short fretDefs[N_STR][N_FRET];

int mod_final;
int vol;
int vol_1;
int modal;
int modal_buffer;
int buffer_mod[2];
int mod[2];
int mod_init[2];
int s_init[2];
int pre_vol;
int pre_mod;
bool volca = false;
int volume_cc = VOLUME_CC;
int mod_cc = MOD_CC;
int channel = MIDI_CHANNEL;

int modal_array [6][7] =  {{0,2,4,5,7,9,11},   //ionian
                           {0,2,3,5,7,9,10},   //dorian
                           {0,1,3,5,7,8,10},   //phyrgian
                           {0,2,4,6,7,9,11},   //lydian
                           {0,2,4,5,7,9,10},   //mxyolydian
                           {0,2,3,5,7,8,10}};    //aeolian
                           //{0,1,3,5,6,8,10}};

short T_vals[N_STR];
bool T_active[] = {false, false, false, false}; //is it currently active
short T_hit[N_STR];                             //has it been hit on this loop
int T_pins[] = {T0, T1, T2, T3};

short S_vals[N_STR];                            //current sensor values
short S_old[N_STR];                             //old sensor values for comparison
int S_active[N_STR];                            //currently active notes
int S_pins[] = {S0,S1};
int fretTouched[N_STR];

bool modreset = true;

//E A D G
int offsets_default[] = {40, 45, 50, 55};

//B E A D
int offsets_transposed[] = {35, 40, 45, 50};

//default offsets
int offsets[] = {40, 45, 50, 55};


Adafruit_NeoPixel pixels = Adafruit_NeoPixel(N_PIXELS, NEO_PIN, NEO_GRB + NEO_KHZ800);
int led_number[2] = {0,0};
int led_color;
int led;
int prev_led;

bool stickActive = false;
bool stickState = false;
bool btnState = false;
int stickZeroX;
int stickZeroY;

unsigned long last_read;

QuickStats stats;

void setup() {
  
  //read fret definitions from EEPROM
  for (int i=0; i<N_STR; i++){
    for (int j=0; j<N_FRET; j++){
      fretDefs[i][j] = EEPROMReadShort(j * sizeof(short) + (N_FRET*i*sizeof(short)));
    }
  }
  Serial.begin(31250);
  //Serial.begin(115200);
  
  for(int i=0; i<N_STR; i++){
    pinMode(T_pins[i], INPUT);
    pinMode(S_pins[i], INPUT);
  }
  
  pinMode(JSX, INPUT);
  pinMode(JSY, INPUT);
  pinMode(JSSEL, INPUT);
  digitalWrite(JSSEL, HIGH);

  pinMode(PIN_LED, OUTPUT);

  pixels.begin();
  pixels.setBrightness(50);
  pixels.show();
  //calibrate joystick
  stickZeroX = analogRead(JSX);
  stickZeroY = analogRead(JSY);

  //calibrate();

  pinMode(TRANSPOSE_UP, INPUT_PULLUP);
  pinMode(TRANSPOSE_DOWN, INPUT_PULLUP);
  pinMode(A0,INPUT);
  pinMode(3,INPUT_PULLUP);
  //pinMode(S1_VCC, OUTPUT);
  pinMode(VOLUME,INPUT);
  pinMode(M0,INPUT);
  pinMode(M1,INPUT);
  //digitalWrite(S1_VCC, HIGH);c
  mod_init[0] = analogRead(M0);
  mod_init[1] = analogRead(M1);
  s_init[1] = analogRead(S1);
  s_init[0] = analogRead(S0);
}

void loop() {
  readButtons();
  readModulationAndVol();
  readControls();
  determineFrets();
  legatoTest();
  pickNotes();
  //readJoystick();
  cleanUp();
  delay(5);
}

void readModulationAndVol(){
  buffer_mod[0] = analogRead(M0);
  buffer_mod[1] = analogRead(M1);
  vol_1 = analogRead(VOLUME);
  modal_buffer = analogRead(MODAL);
  modal_buffer = map(modal_buffer, 0, 700, 0, 7);
  mod[1] = map(buffer_mod[1],mod_init[1],mod_init[1]+400,0,127);
  mod[0] = map(buffer_mod[0], 500,500+300, 0, 127);
  mod_final = max(mod[0],mod[1]);
  vol_1 = map(vol_1, 0, 300, 0, 127);
  vol = vol_1;
  if(abs(modal_buffer != modal)){
    if(modal_buffer > 7){
      modal = 7;
      modal_buffer = 7;      
    }

    else
      modal = modal_buffer;
    onLED(modal+1,0,255,0);
    delay(500);
    onLED(N_PIXELS,0,0,0);
    Serial.println(modal);
  }
  
  if(abs(vol - pre_vol) > 1 && vol <= 127){
    Serial.println("vol");
    if (vol >= 127)
      vol = 127;
    if (vol <= 1)
      vol = 0;
    controllerChange(volume_cc,vol);
    pre_vol = vol;
  }
  if(abs(mod_final - pre_mod) > 5){
   
    if (mod_final < MOD_THRESHOLD )
      controllerChange(mod_cc,0);
    else if ( mod_final <= 127 )
      controllerChange(mod_cc,mod_final);
       Serial.println("mod");
    pre_mod = mod_final;    
  }
}

void readButtons(){
  int up = digitalRead(TRANSPOSE_UP);
  int down = digitalRead(TRANSPOSE_DOWN);
  int clear_notes = digitalRead(3);
  //Serial.println(up);
  if((millis() - lastDebounceTime ) > debounceDelay){
    //Serial.println("tranposeing");
    if (!down && !clear_notes)
      transpose(-2);
    else if (!up && !clear_notes)
      transpose(2);
    else if(!up && down)
      transpose(1);
    else if(!down && up)
      transpose(-1);
    else if(!up && !down){
      if(!volca){
        volume_cc = VOLCA_VOLUME_CC;
        mod_cc = VOLCA_MOD_CC;
        channel = VOLCA_MIDI_CHANNEL;
        volca = true;
        Serial.println("volca");
      }
      else if (volca){
        volume_cc = VOLUME_CC;
        mod_cc = MOD_CC;
        channel = MIDI_CHANNEL;
        volca = false;
        Serial.println("!volca");
      }         
    }
  
    else if (!clear_notes){
      controllerChange(MUTE_CC,0);
    }

    lastDebounceTime = millis();
  }
}


void pickNotes(){
  for (int i=0; i<N_STR; i++){
    if(T_hit[i]){
      if(S_active[i]){
        //turn off active note on this string
        noteOff(0x80 + channel, S_active[i]);
        //Serial.println("picknoteoff");
      }
      if (fretTouched[i] == 1){
        noteOff(0x80 + channel, S_active[i]);
        continue;
      }
      else{
        S_active[i] = fretTouched[i] + offsets[i];
        noteOn(0x90 + channel, S_active[i], 100);
        //Serial.println("picknoteon");        
      }

    }
  }
}

void legatoTest(){
  for(int i=0; i<N_STR; i++){
    if(S_active[i]){

      int note = fretTouched[i] + offsets[i];
      if (note != S_active[i] && fretTouched[i] == -1){
        noteOff(0x80 + channel, S_active[i]);
        
        S_active[i] = note;
        //clrLED();
        continue;
        
      }

      if(note != S_active[i] && (fretTouched[i] || T_active[i])){
        Serial.println("legatonote");
        noteOn(0x90 + channel, note, 120);
        noteOff(0x80 + channel, S_active[i]);
        S_active[i] = note;

        

      }
    }
  }

  led = max(led_number[0],led_number[1]);
  //Serial.println(led_number[0]);
   //Serial.println(led_number[1]);
  if (led == -1){
    led = 0;
    onLED(N_PIXELS,0,0,0);
  }
    
  else{
    led = led + 1;
    led = map(led,0,N_FRET - 1,0,N_PIXELS);
    led_color = map(led,0,30,0,255);
    //Serial.println(led_number);
    if((millis() - ledDebounceTime ) > ledDebounceDelay){
      for(int i=0;i<led;i++){
        //pixels.Color takes RGB values, from 0,0,0 up to 255,255,255
        pixels.setPixelColor(i, Wheel(led_color));
      }
      if(prev_led > led)
        for(int i=led;i<N_PIXELS;i++){
          //pixels.Color takes RGB values, from 0,0,0 up to 255,255,255
          pixels.setPixelColor(i, (pixels.Color(0, 0, 0)));
        }
        pixels.show();
        ledDebounceTime = millis(); 
        prev_led = led ;
    }
  }

  
    
}

void cleanUp(){
  for (int i=0; i<N_STR; i++){
    if(S_active[i] && !fretTouched[i] && !T_active[i]){
        noteOff(0x80 + channel, S_active[i]);
        S_active[i] = 0;
    }
  }
}

void readControls(){
    //read the strings and the triggers
    for (int i=0; i<N_STR; i++){
      T_hit[i] = checkTriggered(i);
      //Serial.println(T_hit[i]);
      //if(i == 1 && abs(buffer_mod[i] - mod_init[i] > 1)){
       S_vals[i] = analogRead(S_pins[i]);
        //Serial.println(S_vals[i]);
       //if(abs(S_vals[i] - s_init[i]) < 5)
         //S_vals[i] = 0;
      //}
//      else if(i == 0)
//        S_vals[i] = analogRead(S_pins[i]);
      //else
        //S_vals[i] = 0;

    }

}

void determineFrets () {
 //---------Get Fret Numbers------
 for (int i=0; i< N_STR; i++) {
   
   short s_val = S_vals[i];
   
    //check for open strings
   if (s_val == 0 ) {
    S_old[i] = s_val;
    fretTouched[i]=-1;
    led_number[i] = fretTouched[i];
  }

  else{
      //loop through the array of fret definitions
    for (int j=1; j<N_FRET; j++) {
      int k = j - 1;
      if (s_val >= fretDefs[i][j] && 
        s_val < fretDefs[i][k] &&
      abs((int)s_val-(int)S_old[i]) > S_PAD) {
        
        S_old[i] = s_val;
        fretTouched[i] = j - 1;
        led_number[i] = fretTouched[i];
        if(modal < 7){
          // not chromatic mode
          if(i == 0) 
            fretTouched[i] = modal_array[modal][fretTouched[i]%7] + (fretTouched[i]/7) * 12;
          else if(i == 1)
          {
             if(modal == 3)
              fretTouched[i] = (modal_array[(modal+3)%6][fretTouched[i]%7] + (fretTouched[i]/7) * 12) + 2; //fix for locrian
             else if(modal > 3){
              fretTouched[i] = (modal_array[(modal+2)%6][fretTouched[i]%7] + (fretTouched[i]/7) * 12);
             }
             else{
              fretTouched[i] = modal_array[(modal+3)%6][fretTouched[i]%7] + (fretTouched[i]/7) * 12;
             }
          }

        }          
        Serial.println("fret");
        Serial.println(i);
        Serial.println(fretTouched[i]);
        Serial.println("");
       }
     }
   }
 }

}


void unset(int i){
  //this function doesn't even do anything!!
}



void calibrate(){
  if (1) {
  int btn = 1;
  Serial.println("calibrating...");
  for (int i=0; i<N_STR; i++) {
    //Flash the LED too indicate calibration
    onLED(10,250,0,0);
    delay(100);
    clrLED();
    onLED(10,250,0,0);
    delay(100);
    clrLED();
    onLED(10,250,0,0);
    delay(100);
    clrLED();
  
    short sensorMax = 0;
    short sensorMin = 1023;
    short val;
    
      //loop through the array of fret definitions
      for (int j=N_FRET - 1; j>=0; j--) {
      
        int response = false;
      
        //wait for response
        Serial.println("waiting");
        while (!response) {
           
           if (checkTriggered(i)) {
              val = (analogRead(S_pins[i]));
              response = true;


                      //write to memory
        clrLED();
        int addr = j * sizeof(short) + (N_FRET*i*sizeof(short));
        Serial.print("Writing ");
        Serial.print(val);
        Serial.print(" to address: ");
        Serial.println(addr);
        EEPROMWriteShort(addr, val);
          }
          delay(10);
        }
      

        
        delay(100);
        onLED(10,250,0,0);
      }
    for (int j=0; j<N_FRET; j++) {
      short v = EEPROMReadShort(j * sizeof(short) + (N_FRET*i*sizeof(short)));
      fretDefs[i][j] = v;
    }
  }
  
  clrLED();
  }
}

void EEPROMWriteShort(int address, int value){
      //One = Most significant -> Two = Least significant byte
      byte two = (value & 0xFF);
      byte one = ((value >> 8) & 0xFF);

      //Write the 4 bytes into the eeprom memory.
      EEPROM.write(address, two);
      EEPROM.write(address + 1, one);
}

short EEPROMReadShort(int address){
      //Read the 2 bytes from the eeprom memory.
      long two = EEPROM.read(address);
      long one = EEPROM.read(address + 1);

      //Return the recomposed short by using bitshift.
      return ((two << 0) & 0xFF) + ((one << 8) & 0xFFFF);
}

void onLED(int led,int red,int green, int blue){
  for(int i=0;i<led;i++){
    //pixels.Color takes RGB values, from 0,0,0 up to 255,255,255
    pixels.setPixelColor(i, pixels.Color(red, green, blue));
  }
  pixels.show();
}

void clrLED(){
  for(int i=0;i<N_PIXELS;i++){
    pixels.setPixelColor(i, pixels.Color(0,0,0)); // turn off
  }
  pixels.show();
}

//check if a trigger has been hit. Return 0 if not, the value of the trigger if it has
short checkTriggered(int i){
  
  short v = analogRead(T_pins[i]);
  //Serial.println(v);
  T_vals[i] = v;
  short ret = 0;
  if(!T_active[i] && v > THRESH){
    T_active[i] = true;
    ret = v;
    Serial.println("triggered");
  }
  else if(T_active[i] && v < THRESH - T_PAD){
    T_active[i] = false;
    Serial.println("un-triggered");
  }
  //Serial.println(ret);
  return ret;
}

void transpose(int dir){
  switch(dir){
    case 1:
      for (int i=0; i<N_STR; i++) {
        offsets[i] = offsets[i] + 1;
        Serial.println(offsets[0]);
      }
      break;
  
    case -1:
      for (int i=0; i<N_STR; i++) {
        offsets[i] = offsets[i] - 1;
        Serial.println(offsets[0]);
      }
      break;
   case 2:
      for (int i=0; i<N_STR; i++) {
        offsets[i] = offsets[i] + 12;
        Serial.println(offsets[0]);
      }
      break;
   case -2:
      for (int i=0; i<N_STR; i++) {
        offsets[i] = offsets[i] - 12;
        Serial.println(offsets[0]);
      }
      break;   
  }
}

void readJoystick(){
   if (digitalRead(JSSEL) == LOW) {
    //activate joystick
    if (!btnState) {
      //make sure modwheel value is set to 0 when stick is off
      if (stickActive) controllerChange(1, 0);
      stickActive = !stickActive;
      Serial.println(stickActive);
    }
    btnState = true;
  }
  //reset once stick is no longer beingnote pressed
  if (digitalRead(JSSEL) == HIGH && btnState) btnState = false;
  
  if (stickActive) {
    //read positions from center
    float xPos = map(analogRead(JSX), stickZeroX, 1023, 0, 127);
    float yPos = map(analogRead(JSY), stickZeroY, 1023, 0, 127);
    
    //get absolute position from center
    float z = sqrt(sq(xPos) + sq(yPos));
    int stickVal = (int)constrain(z, 0, 127);
    
    if (stickVal > 0) {
      stickState = true;
      controllerChange(1, stickVal);
    }
    else if (stickState && stickVal == 0) {
      stickState = false;
      controllerChange(1, 0);
    }
  }
}

//note-on message
void noteOn(int cmd, int pitch, int velocity) {
  
  Serial.write(byte(cmd));
  Serial.write(byte(pitch));
  Serial.write(byte(velocity));
  digitalWrite(PIN_LED, HIGH);
}
//note-off message
void noteOff(int cmd, int pitch) {
  
  Serial.write(byte(cmd));
  Serial.write(byte(pitch));
  Serial.write(byte(0));
  digitalWrite(PIN_LED, LOW);
}

//Sends controller change to the specified controller
void controllerChange(int controller, int value) {
  Serial.println("cc");
  int ch = 176 + channel;
  Serial.write(byte(ch));
  Serial.write(byte(controller));
  Serial.write(byte(value));
}


// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if(WheelPos < 85) {
    return pixels.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if(WheelPos < 170) {
    WheelPos -= 85;
    return pixels.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return pixels.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}

