#include <PinChangeInterrupt.h>
#include <PinChangeInterruptBoards.h>
#include <PinChangeInterruptPins.h>
#include <PinChangeInterruptSettings.h>

#include <Wire.h>
#include <hd44780.h>                       
#include <hd44780ioClass/hd44780_I2Cexp.h>


hd44780_I2Cexp lcd; 



const int debounceDelay = 120;



const int buttonLeft = 4;
const int buttonRight = 3;
const int buttonDown = 5;
const int buttonUp = 6;
const int buttonSelect = 7;
const int buzzer = 2;

volatile long lastButtonUpPressed = 0;
volatile long lastButtonDownPressed = 0;

int cursorRow;
int cursorColumn;

int characterRow;
int characterColumn;
int scored = 0;

int appleRow;
int appleColumn;

int snakeHeadRow;
int snakeHeadColumn;
int deleteRow;
int deleteColumn;

int snakeLength;

byte endlessRunnerCharacter[8] = {
	0b00100,
	0b01010,
	0b00100,
	0b11111,
	0b00100,
	0b00100,
	0b01010,
	0b10001
};

byte endlessRunnerColumn[8] = {
	0b11111,
	0b10101,
	0b10101,
	0b10101,
	0b10101,
	0b10101,
	0b10101,
	0b11111
};

byte snakeApple[8] = {
	0b00011,
	0b00100,
	0b01010,
	0b10001,
	0b10001,
	0b10001,
	0b10001,
	0b01110
};

byte snake[8] = {
	0b11111,
	0b11111,
	0b11111,
	0b11111,
	0b11111,
	0b11111,
	0b11111,
	0b11111
};

byte snakeHead[8] = {
	0b11111,
	0b11111,
	0b10101,
	0b10101,
	0b11111,
	0b11111,
	0b11111,
	0b11111
};

enum DIRECTIONS {
  UP,
  DOWN,
  LEFT,
  RIGHT
};

DIRECTIONS snakeDirection;

DIRECTIONS movement[4][20];

int snakePlacement[4][20];

enum MODES {
  MAIN_MENU,
  SNAKE,
  ENDLESS_RUNNER,
  END_GAME
};

volatile MODES current_mode;

void play_end_game_sound() {
  tone(buzzer, 500, 1000); 

  delay(1000);

  tone(buzzer, 400, 1000); 

  delay(1000);

  tone(buzzer, 300, 1000); 
  
  delay(1000);
}

// time that has elapsed sinced refreshing the screen
int delayMs;
// total time that has elapsed since starting the game
int totalTime;
// the lower the number, the faster the game
int difficulty;
int score;

int columnSetups[4] = { 10111, 11011, 11101, 11110};

struct column {
  int x;
  int rows[4];
};

column columns[3];



void pressButtonUp() {
  if (current_mode == MAIN_MENU && cursorRow > 2) {
    cursorRow--;
  } else if (current_mode == SNAKE && snakeDirection != DOWN) {
    snakeDirection = UP;
  } else if (current_mode == ENDLESS_RUNNER && characterRow > 0) {

    unsigned long currentTime = millis();

    // debounce
    if ((currentTime - lastButtonUpPressed) > debounceDelay) {
        characterRow--;
        lastButtonUpPressed  = currentTime;
    }
    
  }
}

void pressButtonDown() {
  if (current_mode == MAIN_MENU && cursorRow < 3) {
      cursorRow++;
  } else if (current_mode == SNAKE && snakeDirection != UP) {
    snakeDirection = DOWN;
  } else if (current_mode == ENDLESS_RUNNER && characterRow < 3) {

    unsigned long currentTime = millis();

    // debounce
    if ((currentTime - lastButtonDownPressed) > debounceDelay) {
        characterRow++;
        lastButtonDownPressed  = currentTime;
    }
    
  }
}

void pressButtonLeft() {
  if (snakeDirection != RIGHT) {
    snakeDirection = LEFT;
  }
}

void pressButtonRight() {
  if (snakeDirection != LEFT) {
    snakeDirection = RIGHT;
  }
}




void setup() {
  lcd.begin(20, 4);





  pinMode(buttonUp, INPUT_PULLUP);
  pinMode(buttonDown, INPUT_PULLUP);
  pinMode(buttonLeft, INPUT_PULLUP);
  pinMode(buttonRight, INPUT_PULLUP);
  pinMode(buttonSelect, INPUT_PULLUP);
  pinMode(buzzer, OUTPUT);

  attachPCINT(digitalPinToPCINT(buttonRight), pressButtonRight, CHANGE);
  attachPCINT(digitalPinToPCINT(buttonLeft), pressButtonLeft, CHANGE);
  attachPCINT(digitalPinToPCINT(buttonUp), pressButtonUp, CHANGE);
  attachPCINT(digitalPinToPCINT(buttonDown), pressButtonDown, CHANGE);



  

  cursorRow = 2;
  cursorColumn = 0;      

  characterRow = 2;
  characterColumn = 3;

  score = 0;



  current_mode = MAIN_MENU;


  // ---- ENDLESS RUNNER ------

  // create the characters for the endless runner game
  lcd.createChar(0, endlessRunnerCharacter);
  lcd.createChar(1, endlessRunnerColumn);
  lcd.createChar(2, snakeApple);
  lcd.createChar(3, snake);
  lcd.createChar(4, snakeHead);

  
  difficulty = 20000;
  delayMs = difficulty;
  totalTime = 0;
  
}

void generateRandomColumn(int rows[]) {
  int randomIndex = random(0, 4);
  int number = columnSetups[randomIndex];


  for (int i = 0; i < 4; i++) {
    rows[i] = number % 10;
    number /= 10;
  }

}





void loop() {




  if (current_mode == MAIN_MENU) {
    
    lcd.setCursor(6, 0);            
    lcd.print("WELCOME! "); 
    lcd.setCursor(2, 1);            
    lcd.print("SELECT YOUR GAME:");          
    lcd.setCursor(0, 2);            
    lcd.print("       SNAKE");          
    lcd.setCursor(0, 3);            
    lcd.print("   ENDLESS RUNNER");

    lcd.setCursor(cursorColumn, cursorRow);
    lcd.print(">");

    if (digitalRead(buttonSelect) == LOW)
    {

      if (cursorRow == 2) {
        lcd.clear();
        // randomize the apple coordinates
        do {
        appleRow = random(0,4);
        appleColumn = random(0,20);
        } while (snakePlacement[appleRow][appleColumn] != 0);

        // reset the coordinated of the snake's head
        snakeHeadRow = 2;
        snakeHeadColumn = 5;

        // reset the coordinates for deletion
        deleteRow = snakeHeadRow;
        deleteColumn = snakeHeadColumn;

        for (int i = 0; i < 4; i++) {
          for (int j = 0; j < 20; j++) {
            snakePlacement[i][j] = 0;
          }
        }

        snakePlacement[snakeHeadRow][snakeHeadColumn] = 1;
        movement[snakeHeadRow][snakeHeadColumn] = RIGHT;

        // reset the snake's length
        snakeLength = 1;
        snakeDirection = RIGHT;

        difficulty = 20000;
        delayMs = difficulty;
        totalTime = 0;

        current_mode = SNAKE;


        // reset the score
        score = 0;
        delay(120);
      }
      else {
        lcd.clear();
        current_mode = ENDLESS_RUNNER;
        // set the columns
        for (int i = 0; i < 3; i++) {
          generateRandomColumn(columns[i].rows);
          columns[i].x = 12 + (7 * i);
        }

        // reset the difficulty
        difficulty = 20000;
        delayMs = difficulty;
        totalTime = 0;

        // reset the character's position
        characterRow = 2;
        characterColumn = 3;

        // reset the score
        score = 0;
        delay(100);
      }
    }
  }

  if (current_mode == SNAKE) {

    if (delayMs / difficulty > 0) {

      delayMs = 0;
      lcd.clear();



      switch (snakeDirection) {
          case UP:
              movement[snakeHeadRow][snakeHeadColumn] = UP;
              snakeHeadRow--;

              // the snake eats itself
              if (snakePlacement[snakeHeadRow][snakeHeadColumn] == 1) {
                lcd.clear();
                current_mode = END_GAME;
              }

              snakePlacement[snakeHeadRow][snakeHeadColumn] = 1;
              break; 
          case DOWN:
              movement[snakeHeadRow][snakeHeadColumn] = DOWN;
              snakeHeadRow++;

              // the snake eats itself
              if (snakePlacement[snakeHeadRow][snakeHeadColumn] == 1) {
                lcd.clear();
                current_mode = END_GAME;
              }

              snakePlacement[snakeHeadRow][snakeHeadColumn] = 1;
              break; 
          case LEFT:
              movement[snakeHeadRow][snakeHeadColumn] = LEFT;
              snakeHeadColumn--; 

              // the snake eats itself
              if (snakePlacement[snakeHeadRow][snakeHeadColumn] == 1) {
                lcd.clear();
                current_mode = END_GAME;
              }

              snakePlacement[snakeHeadRow][snakeHeadColumn] = 1;
              break; 
          case RIGHT:
              movement[snakeHeadRow][snakeHeadColumn] = RIGHT;
              snakeHeadColumn++; 

              // the snake eats itself
              if (snakePlacement[snakeHeadRow][snakeHeadColumn] == 1) {
                lcd.clear();
                current_mode = END_GAME;
              }

              snakePlacement[snakeHeadRow][snakeHeadColumn] = 1;
              break; 
          default: 
              break;        
      }




      // the snake eats an apple
      if (snakeHeadRow == appleRow && snakeHeadColumn == appleColumn) {
        tone(buzzer, 850, 200);
        // randomize the apple coordinates
        do {
        appleRow = random(0,4);
        appleColumn = random(0,20);
        } while (snakePlacement[appleRow][appleColumn] != 0);
        snakeLength++;

        // increase snake's length 
        switch (movement[deleteRow][deleteColumn]) {
          case UP:
              deleteRow++;
              snakePlacement[deleteRow][deleteColumn] = 1;
              break; 
          case DOWN:
              deleteRow--;
              snakePlacement[snakeHeadRow][snakeHeadColumn] = 1;
              break; 
          case LEFT:
              deleteColumn++;
              snakePlacement[deleteRow][deleteColumn] = 1;
              break; 
          case RIGHT:
              deleteColumn--;
              snakePlacement[deleteRow][deleteColumn] = 1;
              break; 
          default: 
              break;        
        }


        score++;
      }

      // delete the last cell to create a movement effect
      snakePlacement[deleteRow][deleteColumn] =  0;



      // shift the cell to be deleted based on the snake's movement
      switch (movement[deleteRow][deleteColumn]) {
            case UP:
                deleteRow--;
                break; 
            case DOWN:
                deleteRow++;
                break; 
            case LEFT:
                deleteColumn--;
                break; 
            case RIGHT:
                deleteColumn++;
                break; 
            default: 
                break;        
        }

      


      // render the snake
      for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 20; j++) {
          if (snakePlacement[i][j] == 1) {
            lcd.setCursor(j, i);
            if (i == snakeHeadRow && j == snakeHeadColumn) {
              lcd.write((uint8_t)4);
            } else {
              lcd.write((uint8_t)3);
            }
            
          }
        }
      }






      // render the apple
      lcd.setCursor(appleColumn, appleRow);
      lcd.write((uint8_t)2);

      // the snake hits the border
      if (snakeHeadRow < 0 || snakeHeadRow > 3 || snakeHeadColumn < 0 || snakeHeadColumn > 19) {
        current_mode = END_GAME;
      }


    }

    if (digitalRead(buttonSelect) == LOW)
    {
      lcd.clear();
      current_mode = MAIN_MENU;
      delay(100);
    }




    delayMs++;
    totalTime++;


    // increase difficulty as the time passes
    if (totalTime > 50000) {

      if (difficulty >= 16000) {
        difficulty -= 1000;
      }

      totalTime = 0;
    }

  }

  if (current_mode == ENDLESS_RUNNER) {



    if (delayMs / difficulty > 0) {



      if (scored > 0) {
        tone(buzzer, 850, 200);
        scored = 0;
      }

      delayMs = 0;
      lcd.clear();

      lcd.setCursor(characterColumn,characterRow);

      lcd.write((uint8_t)0);


      for (int i = 0; i < 3; i++) {
        if (columns[i].x <= 19) {
          for (int j =0; j < 4; j++) {
            if (columns[i].rows[j] == 1) {
              lcd.setCursor(columns[i].x,j);

              lcd.write((uint8_t)1);

              // you lose
              if (characterColumn == columns[i].x && characterRow == j) {
                current_mode = END_GAME;
              }
            }
          }
        }
      }

      for (int i = 0; i < 3; i++) {
        if (columns[i].x > -1) {
          columns[i].x--;
        }
        if (columns[i].x < 0) {
          
          columns[i].x = 19;
          generateRandomColumn(columns[i].rows);
          score++;
          scored++;
          
        }
      }


    }

    if (digitalRead(buttonSelect) == LOW)
    {
      lcd.clear();
      current_mode = MAIN_MENU;
      delay(100);
    }



    delayMs++;
    totalTime++;


    // increase difficulty as the time passes
    if (totalTime > 50000) {

      if (difficulty >= 16000) {
        difficulty -= 1000;
      }

      totalTime = 0;
    }
  }

  if (current_mode == END_GAME) {

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("     GAME OVER!");
    lcd.setCursor(0,1);
    lcd.print("   YOUR SCORE IS:");
    lcd.setCursor(9, 2);
    lcd.print(String(score));
    lcd.setCursor(0,3);
    lcd.print("  CONGRATULATIONS!");

    

    play_end_game_sound();


    // go back to the main menu
    lcd.clear();
    current_mode = MAIN_MENU;
    delay(120);
  }

}