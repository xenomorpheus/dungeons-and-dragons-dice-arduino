/*
 *  DnD Dice. Mike Bruins, 2016.
 *
 * Features:
 *  Dice styles d2,d3,d4,d6,d8,d10,d12,d20,d100
 *  Can do 5d6 for example.
 *  Can take best 3d6 from 5 attempts.
 *  Can choose to re-roll ones, or not.
 *
 * Bugs:
 *  The seeding of the random number generator doesn't work.
 *
 * Display:
 *
 * "XX Dddd Aaa Rr"      - Line 1
 *
 *  01234567890123       - Character offset
 *
 * Legend:
 *   XX    - Rolls to count (may be the best of several attempts)
 *   Dddd  - Sides on the dice.
 *   Aaa   - Attempts at rolling.  If non-zero then this is the number of rolls.
 *                           (best 3 rolls in 5 attempts)
 *   Rr    - Re-Roll any 1s.
 *
 */

#define VERSION "V1.0 Mike Bruins"
#include <LiquidCrystal.h>
#include <Stdio.h>

//create object to control an LCD.
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);           // select the pins used on the LCD panel

// keyboard
#define KEY_RIGHT   0
#define KEY_UP      1
#define KEY_DOWN    2
#define KEY_LEFT    3
#define KEY_SELECT  4
#define KEY_NONE    5

// sequence starting at 0
#define DICE02 0
#define DICE03 1
#define DICE04 2
#define DICE06 3
#define DICE08 4
#define DICE10 5
#define DICE12 6
#define DICE20 7
#define DICE100 8
// DICE_TYPE_MAX is the number of last dice
#define DIE_TYPE_MAX DICE100
#define DIE_ROLL_MAX 30
// If we are taking the best rolls from some attempts.
#define DIE_ATTEMPT_MAX 100
int roll_attempt[DIE_ATTEMPT_MAX + 1];

// The UI cursor.
// Sequence starting at 0
#define CURSOR_DIE_ROLL_COUNT  0
#define CURSOR_DIE_TYPE        1
#define CURSOR_DIE_BEST_OF     2
#define CURSOR_DIE_REROLL_ONES 3
#define CURSOR_MAX CURSOR_DIE_REROLL_ONES
int cursor = 0;
int cursor_offset[CURSOR_MAX + 1] = { 1, 6, 10, 13 };

// Properties of the die(s)
int die_type_sides[DIE_TYPE_MAX + 1] = { 2, 3, 4, 6, 8, 10, 12, 20, 100 };
int die_type = DICE20; // Default is a D20.
int die_rolls = 1; // Default is 1 roll.
int die_reroll_ones = 0; // Default is NOT to re-roll ones
int die_best_of_rolls_attempts = 0; // Default is not to have extra attempts and take the best rolls.
long die_result = -1;

void setup() {
	pinMode(13, OUTPUT); //we'll use the debug LED to output a heart-beat

	lcd.begin(16,2);
        lcd.cursor();
        lcd.blink();
	randomSeed(analogRead(1)); // Don't use 0 as keys are on that line.
	show_die();
}
/*
 * Show the current status
 */
void show_die() {
	char line1[17];
	char line2[17];
	lcd.clear();
        lcd.setCursor(0, 0); // top left
	sprintf(line1, "%2d D%3d A%2d R%1d", die_rolls, die_type_sides[die_type],
			die_best_of_rolls_attempts, die_reroll_ones);
	lcd.print(line1);
        lcd.setCursor(0, 1); // bottom left
	if (die_result == -1) {
		lcd.print(VERSION);
	} else if (die_result) {
		sprintf(line2, "%ld  ", die_result);
		lcd.print(line2);
	}
	lcd.setCursor(cursor_offset[cursor], 0);
}
/*
 * Do the dice rolls
 */
void roll_dice() {

	int sides = die_type_sides[die_type];
	int die_rolls_with_attempts = die_rolls;
	int roll_count;

	// Best of several attempts
	if (die_best_of_rolls_attempts > die_rolls_with_attempts)
		die_rolls_with_attempts = die_best_of_rolls_attempts;

	// zero the result before we start
	die_result = 0;

	// Each loop is a die roll
	for (roll_count = 0; roll_count < die_rolls_with_attempts; roll_count++) {
		// Random between 1 and the number of sides.

		// Re-roll ones if told to do so.
		// Note we don't enable this for a d1 :-)
		int roll;
		do {
			roll = random(1, 1 + sides);
		} while (sides != 1 && die_reroll_ones && (roll == 1));

		// If we are doing a straight number of dice rolls, not extra attempts.
		die_result += roll;

		// If we are doing extra attempts.
		roll_attempt[roll_count] = roll;
	}

	// if no extra attempts then the die_result is already the total.
	if (die_best_of_rolls_attempts <= die_rolls)
		return;

	// zero the result before we start
	die_result = 0;

	//  Okay now we have to work harder. Find the maximum rolls in the pool.
	int roll_pool_id;
	for (roll_count = 0; roll_count < die_rolls; roll_count++) {

		// We grab the first value as the trial maximum.
		int max_value_id = 0;
		int max_value = roll_attempt[max_value_id];
		for (roll_pool_id = 1; roll_pool_id < die_rolls_with_attempts; roll_pool_id++) {

			// If the roll is greater than our current max, then set that as the max roll in pool.
			if (roll_attempt[roll_pool_id] > max_value) {
				max_value_id = roll_pool_id;
				max_value = roll_attempt[max_value_id];
			}
		}

		// Add on the maximum roll found in the pool.
		die_result += roll_attempt[max_value_id];

		// Clear that roll from the pool so it can't be chosen again.
		roll_attempt[max_value_id] = 0;

	}
	// die_result will be holding the total.
}

int read_LCD_buttons(int sensor){               // read the buttons
    int adc_key_in = analogRead(sensor);       // read the value from the sensor

    // my buttons when read are centered at these valies: 0, 144, 329, 504, 741
    // we add approx 50 to those values and check to see if we are close
    // We make this the 1st option for speed reasons since it will be the most likely result

    if (adc_key_in > 1000) return KEY_NONE;

    // For V1.1 use this threshold
   /*
    if (adc_key_in < 50)   return KEY_RIGHT;
    if (adc_key_in < 250)  return KEY_UP;
    if (adc_key_in < 450)  return KEY_DOWN;
    if (adc_key_in < 650)  return KEY_LEFT;
    if (adc_key_in < 850)  return KEY_SELECT;
   */

   // For V1.0 comment the other threshold and use the one below:
     if (adc_key_in < 60)   return KEY_RIGHT;
     if (adc_key_in < 205)  return KEY_UP;
     if (adc_key_in < 400)  return KEY_DOWN;
     if (adc_key_in < 600)  return KEY_LEFT;
     if (adc_key_in < 800)  return KEY_SELECT;

    return KEY_NONE;                // when all others fail, return this.
}


/*
 * Main loop.
 * Read the keyboard and output the state of the die.
 */

void loop() {
	//Key message
	static int oldkey = -1;
	int key = read_LCD_buttons(0); // read the value from the sensor
	if (key != oldkey) // if keypress is detected
	{
		delay(50); // wait for debounce time
                key = read_LCD_buttons(0); // read the value from the sensor
                oldkey = key;

		// cursor left
		if (key == KEY_LEFT) {
			if (cursor > 0)
				cursor--;
		}
		// cursor right
		else if (key == KEY_RIGHT) {

			if (cursor < CURSOR_MAX)
				cursor++;
		}

		// increment value under cursor
		else if (key == KEY_UP) {

			if (cursor == CURSOR_DIE_ROLL_COUNT) {

				// Rolls add 1
				if (die_rolls < DIE_ROLL_MAX)
					die_rolls++;

				// Sanity check the die_best_of_rolls_attempts
				if (die_rolls >= die_best_of_rolls_attempts)
					die_best_of_rolls_attempts = 0;
			} else if (cursor == CURSOR_DIE_TYPE) {
				// Dice type add 1
				if (die_type < DIE_TYPE_MAX)
					die_type++;
				// switch off "best of" when changing die type
				die_best_of_rolls_attempts = 0;
				// switch off "re-roll ones" when changing die type
				die_reroll_ones = 0;

			}

			else if (cursor == CURSOR_DIE_BEST_OF) {

				// Best-Of Rolls add 1
				die_best_of_rolls_attempts++;

				// If set, we must be greater than die_rolls.
				if (die_best_of_rolls_attempts <= die_rolls)
					die_best_of_rolls_attempts = die_rolls + 1;

			} else if (cursor == CURSOR_DIE_REROLL_ONES) {
				die_reroll_ones = 1;
			}

		}
		// decrement value under cursor
		else if (key == KEY_DOWN) {

			if (cursor == CURSOR_DIE_ROLL_COUNT) {

				// Rolls minus 1
				if (die_rolls > 1)
					die_rolls--;

				// Sanity check the die_best_of_rolls_attempts
				if (die_best_of_rolls_attempts > die_rolls)
					die_best_of_rolls_attempts = die_rolls;

			} else if (cursor == CURSOR_DIE_TYPE) {
				// Dice type minus 1
				if (die_type > 0)
					die_type--;
				// switch off "best of" when changing die type
				die_best_of_rolls_attempts = 0;
				// switch off "reroll ones" when changing die type
				die_reroll_ones = 0;

			}

			else if (cursor == CURSOR_DIE_BEST_OF) {
				// Best-Of Rolls minus 1
				if (die_best_of_rolls_attempts > 0)
					die_best_of_rolls_attempts--;

				// We drop attempts if it is less than or equal to normal rolls.
				if (die_best_of_rolls_attempts <= die_rolls)
					die_best_of_rolls_attempts = 0;

			} else if (cursor == CURSOR_DIE_REROLL_ONES) {
				die_reroll_ones = 0;
			}

		}

		// Roll Dice
		else if (key == KEY_SELECT) {
			// die_result set by side-effect
			roll_dice();

		}

		if (key >= 0) {
			show_die();
		}
	}

	//delay(1000);
	digitalWrite(13, LOW);

}
