#include "sys.h"
#include "delay.h"
#include "usart.h"
#include "led.h"
#include "key.h"
#include "math.h"
#include "lcd.h"
//#include "timer.h"
//#include "adc.h"
//#include "dac.h"
#include "matrixkey.h"
#include "fdc2214.h"
//#include "nixietube.h"

//Pins used：
//1.PB9,PB8 : I2C for MPU6050
//2.PC0 : INT for MPU6050
//3.PA5 : ADC Input  CH5, 12bit mode
//4.PA4 : DAC Output CH1, 12bit
//5.PB0-PB7 : 4X4 matrix keys
//7.PC4(SCL) PC5(SDA) ： I2C for fdc2214 PC

//Abilities:
//1.PID 采样频率由TIM3定时器决定
//

#define TRAININGTIMES 20
#define DETEFECTTIMES 10

float mean_g[8]={-100,-100,-100,-100,-100,-100,-100,-100};
float unst_g[8]={0,0,0,0,0,0,0,0};
float measurebuffer[10];

float mean(float measurebuffer[],int len){
	int i=0;
	float meansum=0;
	for(i=0;i<len;i++){
		meansum+=measurebuffer[i];
	}
	meansum=meansum/len;
	return meansum;
}

float stdsd(float measurebuffer[],int len){
	int i=0;
	float errsum=0;
	float mean_g = mean(measurebuffer,len);
	for(i=0;i<len;i++){
		errsum+=abs((10000*measurebuffer[i]-10000*mean_g));
	}
	errsum=10*errsum/(len-1);//adjust sensitivity
	return errsum/10000;
}

int equal2(int row,int column,int expect_row,int expect_column){
	return row==expect_row&&column==expect_column?1:0;
}

void gesture_train(int n,float fdc2214temp){
	int i;
	for(i=0;i<TRAININGTIMES;i++){
			measurebuffer[i]=Cap_Calculate(0)-fdc2214temp; // CH0 sample for 20 times.
			delay_ms(100);
		}
		mean_g[n-1]=mean(measurebuffer,TRAININGTIMES);
		unst_g[n-1]=stdsd(measurebuffer,TRAININGTIMES);
		printf("mean=%f, uncertainty=%f\n",mean_g[n-1],unst_g[n-1]);
}



int Parse_Key(int row,int column,float fdc2214temp){
	int i;
	float fdc2214in;
	if(equal2(row,column,1,1)){//1
		printf("for Traning of gesture 1.\n");
		gesture_train(1,fdc2214temp);
	}
	else if(equal2(row,column,1,2)){//2
		printf("for Traning of gesture 2.\n");
		gesture_train(2,fdc2214temp);
	}
	else if(equal2(row,column,1,3)){//3
		printf("for Traning of gesture 3.\n");
		gesture_train(3,fdc2214temp);
	}
	else if(equal2(row,column,2,1)){//4
		printf("for Traning of gesture 4.\n");
		gesture_train(4,fdc2214temp);
	}
	else if(equal2(row,column,2,2)){//5
		printf("for Traning of gesture 5.\n");
		gesture_train(5,fdc2214temp);
	}
	else if(equal2(row,column,4,1)){//*
		printf("for Traning of gesture JD.\n");
		gesture_train(6,fdc2214temp);
	}
	else if(equal2(row,column,4,3)){//#
		printf("for Traning of gesture ST.\n");
		gesture_train(7,fdc2214temp);
	}
	else if(equal2(row,column,4,4)){//D
		printf("for Traning of gesture B.\n");
		gesture_train(8,fdc2214temp);
	}
	else if(equal2(row,column,1,4)){//A
		printf("For Detection of JDSTB.\n");
		for(i=0;i<DETEFECTTIMES;i++){
			measurebuffer[i]=Cap_Calculate(0)-fdc2214temp; // CH0 detect for 10 times for mean.
			delay_ms(100);
		}
		fdc2214in=mean(measurebuffer,DETEFECTTIMES);
		
		if(fabs(fdc2214in-mean_g[6-1])<=unst_g[6-1]){
			printf("recognized : gesture JD .\n");
		}
		else if(fabs(fdc2214in-mean_g[7-1])<=unst_g[7-1]){
			printf("recognized : gesture ST .\n");
		}
		else if(fabs(fdc2214in-mean_g[8-1])<=unst_g[8-1]){
			printf("recognized : gesture B .\n");
		}
		delay_ms(1000);
	}
	else if(equal2(row,column,2,4)){//B
		printf("For Detection of 12345.\n");
		for(i=0;i<DETEFECTTIMES;i++){
			measurebuffer[i]=Cap_Calculate(0)-fdc2214temp; // CH0 detect for 10 times for mean.
			delay_ms(100);
		}
		fdc2214in=mean(measurebuffer,DETEFECTTIMES);
		for(i=1;i<6;i++){
			if(fabs(fdc2214in-mean_g[i-1])<=unst_g[i-1]){
				printf("recognized : gesture %d.\n",i);
				delay_ms(1000);
				break;
			}
		}
		
	}
	else if(equal2(row,column,4,2)){//0,无训练检测模式,JD/SD/B
		mean_g[6-1]=2.5; unst_g[6-1]=0.5;//JD: 2-3
		mean_g[7-1]=1.4; unst_g[7-1]=0.6;//ST: 0.8-2
		mean_g[8-1]=3.5;  unst_g[8-1]=0.5;// B: 3-4
		
		printf("For Untrained Detection of JDSTB.\n");
		for(i=0;i<DETEFECTTIMES;i++){
			measurebuffer[i]=Cap_Calculate(0)-fdc2214temp; // CH0 detect for 10 times for mean.
			delay_ms(100);
		}
		fdc2214in=mean(measurebuffer,DETEFECTTIMES);
		printf("detect value: %f \n",fdc2214in);
		
		if(fabs(fdc2214in-mean_g[6-1])<=unst_g[6-1]){
			printf("recognized : gesture JD .\n");
		}
		else if(fabs(fdc2214in-mean_g[7-1])<=unst_g[7-1]){
			printf("recognized : gesture ST .\n");
		}
		else if(fabs(fdc2214in-mean_g[8-1])<=unst_g[8-1]){
			printf("recognized : gesture B .\n");
		}
		else{
			printf("Confused with gesture recognition......\n");
		}
		delay_ms(1000);
	}
	else if(equal2(row,column,3,4)){//C,无训练检测模式,1/2/3/4/5
		mean_g[1-1]=1.70; unst_g[1-1]=0.15;
		mean_g[2-1]=2.05; unst_g[2-1]=0.2;
		mean_g[3-1]=2.4;  unst_g[3-1]=0.15;
		mean_g[4-1]=2.7; unst_g[4-1]=0.15;
		mean_g[5-1]=3.5; unst_g[5-1]=0.5;
		
		printf("For Untrained Detection of 12345.\n");
		for(i=0;i<DETEFECTTIMES;i++){
			measurebuffer[i]=Cap_Calculate(0)-fdc2214temp; // CH0 detect for 10 times for mean.
			delay_ms(100);
		}
		fdc2214in=mean(measurebuffer,DETEFECTTIMES);
		printf("detect value: %f \n",fdc2214in);
		
		for(i=1;i<6;i++){
			if(fabs(fdc2214in-mean_g[i-1])<=unst_g[i-1]){
				printf("recognized : gesture %d.\n",i);
				delay_ms(1000);
				break;
			}
		}
	}
	
	return 0;
}


int main(void)
{ 
	int mine_or_others=1;
	int row=0;
	int column=0;
	
	float res0,res1,res2,res3;
	float temp0,temp1,temp2,temp3;
	
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);//设置系统中断优先级分组2
	delay_init(168);  			//初始化延时函数
	uart_init(460800);			//初始化串口波特率为50,0000//460800:OK
	LED_Init();					//初始化LED 
	KEY_Init();					//初始化按键
 	LCD_Init();				//LCD初始化
	//TIM3_Int_Init(1000-1,8400-1);	//Timer for PID 定时器时钟84M，分频系数8400，所以84M/8400=10Khz的计数频率，计数1000次为100ms，every 0.1s一次中断 
	MATRI4X4KEY_Init();
	while(FDC2214_Init());
	
	printf("Init OK\n.");
	delay_ms(10);
	delay_ms(1000);//延时1s使得这是手可以缩回防止手放在复位按钮时影响电容。
	
	temp0 = Cap_Calculate(0);//读取初始值******
	temp1 = Cap_Calculate(1);
	temp2 = Cap_Calculate(2);
	temp3 = Cap_Calculate(3);
	printf("temp0 = %f,temp1 = %f,temp2 = %f,temp3 = %f\n",temp0,temp1,temp2,temp3);
	
 	while(1)
	{
		row= -1;
		column= -1;
		res0 = Cap_Calculate(0);//采集数据
		res1 = Cap_Calculate(1);
		res2 = Cap_Calculate(2);
		res3 = Cap_Calculate(3);
		printf("CH0;%3.3f CH1;%3.3f CH2;%3.3f CH3;%3.3f\r\n",res0-temp0,res1-temp1,res2-temp2,res3-temp3);
		if(MATRI4_4KEY_Scan(&row, &column)==0)
			printf("key pressed row=%d, column=%d\n",row,column);
		if(mine_or_others==1){
			Parse_Key(row,column,temp0);
		}
		delay_ms(1000);
	} 	
}
