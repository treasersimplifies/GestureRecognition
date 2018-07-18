#ifndef MYPID_h
#define MYPID_h
#include "usart.h"

typedef int BOOL;

#define FALSE  0
#define TRUE  1

#define AUTOMATIC  0
#define MANUAL  -1

#define DIRECT  0
#define REVERSE  1

#define P_ON_M 0
#define P_ON_E 1
  
void PID_Init(double*, double*, double*,        // * constructor.  links the PID to the Input, Output, and 
        double, double, double, int, int);//   Setpoint.  Initial tuning parameters are also set here.
                                          //   (overload for specifying proportional mode)

BOOL PID_Compute(void);                       // * performs the PID calculation.  it should be
                                          //   called every time loop() cycles. ON/OFF and
                                          //   calculation frequency can be set using SetMode
                                          //   SetSampleTime respectively

void SetTunings(double, double,       // * overload for specifying proportional mode
                    double, int);   

void SetSampleTime(int);              // * sets the frequency, in Milliseconds, with which 
                                          //   the PID calculation is performed.  default is 100
    

void SetOutputLimits(double, double); // * clamps the output to a specific range. 0-255 by default, but
									  //   it's likely the user will want to change this depending on
									  //   the application
	
void SetMode(int Mode);               // * sets PID to either Manual (0) or Auto (non-0)

void Initialize(void);
      	  
void SetControllerDirection(int);	  // * Sets the Direction, or "Action" of the controller. DIRECT
										  //   means the output will increase when error is positive. REVERSE
										  //   means the opposite.  it's very unlikely that this will be needed
										  //   once it is set in the constructor.							  

void PID_ShowConfig(void);

  //Display functions ****************************************************************
double GetKp(void);						  // These functions query the pid for interal values.
double GetKi(void);						  //  they were created mainly for the pid front-end,
double GetKd(void);						  // where it's important to know what is actually 
int GetMode(void);						  //  inside the PID.
int GetDirection(void);					  //



#endif

