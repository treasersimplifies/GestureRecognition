#include "matrixkey.h"
#include "sys.h"
#include "delay.h" 
#include "usart.h"

//矩阵键盘初始化函数

void MATRI4X4KEY_Init(void){
	int i;
	RCC->AHB1ENR|=1<<1;//使能PORTB时钟 
	GPIO_Set(GPIOB,PIN0|PIN1|PIN2|PIN3,GPIO_MODE_OUT,GPIO_OTYPE_PP,GPIO_SPEED_100M,GPIO_PUPD_PD); //PB0-PB3设置为输出模式
	GPIO_Set(GPIOA,PIN0|PIN1|PIN2|PIN3,GPIO_MODE_IN,0,0,GPIO_PUPD_PD); //PB4-PB7设置为输入模式
	for(i=0;i<4;i++){//将全部的行输出0，
		BIT_ADDR(GPIOB_ODR_Addr,i)=0;  
	}	
}
//逐行扫描法：
int MATRI4_4KEY_Scan(int * row, int * column){
	
	int i =5;
	int j =5;
	
	for(i=3;i>=0;i--){
		PBout(i)=1;
		for(j=0;j<4;j++){
			if(PAin(j)==1){
				delay_ms(10);
				if(PAin(j)==1){//双重判断，消抖
					//printf("into PBin(%d)=1\n",j);
					*row = i+1;
					*column = j+1;
					PBout(i)=0;
					//printf("row = %d, column = %d \n",i,j);
					//delay_ms(1000);
					return 0;
				}
			}
		}
		PBout(i)=0;//一定要记得将这一行的输出重新置零
	}
	return -1;
	
}


//NOT USED:

//行列反转法：
int MATRI4X4KEY_Scan(int * row, int * column){
	int i,j;
	delay_ms(1);
	
	//PB0-PB3
	GPIO_Set(GPIOB,PIN0|PIN1|PIN2|PIN3,GPIO_MODE_OUT,GPIO_OTYPE_PP,GPIO_SPEED_100M,GPIO_PUPD_PD); //PB0-PB3设置为输出模式
	GPIO_Set(GPIOB,PIN4|PIN5|PIN6|PIN7,GPIO_MODE_IN,0,0,GPIO_PUPD_PD); //PB4-PB7设置为输入模式
	for(i=0;i<4;i++){//将全部的行输出1，
		BIT_ADDR(GPIOB_ODR_Addr,i)=1;  
	}	
	delay_ms(1);
	for(j=4;j<8;j++){//判断那一列有输入
		if(BIT_ADDR(GPIOA_IDR_Addr,j)==1){
			*row=j-3;//得到所按下的那个键的列值
			printf("ROW!!!\n");
			break;
		}
	}
	
	
	for(i=0;i<4;i++){			//将全部的行输出0，
		BIT_ADDR(GPIOB_ODR_Addr,i)=0;  
	}	
	//PB4-PB8
	GPIO_Set(GPIOB,PIN0|PIN1|PIN2|PIN3,GPIO_MODE_IN,0,0,GPIO_PUPD_PD); //PB0-PB3设置为输入模式
	GPIO_Set(GPIOB,PIN4|PIN5|PIN6|PIN7,GPIO_MODE_OUT,GPIO_OTYPE_PP,GPIO_SPEED_100M,GPIO_PUPD_PD); //PB4-PB7设置为输出模式
	//行列反转一下，得到行值：
	for(j=4;j<8;j++){//将全部的行输出1，
		BIT_ADDR(GPIOB_ODR_Addr,j)=1;  
	}	
	delay_ms(1);
	for(i=0;i<4;i++){//判断那一列有输入
		if(BIT_ADDR(GPIOA_IDR_Addr,i)==1){
			*column=i+1;//得到所按下的那个键的列值
			printf("COL!!!\n");
			break;
		}
	}
	if(*row>=0)
		return 0;
	else
		return -1;
}


