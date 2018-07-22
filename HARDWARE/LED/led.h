#ifndef __LED_H
#define __LED_H
#include "sys.h"

//////////////////////////////////////////////////////////////////////////////////	 
									  
////////////////////////////////////////////////////////////////////////////////// 	

 

void LED_Init(void);//≥ı ºªØ	
void LED_Training(void);
void LED_Trainsuccess(void);
void LED_Trainfail(void);
void LED_Off(void);
void LED_gesture_on(int i);
void LED_gestureST_on(void);
void LED_gestureJD_on(void);
void LED_gestureB_on(void);

#endif
