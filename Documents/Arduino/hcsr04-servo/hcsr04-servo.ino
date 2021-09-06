    /*
    * Ultrasonic Sensor HC-SR04 and Arduino Tutorial
    *
    * by Dejan Nedelkovski,
    * www.HowToMechatronics.com
    *
    */
    #include <Servo.h> 
    // Declare the Servo pin 
    int servoPin = 3; 
    // Create a servo object 
    Servo Servo1; 
    // defines pins numbers
    const int trigPin = 9;
    const int echoPin = 10;
    // defines variables
    long duration;
    int distance;
    void setup() {
      // We need to attach the servo to the used pin number 
      Servo1.attach(servoPin); 
      pinMode(trigPin, OUTPUT); // Sets the trigPin as an Output
      pinMode(echoPin, INPUT); // Sets the echoPin as an Input
      Serial.begin(9600); // Starts the serial communication
    }
    void loop() {
    // Clears the trigPin
    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);
    // Sets the trigPin on HIGH state for 10 micro seconds
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);
    // Reads the echoPin, returns the sound wave travel time in microseconds
    duration = pulseIn(echoPin, HIGH);
    // Calculating the distance
    distance= duration*0.034/2;
    // Prints the distance on the Serial Monitor
    Serial.print("Distance: ");
    Serial.println(distance);

    if(distance<=10){
         Servo1.write(0); 
         delay(1000); 
         // Make servo go to 90 degrees 
         Servo1.write(90); 
         delay(10000); 
      }
    
    }
