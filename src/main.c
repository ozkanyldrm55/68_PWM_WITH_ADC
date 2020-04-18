/*PA0 bacagina potansiyometre ADC ve degerini degistirirsek ledlerin parlakligi degisecek*/

#include "stm32f4xx.h"
#include "stm32f4_discovery.h"

GPIO_InitTypeDef GPIO_InitStruct;

TIM_TimeBaseInitTypeDef TIM_InitStruct;
TIM_OCInitTypeDef TIMOC_InitStruct; // PWM icin

ADC_InitTypeDef ADC_InitStruct;
ADC_CommonInitTypeDef ADC_CommonInitStruct;

uint16_t adc_value;
uint8_t pwm_value = 0;

/* ADC_Resolution_12b yani 0 - 4096 arasinda degerler alir ancak pulse degerimiz max 99 dur.
   uint8_t map fonk ile 0 - 4096 arasindaki degerleri 0 - 99 arasina uyarliyoruz. 
   pwm_value = map(adc_value, 0 , 4095 , 0, 99) */

uint8_t map(uint32_t A , uint32_t B , uint32_t C , uint32_t D , uint32_t E)
{
	return (A * E) / C;
}

void GPIO_Config() {
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD,ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA,ENABLE);

	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF;  // timer olarak kullanildigi icin AF olarak kullandik.
	GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15;
	GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_100MHz;

	GPIO_Init(GPIOD , &GPIO_InitStruct);

	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AN;
	GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_0;
	GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_100MHz;

	GPIO_Init(GPIOA , &GPIO_InitStruct);

	/*ledler 12-13-14-15 timer4 un channellari bagli oldugu icin timer 4 u kullandik*/
	GPIO_PinAFConfig(GPIOD,GPIO_PinSource12,GPIO_AF_TIM4);
	GPIO_PinAFConfig(GPIOD,GPIO_PinSource13,GPIO_AF_TIM4);
	GPIO_PinAFConfig(GPIOD,GPIO_PinSource14,GPIO_AF_TIM4);
	GPIO_PinAFConfig(GPIOD,GPIO_PinSource15,GPIO_AF_TIM4);
}

void TIM_Config() {
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4,ENABLE);

	/* PWM_FREQ = 10KHz olmasini istedik 10000mhz
	 *
	 * tim_tic = 84000000 / (psc+1) = 1000000 , psc = 83
	 *
	 * periyod = (1000000 / 10000=PWM_FREQ)-1 = 99
	 
	   pulse = (periyod + 1) * (duty cycl %) - 1
	 **/
	
	TIM_InitStruct.TIM_ClockDivision = TIM_CKD_DIV1;
	TIM_InitStruct.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_InitStruct.TIM_Period = 99;
	TIM_InitStruct.TIM_Prescaler = 83;
	TIM_InitStruct.TIM_RepetitionCounter = 0;

	TIM_TimeBaseInit(TIM4 , &TIM_InitStruct);
	TIM_Cmd(TIM4,ENABLE);

	TIMOC_InitStruct.TIM_OCMode = TIM_OCMode_PWM1;	// PWM mod1 mod2 muhabbeti , doluluk oranlarýyla ilgili
	TIMOC_InitStruct.TIM_OutputState = ENABLE; 	// cikisi aktif ettik
	TIMOC_InitStruct.TIM_OCNPolarity = TIM_OCNPolarity_High;
}

void ADC_Config(void) {
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1,ENABLE);

	ADC_CommonInitStruct.ADC_Mode = ADC_Mode_Independent;	 // ADC modunu bagimsiz olarak tanimladik
	ADC_CommonInitStruct.ADC_Prescaler = ADC_Prescaler_Div4; // APB2 84MHz dir.ancak adc en fazla 36 MHz ile calistigi icin 84/4 yaptik
	ADC_CommonInit(&ADC_CommonInitStruct);

	ADC_InitStruct.ADC_Resolution = ADC_Resolution_12b; 	// cozunurlugunu max ayarladik ancak bu durum cok da iyi degil ses vs herseyden etkilenir
	ADC_Init(ADC1 , &ADC_InitStruct);
	ADC_Cmd(ADC1,ENABLE); // cevresel paramerteleri kullandigimizda bu sekilde aktif etmeliyiz
}

uint16_t Read_ADC()
{
	ADC_RegularChannelConfig(ADC1,ADC_Channel_0,1,ADC_SampleTime_56Cycles); /*adc1 de okuma yapacagiz,
	A portunun 0.pinine bagli oldugu icin ADC_Channel_0 , 1 tane adc okumasi yapacagiz ,adc ornekleme suresi 56 cycles demektir */

	ADC_SoftwareStartConv(ADC1); // ADC yi yazilimsal olarak baslattik 
				     //adc okumasi bittiginde EOC bayragi kalkar asagida while ile bu durum olusana kadar bekledik
	while(ADC_GetFlagStatus(ADC1,ADC_FLAG_EOC) == RESET); // adc okumasi bitene kadar beklememiz lazim burada o islemi gerceklestiriyoruz.
							      // ADC okumasi bittiginde EOC Flag i kalkmaktadir.
	return ADC_GetConversionValue(ADC1); // ADC degisim degerlerini okumak icin bu degerleri kullandik.yani adc degerini cektik
}

int main(void) {
	GPIO_Config();
	TIM_Config();
	ADC_Config();

  while (1) {
	adc_value = Read_ADC();
	pwm_value = map(adc_value, 0 , 4095 , 0, 99);
	/* pwm pulse degerini 99 olarak belirledik ancak adc 12 bit oldugu icin 4095 e kadar sayar.
	 * bu iki degeri birbirine aranlamak icin bu fonk yazdik */

	  // PD12
	  TIMOC_InitStruct.TIM_Pulse = pwm_value;
	  TIM_OC1Init(TIM4,&TIMOC_InitStruct);
	  TIM_OC1PreloadConfig(TIM4,TIM_OCPreload_Enable); //TIM4 -> CH1 ENABLE

	  // PD13
	  TIMOC_InitStruct.TIM_Pulse = pwm_value;
	  TIM_OC2Init(TIM4,&TIMOC_InitStruct);
	  TIM_OC2PreloadConfig(TIM4,TIM_OCPreload_Enable); //TIM4 -> CH2 ENABLE

	  // PD14
	  TIMOC_InitStruct.TIM_Pulse = pwm_value;
	  TIM_OC3Init(TIM4,&TIMOC_InitStruct);
	  TIM_OC3PreloadConfig(TIM4,TIM_OCPreload_Enable); //TIM4 -> CH3 ENABLE

	  // PD15
	  TIMOC_InitStruct.TIM_Pulse = pwm_value;
	  TIM_OC4Init(TIM4,&TIMOC_InitStruct);
	  TIM_OC4PreloadConfig(TIM4,TIM_OCPreload_Enable); //TIM4 -> CH4 ENABLE
  }
}

void EVAL_AUDIO_TransferComplete_CallBack(uint32_t pBuffer, uint32_t Size){
  /* TODO, implement your code here */
  return;
}

/*
 * Callback used by stm324xg_eval_audio_codec.c.
 * Refer to stm324xg_eval_audio_codec.h for more info.
 */
uint16_t EVAL_AUDIO_GetSampleCallBack(void){
  /* TODO, implement your code here */
  return -1;
}
