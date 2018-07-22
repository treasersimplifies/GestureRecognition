#include "led.h" 								  
////////////////////////////////////////////////////////////////////////////////// 	 

//初始化PA11 12 13 14 15 为输出口.并使能这两个口的时钟		    
//LED IO初始化
void LED_Init(void)
{    	 
  GPIO_InitTypeDef  GPIO_InitStructure;

  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);//使能GPIOA时钟

  //GPIOPA11 12 13 14 15初始化设置
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11 | GPIO_Pin_12| GPIO_Pin_13| GPIO_Pin_14| GPIO_Pin_15;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;//普通输出模式
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;//推挽输出
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;//100MHz
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;//上拉
  GPIO_Init(GPIOF, &GPIO_InitStructure);//初始化
	
  GPIO_SetBits(GPIOF,GPIO_Pin_9 | GPIO_Pin_10);//GPIOF9,F10设置高，灯灭

}

void LED_Training(void){
	PAout(11)=0;
	PAout(12)=1;
	PAout(13)=1;
}
void LED_Trainsuccess(void){
	PAout(11)=1;
	PAout(12)=0;
	PAout(13)=1;
}
void LED_Trainfail(void){
	PAout(11)=1;
	PAout(12)=1;
	PAout(13)=0;
}

void LED_Off(void){
	PAout(11)=1;
	PAout(12)=1;
	PAout(13)=1;
}


