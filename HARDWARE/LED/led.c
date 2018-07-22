#include "led.h" 								  
////////////////////////////////////////////////////////////////////////////////// 	 

//初始化PA11 12 13 14 15 为输出口.并使能这两个口的时钟		    
//LED IO初始化
void LED_Init(void)
{    	 
  GPIO_InitTypeDef  GPIO_InitStructure;
  GPIO_InitTypeDef  GPIO_InitStructure2;

  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);//使能GPIOA时钟

  //GPIOPA11 12 13 14 15初始化设置
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11 | GPIO_Pin_12| GPIO_Pin_13| GPIO_Pin_14| GPIO_Pin_15;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;//普通输出模式
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;//推挽输出
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;//100MHz
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;//上拉
  GPIO_Init(GPIOA, &GPIO_InitStructure);//初始化
	
  GPIO_SetBits(GPIOA,GPIO_Pin_11 | GPIO_Pin_12| GPIO_Pin_13| GPIO_Pin_14| GPIO_Pin_15);//GPIOA10,A11设置高，灯灭
		
  
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOG, ENABLE);//使能GPIOG时钟
	
  GPIO_InitStructure2.GPIO_Pin = GPIO_Pin_2 | GPIO_Pin_3| GPIO_Pin_4| GPIO_Pin_5| GPIO_Pin_6;
  GPIO_InitStructure2.GPIO_Mode = GPIO_Mode_OUT;//普通输出模式
  GPIO_InitStructure2.GPIO_OType = GPIO_OType_PP;//推挽输出
  GPIO_InitStructure2.GPIO_Speed = GPIO_Speed_100MHz;//100MHz
  GPIO_InitStructure2.GPIO_PuPd = GPIO_PuPd_UP;//上拉
  GPIO_Init(GPIOG, &GPIO_InitStructure2);//初始化	
  GPIO_SetBits(GPIOG,GPIO_Pin_2 | GPIO_Pin_3| GPIO_Pin_4| GPIO_Pin_5| GPIO_Pin_6);//GPIOG2 - G6设置高，灯灭

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
	PGout(2)=1;
	PGout(3)=1;
	PGout(4)=1;
	PGout(5)=1;
	PGout(6)=1;
}

void LED_gesture_on(int i){
	i++;
	PGout(2)=(i==2?0:1);
	PGout(3)=(i==3?0:1);
	PGout(4)=(i==4?0:1);
	PGout(5)=(i==5?0:1);
	PGout(6)=(i==6?0:1);
}

void LED_gestureST_on(void){
	PGout(2)=0;
	PGout(3)=1;
	PGout(4)=1;
	PGout(5)=1;
	PGout(6)=1;
}
void LED_gestureJD_on(void){
	PGout(2)=1;
	PGout(3)=0;
	PGout(4)=1;
	PGout(5)=1;
	PGout(6)=1;
}
void LED_gestureB_on(void){
	PGout(2)=1;
	PGout(3)=1;
	PGout(4)=0;
	PGout(5)=1;
	PGout(6)=1;
}
