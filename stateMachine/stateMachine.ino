#include <StateMachineLib.h>
#include <LiquidCrystal.h>
#include <Keypad.h>
#include <dht.h>

dht DHT;

String pass = "1234";
String contra = "";

int cont = 0;
int contE = 0;//Contador para escribir
int contWait = 0;//Contador para esperar

bool seguridadB = true;

//Song
bool band = true;
#define NOTE_D4  294//
#define NOTE_E4  330//
#define NOTE_G4  392//
#define NOTE_GS4 415//
#define NOTE_A4  440//
#define NOTE_AS4 466//
#define NOTE_B4  494//
#define NOTE_C5  523//
#define REST      0
int melody[] = {
  //game over sound
  NOTE_C5,-4, NOTE_G4,-4, NOTE_E4,4, //45
  NOTE_A4,-8, NOTE_B4,-8, NOTE_A4,-8, NOTE_GS4,-8, NOTE_AS4,-8, NOTE_GS4,-8,
  NOTE_G4,8, NOTE_D4,8, NOTE_E4,-2,  
};
int tempo = 200;
int notes = sizeof(melody) / sizeof(melody[0]) / 2;
int wholenote = (60000 * 4) / tempo;
int buzzer = 7; // Pin del buzzer

int thisNote = 0;
unsigned long noteDuration = 0;
unsigned long previousMillis = 0;
bool isPlaying = false;

unsigned long previousLedMillis = 0;
unsigned long prevAmb = 0;
unsigned long prevEv = 0;
unsigned long prev = 0;
//Led parpadeo
unsigned long startMillis = 0;
const long interval = 500; // Intervalo de parpadeo del LED en milisegundos
bool ledState = false; // Estado actual del LED

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
void setupStateMachine()
{
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

void setup() 
{
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

void loop() 
{
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
  contE=0;
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
    cont++;
  }
}
void song(){
  unsigned long currentMillis = millis();
  if (thisNote < notes * 2) {
    if (!isPlaying) {
      int divider = melody[thisNote + 1];
      if (divider > 0) {
        noteDuration = (wholenote) / divider;
      } else if (divider < 0) {
        noteDuration = (wholenote) / abs(divider);
        noteDuration *= 1.5;
      }
      
      tone(buzzer, melody[thisNote], noteDuration * 0.9);
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
void blinkLed() {
  unsigned long currentMillis = millis();

  if (currentMillis - previousLedMillis >= interval) {
    previousLedMillis = currentMillis;
    // Si el LED está apagado, encenderlo y viceversa
    if (ledState) {
      digitalWrite(ledRed, LOW);
    } else {
      digitalWrite(ledRed, HIGH);
    }
    // Cambiar el estado del LED
    ledState = !ledState;
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
void botonHandler(){
  delay(100);
  if(digitalRead(btn)==0){
    band=false;
    Serial.println(digitalRead(btn));
  }
}
void menuConfig(){
  digitalWrite(ledRed, 0);
  digitalWrite(ledGreen, 0);
  digitalWrite(ledBlue, 0);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Config--B");
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
void outputA()
{
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
  menuConfig(); //TODO
  while(band){
    botonHandler();
    if(!band){
      stateMachine.SetState(SC, true, true);
    }
  }
}
void outputC(){
  band = true;
  contWait = 0;
	Serial.println("A   B   C   D   E   F");
	Serial.println("        X    ");
	Serial.println();
  while(band){
    if(!band){
      stateMachine.SetState(SB, true, true);
    }
    monitorAmbiental();
    botonHandler();
    contTo(7, SF);
  }
}
void outputD(){
  contWait = 0;
  thisNote = 0;
	Serial.println("A   B   C   D   E   F");
	Serial.println("            X");
	Serial.println();
  while(band){
    contTo(10, SA);
    song();
    blinkLed();
  }
}
void outputE(){
	Serial.println("A   B   C   D   E   F");
	Serial.println("                X");
	Serial.println();
}
void outputF(){
  band = true;
  contWait = 0;
	Serial.println("A   B   C   D   E   F");
	Serial.println("                    X");
	Serial.println();
  while(band){
    if(!band){
      stateMachine.SetState(SB, true, true);
    }
    monitorEventos();
    botonHandler();
    contTo(3, SC);
  }
}