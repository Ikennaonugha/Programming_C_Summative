#include <LiquidCrystal.h>

const uint8_t ENTRY_BTN = 6, EXIT_BTN = 7, GREEN_LED = 8, RED_LED = 9;
const uint8_t MAX_CAPACITY = 10;
const unsigned long DEBOUNCE_MS = 50, LCD_MS = 250;

LiquidCrystal lcd(12, 11, 5, 4, 3, 2);   // RS, E, D4, D5, D6, D7

/** Occupancy information */
struct Occupancy {
  uint8_t current;      // people inside
  uint8_t maxCapacity;  // maximum allowed
};

struct Occupancy *occ = NULL;
unsigned long lastLcdMs = 0;

/** Allocates the record. Returns false if malloc fails. */
bool initOccupancy(struct Occupancy **o, uint8_t maxCap) {
  *o = (struct Occupancy *)calloc(1, sizeof(struct Occupancy));
  if (*o == NULL) return false;
  (*o)->maxCapacity = maxCap;
  return true;
}

/** Frees the record and clears the pointer. */
void releaseOccupancy(struct Occupancy **o) {
  free(*o);
  *o = NULL;
}

/** How many more people can enter. */
uint8_t availableCapacity(const struct Occupancy *o) {
  return o->maxCapacity - o->current;
}

/** Non-blocking debounce: true once per press (first falling edge). */
bool buttonPressed(uint8_t pin) {
  static int state[14] = {0};               // last accepted state per pin
  static unsigned long changed[14] = {0};   // time of last accepted change
  int reading = digitalRead(pin);

  if (millis() - changed[pin] < DEBOUNCE_MS || reading == state[pin]) return false;
  state[pin] = reading;
  changed[pin] = millis();
  return reading == LOW;
}

/** Entry attempt. Rejects when full. */
void handleEntry(struct Occupancy *o) {
  if (availableCapacity(o) == 0) {
    Serial.println(F("[ERROR] Entry rejected: facility is FULL."));
    return;
  }
  o->current++;
  Serial.println(F("[EVENT] Person entered."));
}

/** Exit attempt. Rejects when empty. */
void handleExit(struct Occupancy *o) {
  if (o->current == 0) {
    Serial.println(F("[ERROR] Exit rejected: facility is EMPTY."));
    return;
  }
  o->current--;
  Serial.println(F("[EVENT] Person left."));
}

/** Green = space available, red = full. */
void updateLeds(const struct Occupancy *o) {
  bool full = (availableCapacity(o) == 0);
  digitalWrite(GREEN_LED, !full);
  digitalWrite(RED_LED, full);
}

/** Shows occupancy on the LCD. */
void updateDisplay(const struct Occupancy *o) {
  char line[17];
  snprintf(line, sizeof(line), "In:%-3u Max:%-3u  ", o->current, o->maxCapacity);
  lcd.setCursor(0, 0);
  lcd.print(line);
  snprintf(line, sizeof(line), "Free:%-3u  %-6s", availableCapacity(o),
           availableCapacity(o) ? "OPEN" : "FULL");
  lcd.setCursor(0, 1);
  lcd.print(line);
}

/** Prints occupancy to the Serial Monitor. */
void printStatus(const struct Occupancy *o) {
  Serial.print(F("Inside: "));   Serial.print(o->current);
  Serial.print(F(" | Free: "));  Serial.print(availableCapacity(o));
  Serial.print(F(" | Max: "));   Serial.println(o->maxCapacity);
}

void setup() {
  Serial.begin(9600);
  pinMode(ENTRY_BTN, INPUT_PULLUP);
  pinMode(EXIT_BTN, INPUT_PULLUP);
  pinMode(GREEN_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  lcd.begin(16, 2);

  if (!initOccupancy(&occ, MAX_CAPACITY)) {
    Serial.println(F("[FATAL] Memory allocation failed."));
    while (true) { }
  }
  updateLeds(occ);
  updateDisplay(occ);
  printStatus(occ);
}

void loop() {
  if (occ == NULL) return;   // shut down

  bool entry = buttonPressed(ENTRY_BTN);
  bool exit_ = buttonPressed(EXIT_BTN);
  if (entry) handleEntry(occ);
  if (exit_) handleExit(occ);
  if (entry || exit_) {
    updateLeds(occ);
    printStatus(occ);
  }

  if (millis() - lastLcdMs >= LCD_MS) {     // periodic LCD refresh
    lastLcdMs = millis();
    updateDisplay(occ);
  }

  if (Serial.available() && Serial.read() == 'q') {   // 'q' = free memory
    releaseOccupancy(&occ);
    Serial.println(F("Memory released. System stopped."));
  }
}
