/*
* Arduino Ribbon Synth MIDI controller
* ------------------------------------
* ©2015 Dean Miller
*/

#include <EEPROM.h>
#include <Wire.h>
#include "Adafruit_Trellis.h"
#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
  #include <avr/power.h>
#endif

#define PIN_LED   13
#define NEO_PIN   11
#define N_PIXELS  2
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

#define MOD_THRESHOLD 20

long noteDebounceTime = 0;
int noteDebounceDelay = 25;

long lastDebounceTime = 0;
int debounceDelay = 200;

short fretDefs[N_STR][N_FRET];

int mod;
int vol;
int vol_1;
int modal;
int modal_buffer;
int mod_1;
int mod_2;
int mod_2_init;
int pre_vol;
int pre_mod;


int modal_array [2][7] =  {{0,2,4,5,7,9,11},
                    {0,2,3,5,7,8,10}};


short T_vals[N_STR];
bool T_active[] = {false, false, false, false}; //is it currently active
short T_hit[N_STR];                             //has it been hit on this loop
int T_pins[] = {T0, T1, T2, T3};

short S_vals[N_STR];                            //current sensor values
short S_old[N_STR];                             //old sensor values for comparison
int S_active[N_STR];                            //currently active notes
int S_pins[] = {S0,S1};
int fretTouched[N_STR];

bool lightsActive = false;

bool transposed = false;
bool modreset = true;

//E A D G
int offsets_default[] = {40, 45, 50, 55};

//B E A D
int offsets_transposed[] = {35, 40, 45, 50};

//default offsets
int offsets[] = {40, 45, 50, 55};

Adafruit_Trellis matrix0 = Adafruit_Trellis();
Adafruit_TrellisSet trellis =  Adafruit_TrellisSet(&matrix0);
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(N_PIXELS, NEO_PIN, NEO_GRB + NEO_KHZ800);

bool stickActive = false;
bool stickState = false;
bool btnState = false;
int stickZeroX;
int stickZeroY;

int channel = 0;
int channelButtons[] = {4, 5, 6, 7};

unsigned long last_read;


void edgeLightsBlue(int btn){
  if (trellis.justPressed(btn)) {
    edgeLight(0, 0, 255);
  }
}

void edgeLightsRed(int btn){
  if (trellis.justPressed(btn)) {
    edgeLight(255, 0, 0);
  }
}

//map out all the trellis button functions here
void (*fns[]) (int i) = {
  edgeLightsBlue,    //0
  edgeLightsRed,     //1
  unset,             //2
  unset,             //3
  unset,             //4
  unset,             //5
  unset,             //6
  unset,             //7
  unset,             //8
  unset,             //9
  unset,             //10
  unset,             //11
  unset,             //12
  unset,             //13
  unset,             //14
  transpose,         //15
};

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

  trellis.begin(0x70);
    // light up all the LEDs in order
  for (uint8_t i=0; i<N_KEYS; i++) {
    trellis.setLED(i);
    trellis.writeDisplay();    
    delay(50);
  }
  // then turn them off
  for (uint8_t i=0; i<N_KEYS; i++) {
    trellis.clrLED(i);
    trellis.writeDisplay();    
    delay(50);
  }
  pixels.begin();
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
  mod_2_init = analogRead(M1);
}

void loop() {
  //we can only read the trellis every 30ms which is ok
  //if(millis() - last_read >= 30){
  //  readTrellis();
  //  last_read = millis();
  //}
  readTranspose();
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
  mod_1 = analogRead(M0);
  mod_2 = analogRead(M1);
  vol_1 = analogRead(VOLUME);
  modal_buffer = analogRead(MODAL);
  modal_buffer = map(modal_buffer, 0, 800, 0, 2);
  mod_2 = map(mod_2,mod_2_init,mod_2_init+300,0,127);
  mod_1 = map(mod_1, 500, 850, 0, 127);
  mod = max(mod_1,mod_2);
  vol_1 = map(vol_1, 0, 300, 0, 127);
  vol = vol_1;

  if(abs(modal_buffer != modal)){
    modal = modal_buffer;
    Serial.println(modal);
  }
  
  if(abs(vol - pre_vol) > 5 && vol <= 127){
    Serial.println("vol");
    if (vol >= 127)
      vol = 127;
    if (vol <= 10)
      vol = 0;
    controllerChange(7,vol);
    pre_vol = vol;
  }
  if(abs(mod - pre_mod) > 5){
    Serial.println("mod");
    if (mod < MOD_THRESHOLD )
      controllerChange(1,0);
    else if ( mod <= 127 )
      controllerChange(1,mod);
    pre_mod = mod;    
  }


  
}

void readTranspose(){
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
    else if(!up)
      transpose(1);
    else if(!down)
      transpose(-1);
    else if (!clear_notes){
      controllerChange(120,0);
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
  if((millis() - noteDebounceTime ) > noteDebounceDelay){
    //read the strings and the triggers
    for (int i=0; i<N_STR; i++){
      T_hit[i] = checkTriggered(i);
      //Serial.println(T_hit[i]);
     S_vals[i] =  analogRead(S_pins[i]);
      //Serial.println(S_vals[i]);
    }
    noteDebounceTime = millis();
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
        if(modal != 0) // not chromatic mode
          fretTouched[i] = modal_array[modal-1][fretTouched[i]%7] + (fretTouched[i]/7) * 12;
        Serial.println("fret");
        Serial.println(i);
        Serial.println(fretTouched[i]);
        Serial.println("");
       }
     }
   }
 }

}

void readTrellis(){
  // If a button was just pressed or released...
  if (trellis.readSwitches()) {
    // go through every button
    for (uint8_t i=0; i<N_KEYS; i++) {
      fns[i](i);
    }
    // tell the trellis to set the LEDs we requested
    trellis.writeDisplay();
  }
}

void unset(int i){
  //this function doesn't even do anything!!
}

void light(int i){
  //light up the button 
  
  // if it was pressed, turn it on
  if (trellis.justPressed(i)) {
    trellis.setLED(i);
  } 
  // if it was released, turn it off
  if (trellis.justReleased(i)) {
    trellis.clrLED(i);
  }
}

void calibrate(){
  if (1) {
  int btn = 1;
  Serial.println("calibrating...");
  for (int i=0; i<N_STR; i++) {
    //Flash the LED too indicate calibration
    setLED(btn);
    delay(100);
    clrLED(btn);
    delay(100);
    setLED(btn);
    delay(100);
    clrLED(btn);
    delay(100);
    setLED(btn);
  
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
        clrLED(btn);
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
        setLED(btn);
      }
    for (int j=0; j<N_FRET; j++) {
      short v = EEPROMReadShort(j * sizeof(short) + (N_FRET*i*sizeof(short)));
      fretDefs[i][j] = v;
    }
  }
  
  clrLED(btn);
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

void setLED(int i){
  trellis.setLED(i);
  trellis.writeDisplay();
}

void clrLED(int i){
  trellis.clrLED(i);
  trellis.writeDisplay();
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



void edgeLight(int red, int green, int blue){
  lightsActive = !lightsActive;
    if(lightsActive){
      for(int i=0;i<N_PIXELS;i++){
        // pixels.Color takes RGB values, from 0,0,0 up to 255,255,255
        pixels.setPixelColor(i, pixels.Color(red, green, blue));
   }
   pixels.show();
    }
    else{
      for(int i=0;i<N_PIXELS;i++){
        pixels.setPixelColor(i, pixels.Color(0,0,0)); // turn off
      }
      pixels.show();
    }
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

/*
void channel1(int btn){
  if (trellis.justPressed(btn)) {
    
  }
}

void setChannel(int c){
   for(int i=0; i<sizeof(channelButtons)/sizeof(int); i++){
    channel
  }
  channel = c;
}
*/


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
  Serial.write(byte(0xb2));
  Serial.write(byte(controller));
  Serial.write(byte(value));
}

