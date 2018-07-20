#include "stepmotor.h"
#include "delay.h" 
//////////////////////////////////////////////////////////////////////////////////	 							  
////////////////////////////////////////////////////////////////////////////////// 	 

//初始化函数
void STEPMOTOR_Init(void)
{
	
  GPIO_InitTypeDef  GPIO_InitStructure;

  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOF, ENABLE);//使能GPIOF时钟

  //GPIOF1,F2,F3初始化设置
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;//普通输出模式
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;//推挽输出
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;//100MHz
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;//上拉
  GPIO_Init(GPIOF, &GPIO_InitStructure);//初始化
	
  PFout(1)=0;
  PFout(2)=1;//DIR = 1
  PFout(3)=1;//ENA = 1
} 

int Step_Ouput(int frequency)
{	 
	 PFout(1)=1;
	 delay_ms(500/frequency);
	 PFout(1)=0;
	 delay_ms(500/frequency);
}




















