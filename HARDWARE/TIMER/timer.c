#include "delay.h"
#include "sys.h"
#include "timer.h"
#include "led.h"
#include "dac.h"
#include "adc.h"
#include "usart.h"
#include "mypid.h"
//通用定时器3中断初始化
//arr：自动重装值。
//psc：时钟预分频数
//定时器溢出时间计算方法:Tout=((arr+1)*(psc+1))/Ft us.
//Ft=定时器工作频率,单位:Mhz
//这里使用的是定时器3!
void TIM3_Int_Init(u16 arr,u16 psc)
{
	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3,ENABLE);  ///使能TIM3时钟
	
	TIM_TimeBaseInitStructure.TIM_Period = arr; 	//自动重装载值
	TIM_TimeBaseInitStructure.TIM_Prescaler=psc;  //定时器分频
	TIM_TimeBaseInitStructure.TIM_CounterMode=TIM_CounterMode_Up; //向上计数模式
	TIM_TimeBaseInitStructure.TIM_ClockDivision=TIM_CKD_DIV1; 
	
	TIM_TimeBaseInit(TIM3,&TIM_TimeBaseInitStructure);//初始化TIM3
	
	TIM_ITConfig(TIM3,TIM_IT_Update,ENABLE); //允许定时器3更新中断
	TIM_Cmd(TIM3,ENABLE); //使能定时器3
	
	NVIC_InitStructure.NVIC_IRQChannel=TIM3_IRQn; //定时器3中断
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=0x01; //抢占优先级1
	NVIC_InitStructure.NVIC_IRQChannelSubPriority=0x03; //子优先级3
	NVIC_InitStructure.NVIC_IRQChannelCmd=ENABLE;
	NVIC_Init(&NVIC_InitStructure);
	
}

double pidin,pidout,pidref;
long i=0;
//定时器3中断服务函数
void TIM3_IRQHandler(void)
{	
	
	u16 adcx;
	u16 dacval=0;
	u16 adc_raw=0;
	float adcvalue;
	dacval=1000;
	if(i%100==0){//采样100次，10s 打印一下PID设置
		PID_ShowConfig();
		delay_ms(1000);
	}
	//printf("Into interupt of TIM3 \n");
	if(TIM_GetITStatus(TIM3,TIM_IT_Update)==SET) //溢出中断
	{	//在此处点名中断时要处理的内容
		
		
		/*  DAC output part  */
		//DAC_SetChannel1Data(DAC_Align_12b_R, dacval);//设置DAC输出值
		
		/*  ADC read in part  */
		adc_raw=Get_Adc_Average(ADC_Channel_5,20);	  //获取通道5的转换值，20次取平均
		adcvalue=(float)adc_raw*(3.3/4096);          //获取计算后的带小数的实际电压值，比如3.1111
		adcx=adcvalue;                            	//赋值整数部分给adcx变量，因为adcx为u16整形
		//delay_ms(250);	
		printf("ADC read in : %f \n",adcvalue);
		
		/* PID part */
		//experiment : PA5/ PA4接在一起，控制PA5输出
		pidin = adc_raw;
		PID_Compute();
		DAC_SetChannel1Data(DAC_Align_12b_R, pidout);
		printf("PID in = %f, PID out = %f",pidin,pidout);
		
	}
	TIM_ClearITPendingBit(TIM3,TIM_IT_Update);  //清除中断标志位
	i++;
	if(i>50000)i=0;
}







