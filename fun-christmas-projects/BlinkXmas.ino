/*
 * turn 5V power on/off using P-chan mosfet to blink Xmas tree leds . Note: use a BJT as driver so that
 * signal is not inverted. Without BJT, a digitalWrite HIGH will turn FET off.
 * 
 *                 5V
 *               +-----+
 *               |     |
 *              10k    |
 *               |     S
 *               +---G  (Si2301)
 *               |     |D
 *               |C    |
  pin 5 - 1K - B      load
          (BC548)|E    |
                 |     |
                Gnd   Gnd

 If signal inversion is not an issue, connect directly:
 
 *                 5V
 *               +-----+
 *               |     |
 *              10k    |
 *               |     S
 * pin 5 -- 1K --+---G  (Si2301)
 *                     |D
 *                     |
                     load
                       |
                      Gnd
 */

#define pwrpin 5
void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(pwrpin, OUTPUT);
  Serial.begin(115200);
  digitalWrite(pwrpin, LOW);         // init led off
  digitalWrite(LED_BUILTIN, LOW);    // init pwr off
}

// the loop function runs over and over again forever
void loop() {
  //digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on (HIGH is the voltage level)
  digitalWrite(pwrpin, HIGH);        // turn pwr on
  delay(1000);                       // wait for a second
  //digitalWrite(LED_BUILTIN, LOW);    // turn the LED off by making the voltage LOW
  digitalWrite(pwrpin, LOW);         // turn pwr off
  delay(1000);                       // wait for a second
}
