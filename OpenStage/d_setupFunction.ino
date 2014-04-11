
void setup() {

 
  pinMode(13,OUTPUT); //PIN 13 is optionally used for testing and debugging with an osciliscope
  pinMode(beepPin,OUTPUT); //Produce sound on this pin

  // Set up the requred AccelStepper instances (one per axis)
  if (axisPresent[0]){
    AccelStepper stepperX(1, stepOut[0], stepDir[0]); 
    mySteppers[0] = &stepperX;
  }
  if (axisPresent[1]){
    AccelStepper stepperY(1, stepOut[1], stepDir[1]); 
    mySteppers[1] = &stepperY;
  }
  if (axisPresent[2]){
    AccelStepper stepperZ(1, stepOut[2], stepDir[2]); 
    mySteppers[2] = &stepperZ;
  }
  if (axisPresent[3]){
    AccelStepper stepperA(1, stepOut[3], stepDir[3]); 
    mySteppers[3] = &stepperA;
  }


  // Connect to the PC's serial port via the Arduino programming USB connection. 
  // This used mainly for printing de-bugging information to the PC's serial terminal
  // during testing. [In future we will add the option for controlling the stage through
  // the USB port]
  Serial.begin(115200); //This is a bit horrible
  if (doSerialInterface){
    if (!controlViaUSB){
       HARDWARE_SERIAL_PORT.begin(115200);
       SerialComms = &HARDWARE_SERIAL_PORT;
     } else {
       SerialComms = &HARDWARE_SERIAL_PORT;
     }
  } //if doSerialInterface


  //Set the micro-step pins as outputs
  pinMode(MS1,OUTPUT);
  pinMode(MS2,OUTPUT);
  pinMode(MS3,OUTPUT);  


  //Set up motor control pins, LED pins, etc, as output lines
  for (byte ii=0; ii<numAxes; ii++){
    if (!axisPresent[ii]) //Skip axes that aren't present 
      continue;
    
    pinMode(stepDir[ii],OUTPUT); 

    pinMode(stepOut[ii],OUTPUT); 
    digitalWrite(stepOut[ii],LOW);

    pinMode(enable[ii],OUTPUT);
    digitalWrite(enable[ii],LOW);//power to motors
  } 

  for (byte ii=0; ii<4; ii++){
    pinMode(stageLEDs[ii],OUTPUT);
    digitalWrite(stageLEDs[ii],LOW);
  }


  //Initialise the 20 by 4 LCD display 
  #ifdef DO_LCD
   lcd.begin(20,4);               
   lcd.home ();                   
   lcd.clear();
  #endif

  // Connect to the USB Shield
  #ifdef DO_GAMEPDAD
    if (Usb.Init() == -1) {
      Serial.print(F("\r\nConnection to USB shield failed"));
    
      //halt and flash warning
      #ifdef DO_LCD
      while(1){ //infinite while loop
       lcd.setCursor (0, 1);   
       lcd.print (" No USB connection!");
       delay(1000);
       lcd.clear();
       delay(1000);
      }// while
      #endif  
    }


     //Pre-calculate the speeds for different hat-stick values. This moves these
     //calculations out of the main loop, and allows for smoother closed-loop hat-stick motions.
     for (byte ii=0; ii<128; ii++){
       for (byte jj=0; jj<4; jj++){         
          //SPEEDMAT[ii][jj]=(ii/127.5)*maxSpeed[jj]; //Plain linear
          SPEEDMAT[ii][jj]=fscale(hatStickThresh, 127.5, 0.04, maxSpeed[jj], ii, curve[jj]); //non-linear mapping
       }  
     }

  #endif
  

  //Display boot message on LCD screen  
  #ifdef DO_LCD
   lcd.setCursor (0,0);   
   lcd.print ("Booting OpenStage");
   lcd.setCursor (0,1);   
  #endif



  //the variable thisStep can take on one of four values for each axis
  //Calculate all of these here
  for (byte ii=0; ii<numAxes; ii++){
    for (byte jj=0; jj<4; jj++){
      if (axisPresent[ii]){
           thisStep[ii][jj] = (fullStep[ii]/360) * stepSize[jj] * gearRatio[ii];
      } //if axisPresent
    } // jj for loop
  } //ii for loop


  #ifdef DO_GAMEPDAD
    // Poll the USB interface a few times. Failing to do this causes the motors to move during 
    // following a rest. I don't know why the following code fixes this, but it does. 
    for (byte ii=1; ii<10; ii++){
      Usb.Task(); 
      delay(100); 
      #ifdef DO_LCD
       lcd.print(".");
      #endif
     } //for loop
    setPSLEDS(); //Set the LEDs on the DualShock to the correct states
  #endif


  //Set default values for the AccelStepper instances
  for (byte ii=0; ii<numAxes; ii++){
    (*mySteppers[ii]).setMaxSpeed(4.0E3); //The max the Arduino Mega is capable of 
    (*mySteppers[ii]).setAcceleration(0); //Must be zero to allow hat stick speed motions
  }


  //An analog stick value of zero likely means that the controller is not connected. 
  //If the game pad is enabled, don't proceed until a contoller is found, or the 
  //stage will move by itself.
  #ifdef DO_GAMEPDAD
    while (PS3.getAnalogHat(LeftHatX)==0){
      #ifdef DO_LCD
        lcd.setCursor(0,1);
        lcd.print("Connect DualShock");
        lcd.setCursor(0,2);
        lcd.print("And Re-Boot");
      #endif
    } //while PS3
  #endif
  
  #ifdef DO_LCD
    lcd.clear();
    lcd.home();
    setupLCD();//Print axis names to LCD
  #endif


}//End of setup function 



