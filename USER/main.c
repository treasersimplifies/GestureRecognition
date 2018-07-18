#include "sys.h"
#include "delay.h"
#include "usart.h"
#include "led.h"
#include "key.h"
//#include "lcd.h"
#include "timer.h"
#include "adc.h"
#include "dac.h"
#include "mpu6050.h"
#include "inv_mpu.h"
#include "inv_mpu_dmp_motion_driver.h" 
#include "mypid.h"
#include "matrixkey.h"
//Pins used：
//1.PB9,PB8 : I2C for MPU6050
//2.PC0 : INT for MPU6050
//3.PA5 : ADC Input  CH5, 12bit mode
//4.PA4 : DAC Output CH1, 12bit
//5.PB0-PB7 : 4X4 matrix keys

//Abilities:
//1.PID 采样频率由TIM3定时器决定
//



//串口1发送1个字符 
//c:要发送的字符
void usart1_send_char(u8 c)
{

	while(USART_GetFlagStatus(USART1,USART_FLAG_TC)==RESET); 
    USART_SendData(USART1,c);   

} 
//传送数据给匿名四轴上位机软件(V2.6版本)
//fun:功能字. 0XA0~0XAF
//data:数据缓存区,最多28字节!!
//len:data区有效数据个数
void usart1_niming_report(u8 fun,u8*data,u8 len)
{
	u8 send_buf[32];
	u8 i;
	if(len>28)return;	//最多28字节数据 
	send_buf[len+3]=0;	//校验数置零
	send_buf[0]=0X88;	//帧头
	send_buf[1]=fun;	//功能字
	send_buf[2]=len;	//数据长度
	for(i=0;i<len;i++)send_buf[3+i]=data[i];			//复制数据
	for(i=0;i<len+3;i++)send_buf[len+3]+=send_buf[i];	//计算校验和	
	for(i=0;i<len+4;i++)usart1_send_char(send_buf[i]);	//发送数据到串口1 
}
//发送加速度传感器数据和陀螺仪数据
//aacx,aacy,aacz:x,y,z三个方向上面的加速度值
//gyrox,gyroy,gyroz:x,y,z三个方向上面的陀螺仪值
void mpu6050_send_data(short aacx,short aacy,short aacz,short gyrox,short gyroy,short gyroz)
{
	u8 tbuf[12]; 
	tbuf[0]=(aacx>>8)&0XFF;
	tbuf[1]=aacx&0XFF;
	tbuf[2]=(aacy>>8)&0XFF;
	tbuf[3]=aacy&0XFF;
	tbuf[4]=(aacz>>8)&0XFF;
	tbuf[5]=aacz&0XFF; 
	tbuf[6]=(gyrox>>8)&0XFF;
	tbuf[7]=gyrox&0XFF;
	tbuf[8]=(gyroy>>8)&0XFF;
	tbuf[9]=gyroy&0XFF;
	tbuf[10]=(gyroz>>8)&0XFF;
	tbuf[11]=gyroz&0XFF;
	usart1_niming_report(0XA1,tbuf,12);//自定义帧,0XA1
}	
//通过串口1上报结算后的姿态数据给电脑
//aacx,aacy,aacz:x,y,z三个方向上面的加速度值
//gyrox,gyroy,gyroz:x,y,z三个方向上面的陀螺仪值
//roll:横滚角.单位0.01度。 -18000 -> 18000 对应 -180.00  ->  180.00度
//pitch:俯仰角.单位 0.01度。-9000 - 9000 对应 -90.00 -> 90.00 度
//yaw:航向角.单位为0.1度 0 -> 3600  对应 0 -> 360.0度
void usart1_report_imu(short aacx,short aacy,short aacz,short gyrox,short gyroy,short gyroz,short roll,short pitch,short yaw)
{
	u8 tbuf[28]; 
	u8 i;
	for(i=0;i<28;i++)tbuf[i]=0;//清0
	tbuf[0]=(aacx>>8)&0XFF;
	tbuf[1]=aacx&0XFF;
	tbuf[2]=(aacy>>8)&0XFF;
	tbuf[3]=aacy&0XFF;
	tbuf[4]=(aacz>>8)&0XFF;
	tbuf[5]=aacz&0XFF; 
	tbuf[6]=(gyrox>>8)&0XFF;
	tbuf[7]=gyrox&0XFF;
	tbuf[8]=(gyroy>>8)&0XFF;
	tbuf[9]=gyroy&0XFF;
	tbuf[10]=(gyroz>>8)&0XFF;
	tbuf[11]=gyroz&0XFF;	
	tbuf[18]=(roll>>8)&0XFF;
	tbuf[19]=roll&0XFF;
	tbuf[20]=(pitch>>8)&0XFF;
	tbuf[21]=pitch&0XFF;
	tbuf[22]=(yaw>>8)&0XFF;
	tbuf[23]=yaw&0XFF;
	usart1_niming_report(0XAF,tbuf,28);//飞控显示帧,0XAF
} 

void MPU6050_MUST_Init(){
	while(mpu_dmp_init())// =0 means success
	{
		/*此处加入初始化失败标志*/
		
		//LCD_ShowString(30,130,200,16,16,"MPU6050 Error");
		//delay_ms(200);
		//LCD_Fill(30,130,239,130+16,WHITE);
 		delay_ms(200);
		printf("MPU dmp init failing....\n");
	}
}

void MPU6050_Perform(){

}

int main(void)
{ 
	//u8 report=1;			//默认开启上报
	//u8 key;
	//u16 adcx;
	u16 dacval=0;
	//float adcvalue;
	
	float pitch,roll,yaw; 		//欧拉角
	short aacx,aacy,aacz;		//加速度传感器原始数据
	short gyrox,gyroy,gyroz;	//陀螺仪原始数据
	short temp;					//温度
	
	extern double pidin,pidout,pidref;
	
	int row=0;
	int column=0;
	
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);//设置系统中断优先级分组2
	delay_init(168);  			//初始化延时函数
	uart_init(460800);			//初始化串口波特率为50,0000//460800:OK
	LED_Init();					//初始化LED 
	KEY_Init();					//初始化按键
 	//LCD_Init();				//LCD初始化
	MPU_Init();					//初始化MPU6050
	Adc_Init();         		//初始化ADC, 0-4095
	Dac1_Init();		 		//DAC通道1初始化	,0-4095
	TIM3_Int_Init(1000-1,8400-1);	//定时器时钟84M，分频系数8400，所以84M/8400=10Khz的计数频率，计数1000次为100ms，every 0.1s一次中断 
	//delay_ms(150);
	MATRI4X4KEY_Init();
	
	pidref=1000;
	PID_Init(&pidin,&pidout,&pidref,2,0,0, P_ON_M, DIRECT);//p,i,d=1,1,1
	SetOutputLimits(1,500);
	
	//MPU6050_MUST_Init();
	DAC_SetChannel1Data(DAC_Align_12b_R,dacval);//初始值为0
	//dacval=2000;
	
 	while(1)
	{
		printf("running....\n");
		delay_ms(500);
		//if(MATRI4_4KEY_Scan(&row, &column)==0)
			//printf("key pressed row=%d, column=%d\n",row,column);
		
		//***************************

		/* MPU6050 measure part */
		if(mpu_dmp_get_data(&pitch,&roll,&yaw)==0)//=0 means success
		{ 
			temp=MPU_Get_Temperature();					//得到温度值
			MPU_Get_Accelerometer(&aacx,&aacy,&aacz);	//得到加速度传感器数据
			MPU_Get_Gyroscope(&gyrox,&gyroy,&gyroz);	//得到陀螺仪数据
			//if(report)mpu6050_send_data(aacx,aacy,aacz,gyrox,gyroy,gyroz);//用自定义帧发送加速度和陀螺仪原始数据
			//if(report)usart1_report_imu(aacx,aacy,aacz,gyrox,gyroy,gyroz,(int)(roll*100),(int)(pitch*100),(int)(yaw*10));
			//以上两句不影响串口输出
			printf("pitch=%f,roll=%f,yaw=%f;\n",pitch,roll,yaw);//printf函数的使用不会影响到匿名四轴上位机对数据的接受。
			//因为Usart_send等函数就算是波特率对了，其编码也是和printf用的是不同的，所以无法在串口程序里查看。
			
			delay_ms(100);
			//delay函数的使用会使得匿名四轴上位机动态显示看上去很卡。
			//但是有利于人眼查看串口数据。
			
		}
		

	} 	
}
