#include "AsyncTaskLib.h"

#include <StateMachineLib.h>
#include <LiquidCrystal.h>
#include <Keypad.h>

String pass = "1234";
String contra = "";
int cont = 0;
int contE = 0;
bool seguridadB = true;
const int ledRed = 10;
const int ledGreen = 9;
const int ledBlue = 8;
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
//Funciones asincronas

AsyncTask aTBlink500H(500, []() { digitalWrite(LED_BUILTIN, HIGH); });
AsyncTask aTBlink500L(500, []() { digitalWrite(LED_BUILTIN, LOW); });

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
  hall = 3,//HALL
  temp = 4,//TEMP
  luz = 5,//LUZ
  boton = 6,
  unknown = 7
};

// Create new StateMachine
StateMachine stateMachine(6, 10);

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
	stateMachine.SetOnLeaving(SD, []() {aTBlink500H.Update(); aTBlink500L.Update();});
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

int readInput()
{
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
      delay(2000);
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
    digitalWrite(ledGreen, 255);
    input = Input::claveCorrecta;
    cont = 0;
    seguridadB=false;
  }else if(cont > 1){
    Serial.println("Bloqueado");
    lcd.print("Bloqueado");
    digitalWrite(ledRed, 255);
    input = Input::sisBloqueado;
    cont = 0;
    seguridadB=false;
  }else{
    Serial.println("Incorrecta");
    lcd.print("Incorrecta");
    digitalWrite(ledBlue, 255);
    cont++;
  }
  contE=0;
}
void outputA()
{
	Serial.println("A   B   C   D   E   F");
	Serial.println("X            ");
	Serial.println();
  seguridad();
}

void outputB()
{
	Serial.println("A   B   C   D   E   F");
	Serial.println("    X                ");
	Serial.println();
}

void outputC()
{
	Serial.println("A   B   C   D   E   F");
	Serial.println("        X    ");
	Serial.println();
}

void outputD()
{
	Serial.println("A   B   C   D   E   F");
	Serial.println("            X");
	Serial.println();
  aTBlink500H.Start();
}
void outputE()
{
	Serial.println("A   B   C   D   E   F");
	Serial.println("                X");
	Serial.println();
}
void outputF()
{
	Serial.println("A   B   C   D   E   F");
	Serial.println("                    X");
	Serial.println();
}

void blink(int inter){
  
}
