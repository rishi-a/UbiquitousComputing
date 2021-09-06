    // Pin D7 has an LED connected on FLORA.
    // give it a name:
    #include <Adafruit_NeoPixel.h>
 
    #define PIN 8
     
    Adafruit_NeoPixel strip = Adafruit_NeoPixel(1, PIN, NEO_GRB + NEO_KHZ800);
     int led = 7;
    void setup() {
      strip.begin();
      strip.setBrightness(0);
      strip.show(); // Initialize all pixels to 'off'
      pinMode(led, OUTPUT); 
      digitalWrite(8, LOW);  
    }
    
    
    // the loop routine runs over and over again forever:
    void loop() {
      digitalWrite(led, HIGH);   // turn the LED on (HIGH is the voltage level)
      delay(1000);               // wait for a second
      digitalWrite(led, LOW);    // turn the LED off by making the voltage LOW
      delay(1000);               // wait for a second
    }
