
// Some global definitions that need to be modified by the user for the stage to work


//------------------------------------------------------------------------------------------------
// * Enable/disable major OpenStage functions 
//
//#define DO_LCD      //Uncomment this line to enable enable LCD character display
//#define DO_GAMEPDAD //Uncomment this line to enable PS3 DualShock as an input device




//------------------------------------------------------------------------------------------------
// * Serial communications
//
// You have three options for serial comms: 
// 1) Control stage via PC serial port and receive optional debug information via USB (virtual 
//    serial port). The latter is available by the verbose option in some of the functions. 
// 2) Control stage via USB (virtual serial) but receive no debug info. 
// 3) Disable serial comms for stage control. 
//
// These three possibilities are controlled by altering the following two variables:
#define DO_SERIAL_INTERFACE
bool controlViaUSB=1;     //Set to 1 to control via USB. See setup() for serial port IDs.

//If using PC serial port, this is the ID of the MEGA's hardware serial line (e.g. Serial3)
//If using an Uno, or another mic with no hardware serial, then this should be set to "Serial"
#define HARDWARE_SERIAL_PORT Serial 
HardwareSerial* SerialComms;  //pointer to stage comms serial object

#ifdef DO_SERIAL_INTERFACE
 bool doSerialInterface=1; //Set to 1 to communicate with the stage via a PC serial port. 
#endif


//------------------------------------------------------------------------------------------------
// * Microscope axis hardware definitions. 
//
//                                   === WARNING ===
//   ******************************************************************************
//  ==> This section must be filled out correctly or you may damage your hardware <==
//   ******************************************************************************
//
//
// The properties of each axis are defined as separate variables (mostly arrays with a length 
// equal to the number of axes). You can have up 4 axes. 

const byte numAxes=1; //Set this to the number of axes on you system 

// axisPresent
// Allows particular axes to be skipped. Useful for testing. 1 means present. 0 means absent. 
// e.g. if you have one axis the following vector might be {1,0,0,0} Although {0,1,0,0} should 
// also work
bool axisPresent[maxAxes]={1,0,0,0}; 


// gearRatio
// Micrometer gear ratios on X,Y,Z in microns per revolution. Unused axes can have any number. 
unsigned short gearRatio[maxAxes]={635,635,250,635}; 


// fullStep
// Stepper motor full step size (in degrees) for X,Y,Z. We have tested 1.8 degree and 0.9
// degree step sizes. Motors with finer step sizes are available but driving them in 
// quickly using sub-micron steps is a limiting factor. If you want to get up and running 
// in the shortest time, 0.9 degree motors are suggested. 
float fullStep[maxAxes]={0.9,0.9,0.9,0.9}; //In degrees

// disableWhenStationary
// bool to tell the system whether or not a motor should be disabled when the stage isn't moving.
// Disabling will reduce noise but can cause the motors to move to the nearest full step when the
// power is switched off. Perhaps it makes make sense to do this in X and Y but not Z. 
bool disableWhenStationary[maxAxes]={0,0,0,0};






//------------------------------------------------------------------------------------------------
// * Motor control DIO lines
// Alter the following based on how your system is wired. 

// stepOut
// One pulse at these pins moves the motor by one step (or one micro-step)
byte stepOut[maxAxes]={11,24,26,0}; //Set these to the step out pins (ordered X, Y, and Z)

// stepDir
// These pins tell the Big Easy Driver to which they connect which direction to rotate the motor
byte stepDir[maxAxes]={12,25,27,0}; //Ordered X, Y, and Z

// enable
// If these pins are low, the motor is enabled. If high it's disabled. Disabling might decrease 
// electrical noise but will lead to errors in absolute positioning. 
byte enable[maxAxes]={3,29,30,0}; //Ordered X, Y, and Z

// The microstep pins.
// These pins define the microstep size. The MS pins on all axes are wired together.
byte MS1=4;
byte MS2=5;
byte MS3=6;





//------------------------------------------------------------------------------------------------
// * AccelStepper 
//Make an array of pointers to the AccelStepper instances to make loops possible
AccelStepper *mySteppers[numAxes];






//------------------------------------------------------------------------------------------------
// * Outputs
// The following are pin definitions of controller outputs. These signal information to the user

byte beepPin=13; //Set this to the pin to which the Piezo buzzer is connected 

// stageLEDs
// LEDs will light when the stage moves or an axis is reset, 
// *NOTE: Updating the stage LEDs for hat-stick motions requires fast digital writes as motor speed
//        is set in a closed-loop based on hat-stick position. So we set the stage LEDs using 
//        direct port writes. The code expects the LEDS to be on the first 3 pins of port C of the
//        Mega. You can use a different port (or even different pins on that port), but then you will
//        have to change the code in pollPS3. Direct port writes on these pins are only implemented 
//        for the closed-loop hat-stick control code. Elsewhere we use conventional Arduino 
//        digitalWrites, as these are adequate. 
//
// PC0: 37 (X axis)
// PC1: 36 (Y axis)
// PC2: 35 (Z axis)
// PC3: 34 (a good idea to reserve this for a 4th axis, such as a PIFOC)
byte stageLEDs[maxAxes]={13,36,35,35};



// Define pins for LCD display (http://learn.adafruit.com/character-lcds/wiring-a-character-lcd)
#if doLCD
   LiquidCrystal lcd(7, 6, 5, 4, 3, 2 );
#endif





//------------------------------------------------------------------------------------------------
// PS3 Controller speed settings
// The following settings define how the PS3 controller will control the stage. If you don't have a
// PS3 controller, you can ignore the following 

// * Speed modes (hat-stick)
//
// There will be four (coarse to fine) speeds which may be selected via the two shoulder buttons 
// (L1 and R1). The currently selected speed will be inidicated by the 4 LEDs on the DualShock.
// 1 is fine and slow and 4 is coarse and fast. Here we define the step size and max speed for those
// 4 speed settings. If you change these, you should verify with an osciloscope and the serial 
// monitor that the right values are being produced. You will also need to verify that you are not
// over-driving you motor and causing it to miss steps. Missing steps means the controller will lose
// its absolute position (there is no feedback from the motors). Note that the selection of speed
// values and step sizes are chosen based on the resulting step frequencies the controller must 
// produce and on resonances the motors might exhibit.   
unsigned short maxSpeed[4]={3.5,25,100,750}; //defined in microns per second


// stepSize
// The microstep sizes for each speed mode.
float stepSize[4]={1/16.0, 1/8.0, 1/4.0, 1/2.0}; //Defined in fractions of a full step

// * Speed mode (D-pad)
//
// The D-pad will be used for making fixed-distance motions of a high speed. The size of the motions
// depends on the coarseFine setting and is defined by the DPadStep array. There's a trade-off between
// speed and accuracy. For these fast motions, I'd like to be hit about 1000 um/s but the AccelStepper 
// moveTo function can only churn out about 4.3 kHz on an Arduino Mega. At 1/4 steps we get reliable
// motions and about 700 um/s. A 1/4 step gives us a 0.156 micron resolution in Z (0.9 degree stepper 
// and 250 um per rev micrometer). The motors MUST be enabled throughout or they will slip and become
// hopelessly lost. To get increased accuracy *and* higher speeds we would need a faster micro-controller,
// or write our own hardware-timer based routines, or slave comercial controllers.
float DPadStep[4]={3,5,10,50}; 
float DPadStepSize=1/2.0;
// Acceleration in X, Y, and Z
unsigned long DPadAccel[maxAxes]={1.0E4, 1.0E4, 1.0E4, 1.0E4};

// * moveTo speeds
//
// The moveToTarget() function is executed when the user travels to a right-button set point
// or responds to a serial command. It does this using the settings described below. The minimum
// step size and motor RPM are reported to the serial terminal on bootup.
float moveToStepSize=1.0/2.0;
unsigned int moveToSpeed[maxAxes]={1600,1600,1200,1600}; 
unsigned int moveToAccel[maxAxes]={1.0E4,1.0E4,1.0E4,1.0E4};





//---------------------------------UNLIKELY TO NEED TO CHANGE THESE-------------------------------
// The following settings can be changed, but you're unlikely to need to do so. 
byte coarseFine=2; //boot up in a fine mode
short hatStickThresh=40; //Ensure we don't get motion for stick values smaller than the following threshold

// curve
// Curve is the non-linear mapping value to convert analog stick position to speed. The idea
// is to make the conversion from analog stick value to motor pulse rate logarithmic. 
// 0 is linear. See the fscale function for details. 
float curve[4]={-7,-6,-5,-4};

