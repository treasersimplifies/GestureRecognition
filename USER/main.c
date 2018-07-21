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
//8.PC6(SCL) PC7(SDA)

//Abilities:
//1.PID 采样频率由TIM3定时器决定
//

#define TSAMPLETIMES 20//train sample times
#define DSAMPLETIMES 5//detect sample times
#define MAXTRAINTIMES 5
#define MAXRANGE 0.3 //当训练时样本点中最大最小之差大于0.2pf时，认为此次训练样本不好，需要重新训练
//好的训练样本满足：均值大于0.8，且单次训练的不同数据点之间差异小于0.2。
//因为均值小于0.8相当于是没有人手放在上面。差异大于0.2说明人手动了或者环境干扰此时非常大

//存储顺序，电容从小到大！
float mean_g[8]={-100,-100,-100,-100,-100,-100,-100,-100};
float unst_g[8]={0,0,0,0,0,0,0,0};
float measurebuffer[10];
int mode=1;//mode 
int compensation = 0;

float MaxRange(float measurebuffer[],int len){//返回数组最大最小的差值
	int i=0;
	float max=measurebuffer[0];
	float min=measurebuffer[0];
	for(i=0;i<len;i++){
		if(measurebuffer[i]>max){
			max = measurebuffer[i];
		}
		if(measurebuffer[i]<min){
			min = measurebuffer[i];
		}
	}
	return max-min;
}

float mean(float measurebuffer[],int len){//calcul mean of an array
	int i=0;
	float meansum=0;
	for(i=0;i<len;i++){
		meansum+=measurebuffer[i];
	}
	meansum=meansum/len;
	return meansum;
}

float stdsd(float measurebuffer[],int len){//calculate the std err of an array 
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
	int t;
	float trainmean;
	float trianmaxrange;
	for(t=0;t<MAXTRAINTIMES;t++){// 5 times max, for 10s
		for(i=0;i<TSAMPLETIMES;i++){
				measurebuffer[i]=Cap_Calculate(0)-fdc2214temp; // CH0 sample for 20 times.
				if(compensation==1)
					measurebuffer[i]-=ZCap_Calculate(0);
				delay_ms(100);
		}
		trainmean=mean(measurebuffer,TSAMPLETIMES);
		trianmaxrange=MaxRange(measurebuffer,TSAMPLETIMES);
		if(trianmaxrange<MAXRANGE){
			if(trainmean<0.8)continue;//无手情况
			mean_g[n-1]=mean(measurebuffer,TSAMPLETIMES);//
			unst_g[n-1]=stdsd(measurebuffer,TSAMPLETIMES);//
			printf("Mean=%f, Uncertainty=%f \r\n",mean_g[n-1],unst_g[n-1]);
			return;
		}
	}
	printf("ManRange = %f, training failed，please have another try. \r\n",MaxRange(measurebuffer,TSAMPLETIMES));
}


int Parse_Key(int row,int column,float fdc2214temp){
	int i;
	float fdc2214in=Cap_Calculate(0)-fdc2214temp;
	if(compensation==1)
		measurebuffer[i]-=ZCap_Calculate(0);
	
	if(equal2(row,column,1,1)){//1
		printf("for Traning of gesture 1.\r\n");
		gesture_train(1,fdc2214temp);
	}
	else if(equal2(row,column,1,2)){//2
		printf("for Traning of gesture 2.\r\n");
		gesture_train(2,fdc2214temp);
	}
	else if(equal2(row,column,1,3)){//3
		printf("for Traning of gesture 3.\r\n");
		gesture_train(3,fdc2214temp);
	}
	else if(equal2(row,column,2,1)){//4
		printf("for Traning of gesture 4.\r\n");
		gesture_train(4,fdc2214temp);
	}
	else if(equal2(row,column,2,2)){//5
		printf("for Traning of gesture 5.\r\n");
		gesture_train(5,fdc2214temp);
	}
	else if(equal2(row,column,4,1)){//*
		printf("for Traning of gesture ST.\r\n");//最小
		gesture_train(6,fdc2214temp);
	}
	else if(equal2(row,column,4,3)){//#
		printf("for Traning of gesture JD.\r\n");//中间
		gesture_train(7,fdc2214temp);
	}
	else if(equal2(row,column,4,4)){//D
		printf("for Traning of gesture B.\r\n");//最大
		gesture_train(8,fdc2214temp);
	}
	else if(equal2(row,column,1,4)){//A, For Detection of ST/JD/B.
		printf("For Detection of STJDB.\r\n");
		for(i=0;i<DSAMPLETIMES;i++){
			measurebuffer[i]=Cap_Calculate(0)-fdc2214temp; // CH0 detect for 10 times for mean.
			if(compensation==1)
				measurebuffer[i]-=ZCap_Calculate(0);
			delay_ms(100);
		}
		fdc2214in=mean(measurebuffer,DSAMPLETIMES);
		
		if(fdc2214in < (mean_g[6-1]+mean_g[7-1])/2 ){//ST，石头电容值最小
			printf("recognized : gesture ST .\r\n");
		}
		else if(fdc2214in < (mean_g[7-1]+mean_g[8-1])/2){
			printf("recognized : gesture JD .\r\n");
		}
		else{										//B，布电容值最大
			printf("recognized : gesture B .\r\n");
		}
		
		/*
		if(fabs(fdc2214in-mean_g[6-1])<=unst_g[6-1]){
			printf("recognized : gesture JD .\n");
		}
		else if(fabs(fdc2214in-mean_g[7-1])<=unst_g[7-1]){
			printf("recognized : gesture ST .\n");
		}
		else if(fabs(fdc2214in-mean_g[8-1])<=unst_g[8-1]){
			printf("recognized : gesture B .\n");
		}*/
	}
	else if(equal2(row,column,2,4)){//B, For Detection of 12345.
		printf("For Detection of 12345.\r\n");
		for(i=0;i<DSAMPLETIMES;i++){
			measurebuffer[i]=Cap_Calculate(0)-fdc2214temp; // CH0 detect for 10 times for mean.
			if(compensation==1)
				measurebuffer[i]-=ZCap_Calculate(0);
			delay_ms(100);
		}
		fdc2214in=mean(measurebuffer,DSAMPLETIMES);
		for(i=1;i<6;i++){
			if(i<5 && (fdc2214in <= (mean_g[i-1]+mean_g[i])/2) ){//fabs(fdc2214in-mean_g[i-1])<=unst_g[i-1]
				printf("recognized : gesture %d.\r\n",i);
				break;
			}
			if(i==5){
				if(fdc2214in > (mean_g[i-2]+mean_g[i-1])/2){
					printf("recognized : gesture %d.\r\n",i);
				}
			}
		}
		
	}
	///////////////
	else if(equal2(row,column,4,2)){//0,无训练检测模式,JD/ST/B
		mean_g[6-1]=2.5; unst_g[6-1]=0.5;//JD: 2-3
		mean_g[7-1]=1.4; unst_g[7-1]=0.6;//ST: 0.8-2
		mean_g[8-1]=3.5;  unst_g[8-1]=0.5;// B: 3-4
		
		printf("For Untrained Detection of JDSTB.\r\n");
		for(i=0;i<DSAMPLETIMES;i++){
			measurebuffer[i]=Cap_Calculate(0)-fdc2214temp; // CH0 detect for 10 times for mean.、
			if(compensation==1)
				measurebuffer[i]-=ZCap_Calculate(0);
			delay_ms(100);
		}
		fdc2214in=mean(measurebuffer,DSAMPLETIMES);
		printf("detect value: %f \r\n",fdc2214in);
		
		if(fabs(fdc2214in-mean_g[6-1])<=unst_g[6-1]){
			printf("recognized : gesture JD .\n");
		}
		else if(fabs(fdc2214in-mean_g[7-1])<=unst_g[7-1]){
			printf("recognized : gesture ST .\r\n");
		}
		else if(fabs(fdc2214in-mean_g[8-1])<=unst_g[8-1]){
			printf("recognized : gesture B .\r\n");
		}
		else{
			printf("Confused with gesture recognition......\r\n");
		}
		delay_ms(1000);
	}
	else if(equal2(row,column,3,4)){//C,无训练检测模式,1/2/3/4/5
		mean_g[1-1]=1.70; unst_g[1-1]=0.15;
		mean_g[2-1]=2.05; unst_g[2-1]=0.2;
		mean_g[3-1]=2.4;  unst_g[3-1]=0.15;
		mean_g[4-1]=2.7; unst_g[4-1]=0.15;
		mean_g[5-1]=3.5; unst_g[5-1]=0.5;
		
		printf("For Untrained Detection of 12345.\r\n");
		for(i=0;i<DSAMPLETIMES;i++){
			measurebuffer[i]=Cap_Calculate(0)-fdc2214temp; // CH0 detect for 10 times for mean.
			if(compensation==1)
				measurebuffer[i]-=ZCap_Calculate(0);
			delay_ms(100);
		}
		fdc2214in=mean(measurebuffer,DSAMPLETIMES);
		printf("detect value: %f \r\n",fdc2214in);
		
		for(i=1;i<6;i++){
			if(fabs(fdc2214in-mean_g[i-1])<=unst_g[i-1]){
				printf("recognized : gesture %d.\r\n",i);
				delay_ms(1000);
				break;
			}
		}
	}
	else if(equal2(row,column,3,3)){//9,printf existed data.
		int i=0;
		for(i=0;i<5;i++){
			printf("Mean of gesture %d = %f, Uncertainty = %f\r\n",i+1,mean_g[i],unst_g[i]);
		}
		printf("Mean of gesture JD = %f, Uncertainty = %f\r\n",mean_g[5],unst_g[5]);
		printf("Mean of gesture ST = %f, Uncertainty = %f\r\n",mean_g[6],unst_g[6]);
		printf("Mean of gesture  B = %f, Uncertainty = %f\r\n",mean_g[7],unst_g[7]);
	}
	else if(equal2(row,column,3,1)){//7
		mode++;
		if(mode==5)
			mode =1;
	}
	else if(equal2(row,column,3,2)){//8
		compensation = 1;
	}
	return 0;
}


int main(void)
{ 
	int mine_or_others=1;
	int row=0;
	int column=0;
	
	float res0;//float res1,res2,res3;
	float Zres0;//float Zres1,Zres2,Zres3;
	float temp0;//float temp1,temp2,temp3;
	float Ztemp0;//float Ztemp1,Ztemp2,Ztemp3;
	
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);//设置系统中断优先级分组2
	delay_init(168);  			//初始化延时函数
	uart_init(9600);			//初始化串口波特率为50,0000//460800:OK
	LED_Init();					//初始化LED 
	KEY_Init();					//初始化按键
 	LCD_Init();				//LCD初始化
	//TIM3_Int_Init(1000-1,8400-1);	//Timer for PID 定时器时钟84M，分频系数8400，所以84M/8400=10Khz的计数频率，计数1000次为100ms，every 0.1s一次中断 
	MATRI4X4KEY_Init();
	while(FDC2214_Init());
	while(ZFDC2214_Init());
	
	printf("Init OK \r\n.");
	delay_ms(1000);//延时1s使得这是手可以缩回防止手放在复位按钮时影响电容。

	temp0 = Cap_Calculate(0);
	//temp1 = Cap_Calculate(1);//读取初始值******
	//temp2 = Cap_Calculate(2);
	//temp3 = Cap_Calculate(3);
	Ztemp0 = ZCap_Calculate(0);
	//Ztemp1 = ZCap_Calculate(1);//读取初始值******
	//Ztemp2 = ZCap_Calculate(2);
	//Ztemp3 = ZCap_Calculate(3);
	//printf("temp0 = %f,temp1 = %f,temp2 = %f,temp3 = %f\n",temp0,temp1,temp2,temp3);
	printf("temp0 = %f,Ztemp0 = %f. \r\n",temp0,Ztemp0);
	
 	while(1)
	{
		row= -1;
		column= -1;
		res0 = Cap_Calculate(0);//采集数据
		//res1 = Cap_Calculate(1);
		//res2 = Cap_Calculate(2);
		//res3 = Cap_Calculate(3);
		Zres0 = ZCap_Calculate(0);//采集数据
		//Zres1 = ZCap_Calculate(1);
		//Zres2 = ZCap_Calculate(2);
		//Zres3 = ZCap_Calculate(3);
		
		//printf("CH0;%3.3f CH1;%3.3f CH2;%3.3f CH3;%3.3f. \n",res0-temp0,res1-temp1,res2-temp2,res3-temp3);
		//printf("ZCH0;%3.3f ZCH1;%3.3f ZCH2;%3.3f ZCH3;%3.3f. \n",Zres0-Ztemp0,Zres1-Ztemp1,Zres2-Ztemp2,Zres3-Ztemp3);
		if(mode ==2)printf("Compensation CH = %3.3f \r\n",res0-2*Zres0-(temp0-2*Ztemp0));
		if(mode ==1)printf("CH0 - =%3.3f,ZCH0 - =%3.3f \r\n",res0-temp0,Zres0-Ztemp0);
		if(mode ==3)printf("CH0 res=%f,ZCH0 res=%f \r\n",res0,Zres0);
		if(mode ==4)printf("CH0 abs =%3.3f,ZCH0 abs =%3.3f \r\n",fabs(res0-temp0),fabs(Zres0-Ztemp0));
		
		if(MATRI4_4KEY_Scan(&row, &column)==0)
			printf("key pressed row=%d, column=%d \r\n",row,column);
		
		if(mine_or_others==1){
			Parse_Key(row,column,temp0-Ztemp0);
		}
		
		delay_ms(1000);
	} 	
}
