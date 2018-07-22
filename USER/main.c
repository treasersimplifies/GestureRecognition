#include "sys.h"
#include "delay.h"
#include "usart.h"
#include "math.h"

#include "led.h"
#include "key.h"
#include "lcd.h"
//#include "timer.h"
//#include "adc.h"
//#include "dac.h"
#include "matrixkey.h"
#include "fdc2214.h"
//#include "nixietube.h"

//Pins used：  

//1.PB0-PB3/PA0-PA3 : 4X4 matrix keys
//2.PC4(SCL) PC5(SDA) ： I2C for fdc2214 PC
//3.PC6(SCL) PC7(SDA)
//4:PB15,PD0,PD1,PD4,PD5,PD8,PD9,PD10,PD14,PD15,PE7~15,PF12,PG12: LCD;
//5.PF1 3 5:波妞开关
//6.PA11 12 13 14 15

//Abilities:
//1.PID 采样频率由TIM3定时器决定
//

#define TSAMPLETIMES 10//train sample times
#define DSAMPLETIMES 5//detect sample times
#define MAXTRAINTIMES 3
#define MAXRANGE 0.2 //当训练时样本点中最大最小之差大于0.2pf时，认为此次训练样本不好，需要重新训练
#define NOHANDSTATUS 0.4
//好的训练样本满足：均值大于0.8，且单次训练的不同数据点之间差异小于0.2。
//因为均值小于0.8相当于是没有人手放在上面。差异大于0.2说明人手动了或者环境干扰此时非常大

//存储顺序，电容从小到大！
float mean_g[8]={-1,-1,-1,-1,-1,-1,-1,-1};
float unst_g[8]={0,0,0,0,0,0,0,0};
float measurebuffer[10];
//使用波妞开关改变：
int printfmode=1;//mode 
int compensation = 0;
//
int mode=0;//=0:training, =1:detect
int current_gesture=0;//1:1,2:2,6:ST,7:JD,8:B
int train_status=2;//=1:train finished, =-1:failed =0:training
int current_key=0;//1:1,16:D
float C0,C1,C2,C3,C4=0;
float ZC0,ZC1,ZC2,ZC3,ZC4=0;

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


/*********************MAIN FUNCTIONS*********************/

void falsegesture_train(int n,float fdc2214temp){
	int i;
	int t;
	float trainmean;
	float trianmaxrange;
	
	LED_Training();
	
	for(t=0;t<MAXTRAINTIMES;t++){// 3 times max, for 10s
		
		for(i=0;i<TSAMPLETIMES;i++){
			measurebuffer[i]=Cap_Calculate(0)-fdc2214temp; // CH0 sample for 10 times.
			if(compensation==1)
				measurebuffer[i]-=ZCap_Calculate(0);
			delay_ms(50);
		}
		trainmean=mean(measurebuffer,TSAMPLETIMES);
		
		trianmaxrange=MaxRange(measurebuffer,TSAMPLETIMES);
		if(trianmaxrange<MAXRANGE){//好训练集情况
			if(trainmean<NOHANDSTATUS)continue;//无手情况
			mean_g[n-1]=mean(measurebuffer,TSAMPLETIMES);//
			unst_g[n-1]=stdsd(measurebuffer,TSAMPLETIMES);//
			printf("Mean=%f, Uncertainty=%f \r\n",mean_g[n-1],unst_g[n-1]);
			//LCD_ShowString(30,70,120,16,16,"Mean=: ");
			//LCD_ShowNum(130,70,mean_g[n-1]*100,3,16);
			//LCD_ShowString(180,70,100,16,16,"/100 pf");
			
			LED_Trainsuccess();
			return;
		}
		//坏训练集情况则需要不断循环，如果循环次数大于10就返回训练是失败
		
		
	}
	
	LED_Trainfail();
	printf("***，ManRange = %f, training failed，please have another try. \r\n",MaxRange(measurebuffer,TSAMPLETIMES));
	
}

int gesture_train(int n,float fdc2214temp){
	int i;
	int t;
	float trainmean;
	float trianmaxrange;
	
	LED_Training();
	
	for(t=0;t<MAXTRAINTIMES;t++){// 3 times max, for 10s
		
		for(i=0;i<TSAMPLETIMES;i++){
			measurebuffer[i]=Cap_Calculate(0)-fdc2214temp; // CH0 sample for 10 times.
			if(compensation==1)
				measurebuffer[i]-=ZCap_Calculate(0);
			delay_ms(50);
		}
		trainmean=mean(measurebuffer,TSAMPLETIMES);
		
		trianmaxrange=MaxRange(measurebuffer,TSAMPLETIMES);
		if(trianmaxrange<MAXRANGE){//好训练集情况
			if(trainmean<NOHANDSTATUS)continue;//无手情况
			mean_g[n-1]=mean(measurebuffer,TSAMPLETIMES);//
			unst_g[n-1]=stdsd(measurebuffer,TSAMPLETIMES);//
			printf("Mean=%f, Uncertainty=%f \r\n",mean_g[n-1],unst_g[n-1]);
			POINT_COLOR=GREEN;
			LCD_ShowString(30,210,210,16,16,"Train Status: OK.");
			//LCD_ShowString(30,70,120,16,16,"Mean=: ");
			//LCD_ShowNum(130,70,mean_g[n-1]*100,3,16);
			//LCD_ShowString(180,70,100,16,16,"/100 pf");
			
			LED_Trainsuccess();
			return 0;
		}
		//坏训练集情况则需要不断循环，如果循环次数大于10就返回训练是失败
	}
	
	LED_Trainfail();
	POINT_COLOR=RED;
	LCD_ShowString(30,210,210,16,16,"Train Status: Failed.");
	printf("***，ManRange = %f, training failed，please have another try. \r\n",MaxRange(measurebuffer,TSAMPLETIMES));
	return 1;
}


int Parse_Key(int row,int column,float fdc2214temp){
	int i;
	float fdc2214in;
	int trainflag;
	
	
	//训练按键：
	if(equal2(row,column,1,1)){//1
		printf("for Traning of gesture 1.\r\n");
		POINT_COLOR=YELLOW;
		LCD_ShowString(30,345,200,12,12,"1:gesture 1 training.");
		trainflag=gesture_train(1,fdc2214temp);
		POINT_COLOR=GREEN;
		if(trainflag==0)
			LCD_ShowString(30,345,200,12,12,"1:gesture 1 trained!!!");
		LCD_ShowNum(100,190,mean_g[0]*100,3,16);
	}
	else if(equal2(row,column,1,2)){//2
		printf("for Traning of gesture 2.\r\n");
		POINT_COLOR=YELLOW;
		LCD_ShowString(30,360,200,12,12,"2:gesture 2 training.");
		trainflag=gesture_train(2,fdc2214temp);
		POINT_COLOR=GREEN;
		if(trainflag==0)
			LCD_ShowString(30,360,200,12,12,"2:gesture 2 trained!!!");
		LCD_ShowNum(140,190,mean_g[1]*100,3,16);
	}
	else if(equal2(row,column,1,3)){//3
		printf("for Traning of gesture 3.\r\n");
		POINT_COLOR=YELLOW;
		LCD_ShowString(30,375,200,12,12,"3:gesture 3 training.");
		trainflag=gesture_train(3,fdc2214temp);
		POINT_COLOR=GREEN;
		if(trainflag==0)
			LCD_ShowString(30,375,200,12,12,"3:gesture 3 trained!!!");
		LCD_ShowNum(180,190,mean_g[2]*100,3,16);
	}
	else if(equal2(row,column,2,1)){//4
		printf("for Traning of gesture 4.\r\n");
		POINT_COLOR=YELLOW;
		LCD_ShowString(30,390,200,12,12,"4:gesture 4 training.");
		trainflag=gesture_train(4,fdc2214temp);
		POINT_COLOR=GREEN;
		if(trainflag==0)
			LCD_ShowString(30,390,200,12,12,"4:gesture 4 trained!!!");
		LCD_ShowNum(220,190,mean_g[3]*100,3,16);
	}
	else if(equal2(row,column,2,2)){//5
		printf("for Traning of gesture 5.\r\n");
		POINT_COLOR=YELLOW;
		LCD_ShowString(30,405,200,12,12,"5:gesture 5 training.");
		trainflag=gesture_train(5,fdc2214temp);
		POINT_COLOR=GREEN;
		if(trainflag==0)
			LCD_ShowString(30,405,200,12,12,"5:gesture 5 trained!!!");
		LCD_ShowNum(260,190,mean_g[4]*100,3,16);
	}
	else if(equal2(row,column,4,1)){//*
		printf("for Traning of gesture ST.\r\n");//最小
		POINT_COLOR=YELLOW;
		LCD_ShowString(30,285,200,12,12,"*:gesture ST training.");
		trainflag=gesture_train(6,fdc2214temp);
		POINT_COLOR=GREEN;
		if(trainflag==0)
			LCD_ShowString(30,285,200,12,12,"*:gesture ST trained!!!");
		LCD_ShowNum(100,170,mean_g[5]*100,3,16);
	}
	else if(equal2(row,column,4,3)){//#
		printf("for Traning of gesture JD.\r\n");//中间
		POINT_COLOR=YELLOW;
		LCD_ShowString(30,300,200,12,12,"*:gesture JD training.");
		trainflag=gesture_train(7,fdc2214temp);
		POINT_COLOR=GREEN;
		if(trainflag==0)
			LCD_ShowString(30,300,200,12,12,"*:gesture JD trained!!!");
		LCD_ShowNum(160,170,mean_g[6]*100,3,16);
	}
	else if(equal2(row,column,4,4)){//D
		printf("for Traning of gesture B.\r\n");//最大
		POINT_COLOR=YELLOW;
		LCD_ShowString(30,315,200,12,12,"*:gesture B training.");
		trainflag=gesture_train(8,fdc2214temp);
		POINT_COLOR=GREEN;
		if(trainflag==0)
			LCD_ShowString(30,315,200,12,12,"*:gesture B trained!!!");
		LCD_ShowNum(220,170,mean_g[7]*100,3,16);
	}
	//判定按键：
	else if(equal2(row,column,1,4)){//A, For Detection of ST/JD/B.
		printf("For Detection of STJDB.\r\n");
		for(i=0;i<DSAMPLETIMES;i++){
			measurebuffer[i]=Cap_Calculate(0)-fdc2214temp; // CH0 detect for 5 times for mean.
			if(compensation==1)
				measurebuffer[i]-=ZCap_Calculate(0);
			delay_ms(50);
		}
		fdc2214in=mean(measurebuffer,DSAMPLETIMES);
		
		if(fdc2214in < (mean_g[6-1]+mean_g[7-1])/2 ){//ST，石头电容值最小
			POINT_COLOR=BLUE;
			LCD_ShowString(30,30,210,16,16,"Gesture Recognized:  ");
			LCD_ShowString(260,30,100,16,16,"ST ");
			printf("recognized : gesture ST .\r\n");
		}
		else if(fdc2214in < (mean_g[7-1]+mean_g[8-1])/2){
			POINT_COLOR=BLUE;
			LCD_ShowString(30,30,210,16,16,"Gesture Recognized:  ");
			LCD_ShowString(260,30,100,16,16,"JD ");
			printf("recognized : gesture JD .\r\n");
		}
		else{										//B，布电容值最大
			POINT_COLOR=BLUE;
			LCD_ShowString(30,30,210,16,16,"Gesture Recognized:  ");
			LCD_ShowString(260,30,100,16,16,"B  ");
			printf("recognized : gesture B .\r\n");
			//LCD_ShowNum(220,170,mean_g[7]*100,3,16);
		}

	}
	else if(equal2(row,column,2,4)){//B, For Detection of 12345.
		printf("For Detection of 12345.\r\n");
		for(i=0;i<DSAMPLETIMES;i++){
			measurebuffer[i]=Cap_Calculate(0)-fdc2214temp; // CH0 detect for 5 times for mean.
			if(compensation==1)
				measurebuffer[i]-=ZCap_Calculate(0);
			delay_ms(50);
		}
		fdc2214in=mean(measurebuffer,DSAMPLETIMES);
		for(i=1;i<6;i++){
			if(i<5 && (fdc2214in <= (mean_g[i-1]+mean_g[i])/2) ){//fabs(fdc2214in-mean_g[i-1])<=unst_g[i-1]
				POINT_COLOR=BLUE;
				LCD_ShowString(30,30,210,16,16,"Gesture Recognized:  ");
				LCD_ShowNum(260,30,i,1,16);
				printf("recognized : gesture %d.\r\n",i);
				break;
			}
			if(i==5){
				if(fdc2214in > (mean_g[i-2]+mean_g[i-1])/2){
					printf("recognized : gesture %d.\r\n",i);
					POINT_COLOR=BLUE;
					LCD_ShowString(30,30,180,16,16,"Gesture Recognized:  ");
					LCD_ShowNum(260,30,i,1,16);
				}
			}
		}
		
	}
	//其他功能性按键：//9,0,C,7,8
	else if(equal2(row,column,3,3)){//9,printf existed data.
		int i=0;
		for(i=0;i<5;i++){
			printf("Mean of gesture %d = %f, Uncertainty = %f\r\n",i+1,mean_g[i],unst_g[i]);
		}
		printf("Mean of gesture JD = %f, Uncertainty = %f\r\n",mean_g[5],unst_g[5]);
		printf("Mean of gesture ST = %f, Uncertainty = %f\r\n",mean_g[6],unst_g[6]);
		printf("Mean of gesture  B = %f, Uncertainty = %f\r\n",mean_g[7],unst_g[7]);
		POINT_COLOR=BLACK;
		LCD_ShowString(30,170,140,16,16,"STJDB C: ");
		LCD_ShowNum(100,170,mean_g[5]*100,3,16);
		LCD_ShowNum(160,170,mean_g[6]*100,3,16);
		LCD_ShowNum(220,170,mean_g[7]*100,3,16);
		LCD_ShowString(30,190,140,16,16,"12345 C: ");
		LCD_ShowNum(100,190,mean_g[0]*100,3,16);
		LCD_ShowNum(140,190,mean_g[1]*100,3,16);
		LCD_ShowNum(180,190,mean_g[2]*100,3,16);
		LCD_ShowNum(220,190,mean_g[3]*100,3,16);
		LCD_ShowNum(260,190,mean_g[4]*100,3,16);
	}
	/*
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
	}*/
	
	/*
	else if(equal2(row,column,3,1)){//7
		printfmode++;
		if(printfmode==5)
			printfmode =1;
	}
	else if(equal2(row,column,3,2)){//8
		compensation = 1;
	}*/
	return 0;
}

void LCD_Shining(){
	
	LCD_Clear(WHITE);
	POINT_COLOR=BLACK;	  
	if(compensation ==0)
		LCD_ShowString(30,10,210,16,16,"Current Mode: Normal");
	else
		LCD_ShowString(30,10,210,16,16,"Current Mode: Compensation");
	LCD_ShowString(30,30,210,16,16,"Gssture Recognized:  ");
		//LCD_ShowString(30,40,210,24,24,"Mode:Judging");
		//LCD_ShowString(30,70,200,16,16,"posture:paper");
		//LCD_ShowString(30,70,200,16,16,"posture:scissors");
	LCD_ShowString(30,50,210,16,16,"Temp C: ");
	POINT_COLOR=BLUE;
	LCD_ShowString(30,70,210,16,16,"Current C0: ");
	LCD_ShowString(30,90,210,16,16,"Current C1: ");
	LCD_ShowString(30,110,210,16,16,"Current C2: ");
	LCD_ShowString(30,130,210,16,16,"Current C3: ");
	LCD_ShowString(30,150,210,16,16,"Current C4: ");
	POINT_COLOR=BLACK;
	LCD_ShowString(30,170,210,16,16,"STJDB C: ");
	LCD_ShowString(30,190,210,16,16,"12345 C: ");
	if(train_status==0)
		LCD_ShowString(30,210,210,16,16,"Train Status: ing...");
	else if(train_status==1)
		LCD_ShowString(30,210,210,16,16,"Train Status: OK.");
	else if(train_status==-1)
		LCD_ShowString(30,210,210,16,16,"Train Status: Failed.");
	else
		LCD_ShowString(30,210,210,16,16,"Train Status: ");
	
 	//LCD_ShowString(30,130,210,24,24,lcd_id);		//显示LCD ID
	
	POINT_COLOR=BROWN;
	LCD_ShowString(30,250,180,16,16,"***** Function of Keys ******");
	LCD_ShowString(30,270,180,12,12,"A:Detection of STJDB");
	POINT_COLOR=RED;
	LCD_ShowString(30,285,180,12,12,"*:gesture ST UNtrained");
	LCD_ShowString(30,300,180,12,12,"#:gesture JD UNtrained");
	LCD_ShowString(30,315,180,12,12,"D:gesture  B UNtrained");
	POINT_COLOR=BROWN;
	LCD_ShowString(30,330,180,12,12,"B:Detection of 12345");
	POINT_COLOR=RED;
	LCD_ShowString(30,345,180,12,12,"1:gesture 1 UNtrained");
	LCD_ShowString(30,360,180,12,12,"2:gesture 2 UNtrained");
	LCD_ShowString(30,375,180,12,12,"3:gesture 3 UNtrained");
	LCD_ShowString(30,390,180,12,12,"4:gesture 4 UNtrained");
	LCD_ShowString(30,405,180,12,12,"5:gesture 5 UNtrained");

	LCD_ShowString(30,420,200,12,12,"9:show ALL training data");
	LCD_ShowString(30,435,180,12,12,"7:change printfmode");
	LCD_ShowString(30,450,240,12,12,"8:change compensation mode");
	
	//MAX=450
}

void LCD_ShowNormalC(){
	
	POINT_COLOR=BLACK;
	LCD_ShowString(30,70,210,16,16,"Current C0: ");
	LCD_ShowNum(120,70,C0*100,3,16);
	LCD_ShowString(170,70,100,16,16," Z=");
	LCD_ShowNum(210,70,ZC0*100,3,16);
	LCD_ShowString(250,70,100,16,16,"/100 pf");
	
	LCD_ShowString(30,90,210,16,16,"Current C1: ");
	LCD_ShowNum(120,90,C1*100,3,16);
	LCD_ShowString(170,90,100,16,16," Z=");
	LCD_ShowNum(210,90,ZC1*100,3,16);
	LCD_ShowString(250,90,100,16,16,"/100 pf");
	
	LCD_ShowString(30,110,210,16,16,"Current C2: ");
	LCD_ShowNum(120,110,C2*100,3,16);
	LCD_ShowString(170,110,100,16,16," Z=");
	LCD_ShowNum(210,110,ZC2*100,3,16);
	LCD_ShowString(250,110,100,16,16,"/100 pf");
	
	LCD_ShowString(30,130,210,16,16,"Current C3: ");
	LCD_ShowNum(120,130,C3*100,3,16);
	LCD_ShowString(170,130,100,16,16," Z=");
	LCD_ShowNum(210,130,ZC3*100,3,16);
	LCD_ShowString(250,130,100,16,16,"/100 pf");
	
	LCD_ShowString(30,150,210,16,16,"Current C4: ");
	LCD_ShowNum(120,150,C4*100,3,16);
	LCD_ShowString(170,150,100,16,16," Z=");
	LCD_ShowNum(210,150,ZC4*100,3,16);
	LCD_ShowString(250,150,100,16,16,"/100 pf");
}

int main(void)
{ 
	int mine_or_others=1;
	int row=0;
	int column=0;
	//int i=0;
	
	float res0;//float res1,res2,res3;
	float Zres0;//float Zres1,Zres2,Zres3;
	float temp0;//float temp1,temp2,temp3;
	float Ztemp0;//float Ztemp1,Ztemp2,Ztemp3;
	
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);//设置系统中断优先级分组2
	delay_init(168);  			//初始化延时函数
	uart_init(9600);			//初始化串口波特率为50,0000//460800:OK
	LED_Init();					//初始化LED 
	KEY_Init();					//初始化按键
 	LCD_Init();				    //LCD初始化
	//TIM3_Int_Init(1000-1,8400-1);	//Timer for PID 定时器时钟84M，分频系数8400，所以84M/8400=10Khz的计数频率，计数1000次为100ms，every 0.1s一次中断 
	MATRI4X4KEY_Init();
	while(FDC2214_Init());
	while(ZFDC2214_Init());
	printf("Init OK \r\n.");
	delay_ms(1000);//延时1s使得这是手可以缩回防止手放在复位按钮时影响电容。
	
	if(PFin(1)==0)
		compensation=1;
	else
		compensation=0;
	
	if(PFin(3)==1&&PFin(5)==1)
		printfmode=1;
	else if(PFin(3)==1&&PFin(5)==0)
		printfmode=2;
	else if(PFin(3)==0&&PFin(5)==1)
		printfmode=3;
	else if(PFin(3)==0&&PFin(5)==0)
		printfmode=4;
	
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
	
	LCD_Shining();
	LCD_ShowString(30,50,100,16,16,"C:temp0=");
	LCD_ShowNum(120,50,temp0,3,16);
	LCD_ShowString(170,50,100,16,16," Z=");
	LCD_ShowNum(210,50,Ztemp0,3,16);
	LCD_ShowString(260,50,100,16,16," pf");
	
 	while(1)
	{
		//LED_Off();
		LCD_ShowString(30,210,210,16,16,"Train Status:         ");
		
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

		C4=C3;
		C3=C2;
		C2=C1;
		C1=C0;
		C0=Cap_Calculate(0)-temp0;
		ZC4=ZC3;
		ZC3=ZC2;
		ZC2=ZC1;
		ZC1=ZC0;
		ZC0=Zres0-Ztemp0;
		
		LCD_ShowNormalC();//类似于以下4个printf
		
		//printf("C0-C4:%f,%f,%f,%f,%f\n",C0,C1,C2,C3,C4);
		//printf("CH0;%3.3f CH1;%3.3f CH2;%3.3f CH3;%3.3f. \n",res0-temp0,res1-temp1,res2-temp2,res3-temp3);
		//printf("ZCH0;%3.3f ZCH1;%3.3f ZCH2;%3.3f ZCH3;%3.3f. \n",Zres0-Ztemp0,Zres1-Ztemp1,Zres2-Ztemp2,Zres3-Ztemp3);
		if(printfmode ==2)printf("Compensation CH = %3.3f \r\n",res0-2*Zres0-(temp0-2*Ztemp0));
		if(printfmode ==1)printf("CH0- = %3.3f, ZCH0- = %3.3f \r\n",res0-temp0,Zres0-Ztemp0);
		if(printfmode ==3)printf("CH0 res=%f, ZCH0 res=%f \r\n",res0,Zres0);
		if(printfmode ==4)printf("CH0 abs =%3.3f, ZCH0 abs =%3.3f \r\n",fabs(res0-temp0),fabs(Zres0-Ztemp0));
		
		if(MATRI4_4KEY_Scan(&row, &column)==0)
			printf("key pressed row=%d, column=%d \r\n",row,column);
		
		if(mine_or_others==1){
			if(compensation==1)
				Parse_Key(row,column,temp0-Ztemp0);
			else
				Parse_Key(row,column,temp0);
		}
		
		delay_ms(800);
	} 	
}
