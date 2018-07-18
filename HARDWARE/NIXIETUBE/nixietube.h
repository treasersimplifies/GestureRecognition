#ifndef __NIXIETUBE_H
#define __NIXIETUBE_H	 
#include "sys.h" 
//////////////////////////////////////////////////////////////////////////////////	 									  
////////////////////////////////////////////////////////////////////////////////// 	


void DigitalTube_Init(void); //数码管初始化
void Led_Set(int A,int B,int C,int D,int E,int F,int G,int DP);
void SingleTube_Set(int num);
void DigitalTube_Set(int num);		//数码管显示函数
void Tube_delay(int time,int num);	//数码管延时函数
void Tube_clear(int num);			//数码管清除函数，针对相位小于5度或者大于135度时
void Tube_scan_all(void);
void Tube_set_all(void);
void Tube_demo(void);
#endif



