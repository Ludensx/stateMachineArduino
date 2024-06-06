#include <StateMachineLib.h>
#include <LiquidCrystal.h>
#include <Keypad.h>
#include <dht.h>

dht DHT;

String pass = "1234";
String contra = "";

int cont = 0;//Contador de intentos
int contE = 0;//Contador para escribir
int contWait = 0;//Contador para esperar

bool seguridadB = true;
bool conf = true;

//Song
bool band = true;
#define NOTE_D4  294
#define NOTE_E4  330
#define NOTE_G4  392
#define NOTE_GS4 415
#define NOTE_A4  440
#define NOTE_AS4 466
#define NOTE_B4  494
#define NOTE_C5  523
#define REST      0

#define NOTE_DS4 311
#define NOTE_FS4 370

int melodyBloq[] = {
  NOTE_C5,-4, NOTE_G4,-4, NOTE_E4,4,
  NOTE_A4,-8, NOTE_B4,-8, NOTE_A4,-8, NOTE_GS4,-8, NOTE_AS4,-8, NOTE_GS4,-8,
  NOTE_G4,8, NOTE_D4,8, NOTE_E4,-2,  
};
int melodyAlarm[] = {
  REST,8, NOTE_DS4,8,
  NOTE_E4,-8, NOTE_FS4,8, NOTE_G4,-8, NOTE_C5,8, NOTE_B4,-8, NOTE_E4,8, NOTE_G4,-8, NOTE_B4,8, NOTE_AS4,2, 
};
//Bloqueo
int tempo = 200;
int notes = sizeof(melodyBloq) / sizeof(melodyBloq[0]) / 2;
int wholenote = (60000 * 4) / tempo;
int divider = 0;
int thisNote = 0;
unsigned long noteDuration = 0;
unsigned long previousMillis = 0;
bool isPlaying = false;
//Alarma
int tempoAlarm = 200;
int notesAlarm = sizeof(melodyAlarm) / sizeof(melodyAlarm[0]) / 2;
int wholenoteAlarm = (60000 * 4) / tempoAlarm;
int dividerAlarm = 0;
int thisNoteAlarm = 0;
unsigned long noteDurationAlarm = 0;
unsigned long previousMillisAlarm = 0;
bool isPlayingAlarm = false;

int buzzer = 7; // Pin del buzzer

//Limites
float limT = 25;
float limL = 500;
float limH = 500;

const char up = '2', down = '5', left = '4', rigth = '6';
const char enter = '*';

char op = 'T';

unsigned long previousLedMillis = 0;
unsigned long prevAlarm = 0;
unsigned long prevAmb = 0;
unsigned long prevEv = 0;
unsigned long prev = 0;
//Led parpadeo
// unsigned long startMillis = 0;
// const long interval = 500; // Intervalo de parpadeo del LED en milisegundos
bool ledState = false; // Estado actual del LED de bloqueo

const int btn = 6;
const int ledRed = 53;
const int ledGreen = 51;
const int ledBlue = 49;

const int ldr = A0, hallP = A1, dhtP = 8;//Sensores

const byte ROWS = 4; 
const byte COLS = 4; 
char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};
byte rowPins[ROWS] = {21, 20, 19, 18};
byte colPins[COLS] = {17, 16, 15, 14};
Keypad keypad = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS );

const int rs = 12, en = 11, d4 = 5, d5 = 4, d6 = 3, d7 = 2;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

// State Alias
enum State
{
	SA = 0,
	SB = 1,
	SC = 2,
	SD = 3,
	Se = 4,
	SF = 5
};

// Input Alias
enum Input
{
	time = 0,
  claveCorrecta = 1,
  sisBloqueado = 2,
  hall = 3,
  temp = 4,
  luz = 5,
  boton = 6,
  unknown = 7
};

// Create new StateMachine
StateMachine stateMachine(6, 12);

// Stores last user input
Input input;

// Setup the State Machine
void setupStateMachine(){
	// Add transitions
	stateMachine.AddTransition(SA, SB, []() { return input == claveCorrecta; });
	stateMachine.AddTransition(SA, SD, []() { return input == sisBloqueado; });

	stateMachine.AddTransition(SB, SC, []() { return input == boton; });

	stateMachine.AddTransition(SC, SB, []() { return input == boton; });
	stateMachine.AddTransition(SC, SF, []() { return input == time; });
	stateMachine.AddTransition(SC, Se, []() { return input == temp; });
	
  stateMachine.AddTransition(SD, SA, []() { return input == time; });
  
  stateMachine.AddTransition(Se, SA, []() { return input == boton; });
  stateMachine.AddTransition(Se, SC, []() { return input == time; });

  stateMachine.AddTransition(SF, SB, []() { return input == boton; });
  stateMachine.AddTransition(SF, Se, []() { return input == hall; });
  stateMachine.AddTransition(SF, SC, []() { return input == time; });

	// Add actions
	stateMachine.SetOnEntering(SA, outputA);
	stateMachine.SetOnEntering(SB, outputB);
	stateMachine.SetOnEntering(SC, outputC);
	stateMachine.SetOnEntering(SD, outputD);
	stateMachine.SetOnEntering(Se, outputE);
	stateMachine.SetOnEntering(SF, outputF);

	stateMachine.SetOnLeaving(SA, []() {Serial.println("Leaving A"); });
	stateMachine.SetOnLeaving(SB, []() {Serial.println("Leaving B"); });
	stateMachine.SetOnLeaving(SC, []() {Serial.println("Leaving C"); });
	stateMachine.SetOnLeaving(SD, []() {Serial.println("Leaving D"); });
	stateMachine.SetOnLeaving(Se, []() {Serial.println("Leaving E"); });
	stateMachine.SetOnLeaving(SF, []() {Serial.println("Leaving F"); });
}

void setup(){
	Serial.begin(9600);

  lcd.begin(16, 2);

	Serial.println("Starting State Machine...");
	setupStateMachine();	
	Serial.println("Start Machine Started");

  pinMode(boton,INPUT);
  pinMode(ledRed, OUTPUT);
  pinMode(ledGreen, OUTPUT);
  pinMode(ledBlue, OUTPUT);

	// Initial state
	stateMachine.SetState(SA, false, true);
}
void loop(){
	input = static_cast<Input>(readInput());

	stateMachine.Update();
}

int readInput(){
	Input currentInput = Input::unknown;
	if (Serial.available())
	{
		char incomingChar = Serial.read();

		switch (incomingChar)
		{
			case '0': currentInput = Input::time; 	break;
			case '1': currentInput = Input::claveCorrecta; break;
			case '2': currentInput = Input::sisBloqueado; break;
			case '3': currentInput = Input::hall; break;
			case '4': currentInput = Input::temp; break;
			case '5': currentInput = Input::luz; break;
			case '6': currentInput = Input::boton; break;
			default: break;
		}
	}
	return currentInput;
}
void seguridad(){
  seguridadB=true;
  contra="";
  contE = 0;
  cont = 0;
  while(seguridadB){
  digitalWrite(ledRed, 0);
  digitalWrite(ledGreen, 0);
  digitalWrite(ledBlue, 0);
    lcd.setCursor(0,0);
    lcd.print("Ingrese Contra:");
    char key = keypad.getKey();
    if (key){
      if(key!='*'){      
        lcd.setCursor(contE++,1);
        contra=contra+key;
        lcd.print("*");
        Serial.println(key);
        Serial.println(contra);
      }else{
      verificar(contra);
      delay(1000);
      contra="";
      lcd.clear();    
      }
    }
  }
}
void verificar(String contra){
  lcd.clear();
  if (contra==pass){
    Serial.println("Correcto");
    lcd.print("Correcta");
    stateMachine.SetState(SB, true, true);
    cont = 0;
    seguridadB=false;
  }else if(cont > 1){
    Serial.println("Bloqueado");
    lcd.print("Bloqueado");
    cont = 0;
    seguridadB=false;
    stateMachine.SetState(SD, true, true);
  }else{
    Serial.println("Incorrecta");
    lcd.print("Incorrecta");
    digitalWrite(ledBlue, 255);
    contE = 0;
    cont++;
  }
}
void songBloq(){
  unsigned long currentMillis = millis();
  if (thisNote < notes * 2) {
    if (!isPlaying) {
      int divider = melodyBloq[thisNote + 1];
      if (divider > 0) {
        noteDuration = (wholenote) / divider;
      } else if (divider < 0) {
        noteDuration = (wholenote) / abs(divider);
        noteDuration *= 1.5;
      }
      
      tone(buzzer, melodyBloq[thisNote], noteDuration * 0.9);
      previousMillis = currentMillis;
      isPlaying = true;
    } else {
      if (currentMillis - previousMillis >= noteDuration) {
        noTone(buzzer);
        thisNote += 2;
        isPlaying = false;
      }
    }
  }
}
void blinkLed(int led, int interval){
  unsigned long currentMillis = millis();

  if (currentMillis - previousLedMillis >= interval) {
    previousLedMillis = currentMillis;
    // Si el LED está apagado, encenderlo y viceversa
    if (ledState) {
      digitalWrite(led, LOW);
    } else {
      digitalWrite(led, HIGH);
    }
    // Cambiar el estado del LED
    ledState = !ledState;
  }
}
void songAlarm(){
  unsigned long currentMillisAlarm = millis();
  if (thisNoteAlarm < notesAlarm * 2) {
    if (!isPlayingAlarm) {
      int dividerAlarm = melodyAlarm[thisNoteAlarm + 1];
      if (dividerAlarm > 0) {
        noteDurationAlarm = (wholenoteAlarm) / dividerAlarm;
      } else if (dividerAlarm < 0) {
        noteDurationAlarm = (wholenoteAlarm) / abs(dividerAlarm);
        noteDurationAlarm *= 1.5;
      }
      
      tone(buzzer, melodyAlarm[thisNoteAlarm], noteDurationAlarm * 0.9);
      prevAlarm = currentMillisAlarm;
      isPlayingAlarm = true;
    } else {
      if (currentMillisAlarm - prevAlarm >= noteDurationAlarm) {
        noTone(buzzer);
        thisNoteAlarm += 2;
        isPlayingAlarm = false;
      }
    }
  }
}
void contTo(int lim, enum State estado){
  unsigned long curr = millis();
  if(curr - prev >= 1000){
    prev = curr;
    Serial.println(contWait++);
  }
  if(contWait>=lim){
    stateMachine.SetState(estado, true, true);
    band=false;
  }
}
void botonHandler(enum State estado){
  delay(100);
  if(digitalRead(btn)==0){
    band=false;
    Serial.println(digitalRead(btn));
    stateMachine.SetState(estado, true, true);
  }
}
void readTemp(){
  Serial.print("Temp: ");
  Serial.println(DHT.temperature);
  lcd.setCursor(0, 0);
  lcd.print("T:");
  lcd.setCursor(2, 0);
  lcd.print(DHT.temperature);
}
void readHum(){
  Serial.print("Hum: ");
  Serial.println(DHT.humidity);
  lcd.setCursor(8, 0);
  lcd.print("Hm:");
  lcd.setCursor(11, 0);
  lcd.print(DHT.humidity);
}
void readLdr(){
  Serial.print("ldr: ");
  Serial.println(analogRead(ldr));
  lcd.setCursor(4, 1);
  lcd.print("Luz:");
  lcd.setCursor(8, 1);
  lcd.print(analogRead(ldr));
}
void readHall(){
  Serial.print("hall: ");
  Serial.println(analogRead(hallP));
  lcd.setCursor(4, 0);
  lcd.print("Hall:");
  lcd.setCursor(9, 0);
  lcd.print(analogRead(hallP));
}
void monitorAmbiental(){
  digitalWrite(ledRed, 0);
  digitalWrite(ledGreen, 0);
  digitalWrite(ledBlue, 0);
  unsigned long currAmb = millis();
  if(currAmb - prevAmb >= 500){
    prevAmb = currAmb;
    int chk = DHT.read11(dhtP);
    lcd.clear();
    readTemp();
    readHum();
    readLdr();
  }
}
void monitorEventos(){
  digitalWrite(ledRed, 0);
  digitalWrite(ledGreen, 0);
  digitalWrite(ledBlue, 0);
  unsigned long currEv = millis();
  if(currEv - prevEv >= 500){
    prevEv = currEv;
    lcd.clear();
    readHall();
  }
}
void alarmLcd(){
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Alarm");
}
bool alarmTemp(){
  if(DHT.temperature > limT) return true;
  else return false;
}
bool alarmLuz(){
  if(analogRead(ldr) > limL) return true;
  else return false;
}
bool alarmHall(){
  if(analogRead(hallP) > limH) return true;
  else return false;
}
void printMenu(){
  lcd.setCursor(2, 0);
  lcd.print("Temp");
  lcd.setCursor(9, 0);
  lcd.print("Luz");
  lcd.setCursor(2, 1);
  lcd.print("Hall");
}
void selectOp(){
  char key = keypad.getKey();
  if(key){
    switch(key){
      case up:
        //op = 'T';
        lcd.clear();
        printMenu();
        op = selTemp();
      break;
      case down:
        //if(op == 'T' || op == 'L') op = 'H';
        lcd.clear();
        printMenu();
        op = selHall();
      break;
      case left:
        //if(op == 'L') op = 'T';
        lcd.clear();
        printMenu();
        op = selTemp();
      break;
      case rigth:
        //if(op == 'T') op = 'L';
        lcd.clear();
        printMenu();
        op = selLuz();
      break;
      case enter:
        lcd.clear();
        menuConfig(op);
      break;
    }
  }
}
char selTemp(){
  lcd.setCursor(1, 0);
  lcd.print("*");
  return('T');
}
char selLuz(){
  lcd.setCursor(8, 0);
  lcd.print("*");
  return('L');
}
char selHall(){
  lcd.setCursor(1, 1);
  lcd.print("*");
  return('H');
}
void menuConfig(char op){
  lcd.clear();
  conf = true;
  while(conf){
    char key = keypad.getKey();
    lcd.setCursor(0,0);
    lcd.print("Config");
    switch(op){
      case 'T':
        lcd.setCursor(7,0);
        lcd.print("Temp");
        lcd.setCursor(0, 1);
        lcd.print(limT);

        if(key){
          if(key == 'A'){
            limT = limT + 1;
          }else if(key == 'B'){
            limT = limT - 1;
          } else if(key == '*'){
            lcd.clear();
            menu();
            conf = false;
          }
        }
      break;
      case 'H':
        lcd.setCursor(7,0);
        lcd.print("Hall");
        lcd.setCursor(0, 1);
        lcd.print(limH);

        if(key){
          if(key == 'A'){
            limH = limH + 50;
          }else if(key == 'B'){
            limH = limH - 50;
          } else if(key == '*'){
            lcd.clear();
            menu();
            conf = false;
          }
        }
      break;
      case 'L':
        lcd.setCursor(7,0);
        lcd.print("Luz");
        lcd.setCursor(0, 1);
        lcd.print(limL);

        if(key){
          if(key == 'A'){
            limL = limL + 50;
          }else if(key == 'B'){
            limL = limL - 50;
          } else if(key == '*'){
            lcd.clear();
            menu();
            conf = false;
          }
        }
      break;
    }
  }
}
void menu(){
  digitalWrite(ledRed, 0);
  digitalWrite(ledGreen, 0);
  digitalWrite(ledBlue, 0);

  printMenu();
  selectOp();
}
void outputA(){
	Serial.println("A   B   C   D   E   F");
	Serial.println("X            ");
	Serial.println();
  seguridad();
}
void outputB(){
  band = true;
	Serial.println("A   B   C   D   E   F");
	Serial.println("    X                ");
	Serial.println();
  lcd.clear();
  while(band){
    menu();
    botonHandler(SC);
  }
}
void outputC(){
  band = true;
  contWait = 0;
	Serial.println("A   B   C   D   E   F");
	Serial.println("        X    ");
	Serial.println();
  while(band){
    if(alarmTemp() && alarmLuz()) stateMachine.SetState(Se, true, true);
    monitorAmbiental();
    botonHandler(SB);
    contTo(7, SF);
  }
}
void outputD(){
  contWait = 0;
  thisNote = 0;
  band = true;
	Serial.println("A   B   C   D   E   F");
	Serial.println("            X");
	Serial.println();
  while(band){
    contTo(10, SA);
    songBloq();
    blinkLed(ledRed, 500);
  }
}
void outputE(){
  contWait = 0;
  thisNoteAlarm = 0;
  band = true;
	Serial.println("A   B   C   D   E   F");
	Serial.println("                X");
	Serial.println();
  alarmLcd();
  while(band){
    contTo(4, SC);
    songAlarm();
    blinkLed(ledBlue, 800);
    botonHandler(SA);
  }
}
void outputF(){
  band = true;
  contWait = 0;
	Serial.println("A   B   C   D   E   F");
	Serial.println("                    X");
	Serial.println();
  while(band){
    if(alarmHall()) stateMachine.SetState(Se, true, true);
    monitorEventos();
    botonHandler(SB);
    contTo(5, SC);
  }
}