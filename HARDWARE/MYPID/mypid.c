#include "mypid.h"
#include "usart.h"
/**********************************************************************************************
 * This is my implementation of PID controll for stm32f407, ARM-C, 
   which is transplanted from Arduino PID Library - Version 1.2.1 by Brett Beauregard.
 
 * Transplant Author : Treaser Lou 
 
 * This Library is licensed under the MIT License
 **********************************************************************************************/

double dispKp;				// * we'll hold on to the tuning parameters in user-entered 
double dispKi;				//   format for display purposes
double dispKd;				//
    
double kp;                  // * (P)roportional Tuning Parameter
double ki;                  // * (I)ntegral Tuning Parameter
double kd;                  // * (D)erivative Tuning Parameter

double *myInput;              // * Pointers to the Input, Output, and Setpoint variables
double *myOutput;             //   This creates a hard link between the variables and the 
double *mySetpoint;           //   PID, freeing the user from having to constantly tell us
                              //   what these values are.  with pointers we'll just know.					  
//unsigned long lastTime;
double outputSum, lastInput;  //merge P-term and I-term into a single variable called ¡°outputSum¡±

//some configuation parameters:
unsigned long SampleTime;
double outMin, outMax;
//int inAuto, pOnE;
//BOOL pOnE = TRUE, pOnM = FALSE;//determine whether PonE or PonM
BOOL inAuto;				   //determine whether auto or manual
int pOn;
int controllerDirection = DIRECT;


/*Constructor (...)*********************************************************
 *    The parameters specified here are those for for which we can't set up
 *    reliable defaults, so we need to have the user set them.
 ***************************************************************************/
 
void PID_Init(double* Input, double* Output, double* Setpoint,
        double Kp, double Ki, double Kd, int POn, int ControllerDirection)
{
    myOutput = Output;
    myInput = Input;
    mySetpoint = Setpoint;
    inAuto = TRUE;				//FALSE

    SetOutputLimits(0, 4095);	//default output limit corresponds to STM32 DAC config in main.c

    SampleTime = 100;			//default Sample Time is 0.1 s, decided by TIM3_Int_Init(1000-1,8400-1) in main.c

    SetControllerDirection(ControllerDirection);
    SetTunings(Kp, Ki, Kd, POn);

    //lastTime = millis()-SampleTime;
}


/* Compute() **********************************************************************
 *   This, as they say, is where the magic happens.  this function should be called
 *   every time "void loop()" executes.  the function will decide for itself whether a new
 *   pid Output needs to be computed.  returns true when the output is computed,
 *   false when nothing has been done.
 **********************************************************************************/
BOOL PID_Compute()
{
   
   //unsigned long now = millis();
   //unsigned long timeChange = (now - lastTime);
   //if(timeChange>=SampleTime)
   //{
	/*     Classic PID: 	output=Kp*e(t)+ Ki*integral(e(t))+Kd*d(e(t))     */
	/*     Practical PID:	output= -kp*(I(t)-setpoint) + integral(Ki(t)*e(t))-Kd*d(I(t)) 
		   where PonE or PonM needs to be decided */
	
      /*Compute all the working error variables*/
	  double input = *myInput;
      double error = *mySetpoint - input;
      double dInput = (input - lastInput);
	  double output;
	
		if(inAuto==MANUAL) return FALSE;	//inAuto=MANUAL=0=FALSE
      outputSum+= (ki * error);

      /*Add Proportional on Measurement, if P_ON_M is specified*/
      if(pOn==P_ON_M) outputSum-= kp * dInput;//if(pOnE==FALSE)
      if(outputSum > outMax) outputSum= outMax;
      else if(outputSum < outMin) outputSum= outMin;

      /*Add Proportional on Error, if P_ON_E is specified*/
	  
      if(pOn==P_ON_E) output = kp * error;
      else output = 0;

      /*Compute Rest of PID Output*/
      output += outputSum - kd * dInput;

	  if(output > outMax) output = outMax;
      else if(output < outMin) output = outMin;
	  *myOutput = output;

      /*Remember some variables for next time*/
      lastInput = input;
      //lastTime = now;
	  return TRUE;
   //}
   //else return FALSE;
}

/* SetTunings(...)*************************************************************
 * This function allows the controller's dynamic performance to be adjusted.
 * it's called automatically from the constructor, but tunings can also
 * be adjusted on the fly during normal operation
 ******************************************************************************/
void SetTunings(double Kp, double Ki, double Kd, int POn)
{
   double SampleTimeInSec;
   if (Kp<0 || Ki<0 || Kd<0) return;

   pOn = POn;
   //pOnE = POn == P_ON_E;

   dispKp = Kp; dispKi = Ki; dispKd = Kd;

   SampleTimeInSec = ((double)SampleTime)/1000;
   kp = Kp;
   ki = Ki * SampleTimeInSec;
   kd = Kd / SampleTimeInSec;

  if(controllerDirection ==REVERSE)
   {
      kp = (0 - kp);
      ki = (0 - ki);
      kd = (0 - kd);
   }
}

/* SetSampleTime(...) *********************************************************
 * sets the period, in Milliseconds, at which the calculation is performed
 ******************************************************************************/
void SetSampleTime(int NewSampleTime)
{
   if (NewSampleTime > 0)
   {
      double ratio  = (double)NewSampleTime
                      / (double)SampleTime;
      ki *= ratio;
      kd /= ratio;
      SampleTime = (unsigned long)NewSampleTime;
   }
}

/* SetOutputLimits(...)****************************************************
 *     This function will be used far more often than SetInputLimits.  while
 *  the input to the controller will generally be in the 0-1023 range (which is
 *  the default already,)  the output will be a little different.  maybe they'll
 *  be doing a time window and will need 0-8000 or something.  or maybe they'll
 *  want to clamp it from 0-125.  who knows.  at any rate, that can all be done
 *  here.
 **************************************************************************/
void SetOutputLimits(double Min, double Max)
{
   if(Min >= Max) return;
   outMin = Min;
   outMax = Max;

   if(inAuto)
   {
	   if(*myOutput > outMax) *myOutput = outMax;
	   else if(*myOutput < outMin) *myOutput = outMin;

	   if(outputSum > outMax) outputSum= outMax;
	   else if(outputSum < outMin) outputSum= outMin;
   }
}

/* SetMode(...)****************************************************************
 * Allows the controller Mode to be set to manual (0) or Automatic (non-zero)
 * when the transition from manual to auto occurs, the controller is
 * automatically initialized
 ******************************************************************************/
void SetMode(int Mode)
{
    BOOL newAuto = Mode;//BOOL newAuto = (Mode == AUTOMATIC);
    if(newAuto && !inAuto)
    {  /*we just went from manual to auto*/
        Initialize();
    }
    inAuto = newAuto;
}

/* Initialize()****************************************************************
 *	does all the things that need to happen to ensure a bumpless transfer
 *  from manual to automatic mode.
 ******************************************************************************/
void Initialize()
{
   outputSum = *myOutput;
   lastInput = *myInput;
   if(outputSum > outMax) outputSum = outMax;
   else if(outputSum < outMin) outputSum = outMin;
}

/* SetControllerDirection(...)*************************************************
 * The PID will either be connected to a DIRECT acting process (+Output leads
 * to +Input) or a REVERSE acting process(+Output leads to -Input.)  we need to
 * know which one, because otherwise we may increase the output when we should
 * be decreasing.  This is called from the constructor.
 ******************************************************************************/
void SetControllerDirection(int Direction)
{
   if(inAuto && Direction !=controllerDirection)
   {
	    kp = (0 - kp);
      ki = (0 - ki);
      kd = (0 - kd);
   }
   controllerDirection = Direction;
}


void ShowConfig(void){
	printf("SampleTime = %ld, outMin=%f, outMax=%f, inAuto=%d, pOn=%d, controllerDirection =%d\n",SampleTime,outMin,outMax,inAuto,pOn,controllerDirection);
}


/* Status Funcions*************************************************************
 * Just because you set the Kp=-1 doesn't mean it actually happened.  these
 * functions query the internal state of the PID.  they're here for display
 * purposes.  this are the functions the PID Front-end uses for example
 ******************************************************************************/
double GetKp(){ return  dispKp; }
double GetKi(){ return  dispKi;}
double GetKd(){ return  dispKd;}
int GetMode(){ return  inAuto;}
int GetDirection(){ return controllerDirection;}

