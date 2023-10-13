/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>

#include "main.h"

#include "ssd1306.h"
#include "fonts.h"

#include "DS3231.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define ADC_JOY_X (adc_value[1])
#define ADC_JOY_Y (adc_value[0])

#define JOY_R (ADC_JOY_X > 4000)
#define JOY_L (ADC_JOY_X < 300)
#define JOY_U (ADC_JOY_Y > 4000)
#define JOY_D (ADC_JOY_Y < 300)
#define JOY_P (!HAL_GPIO_ReadPin(JOY_SW_GPIO_Port, JOY_SW_Pin))
#define LED_101(n) (HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, n))
#define LED_102(n) (HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, n))
#define LED_103(n) (HAL_GPIO_WritePin(LED3_GPIO_Port, LED3_Pin, n))
#define LED_104(n) (HAL_GPIO_WritePin(LED4_GPIO_Port, LED4_Pin, n))
#define BUZ(x) (HAL_GPIO_WritePin(BUZ_GPIO_Port, BUZ_Pin, x))
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;
DMA_HandleTypeDef hdma_adc1;

I2C_HandleTypeDef hi2c1;

/* USER CODE BEGIN PV */
uint16_t adc_value[2], cnt=0,joy, joy_delay1, joy_delay2;
uint8_t joystick,sys=0,init_block=0, use=1, room_sel=0, dis_sel=0, joy_old=0;

uint8_t time_C=0, Cursor=2 ,Cursor_H=2, Cursor_P=0, Cursor_R=0;



uint8_t md_max[4]={50,30,25,20}, md_now[4]={4,1,5,3};

char bf[30];
char *TIME[6]={"YEAR","MONTH","DAY","HOUR","MIN","SEC"};
char *PS[4]={"(Pain killer)","(Anti-inflam)","(IV)","(Fever remedy)"};
char *DISEASE[5]={"[unknown]", "[cold]","[flu]","[enteritis]","[high fever]"};
const char keyboard[4][12] = {
    "1234567890&",
    "QWERTYUOIP ",
    "ASDFGHJKL;*",
    "ZXCVBNM,.  "
};
char keyboard_mem[11];
char LASTDAY[13]={0,31,28,31,30,31,30,31,31,30,31,30,31};
uint8_t set_time[6]={0,1,1,0,0,0}, LED_status[4]={0};
enum date{
	year=0,month,day,hour,min,sec
};
enum joystick_enum{
	nomal=0,up,down,left,right
};
enum system_enum{
	init=0,time_set,main_menu,hospit,discharge,mm,pm,log,emergency_call
};
typedef struct
{
   char name[11];
   uint8_t Disease,Presc, store, use,time101,time102,time103,time104;
} Part;
Part pati_save[7];

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_I2C1_Init(void);
static void MX_ADC1_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void HAL_IncTick(){
	uwTick += uwTickFreq;
	cnt++;
	if(sys==init && init_block < 11 && cnt % 200 == 0) init_block++;
	if(sys != time_set && sys != init && cnt % 1000 == 0){
	        set_time[5]++;
	        if(set_time[5]>59){
	        	set_time[5]=0;
	        	set_time[4]++;
	        	if(set_time[4]>59){
	        		set_time[4]=0;
	        		set_time[3]++;
	        		if(set_time[3]>23){
	        			set_time[3]=0;
	        			set_time[2]++;
	        		}
	        	}
	        }
	}


	if(cnt > 30000) joy_delay1 = joy_delay2 = cnt = 0;

	if(uwTick % 5 == 0){
		HAL_ADC_Start(&hadc1);
		HAL_ADC_PollForConversion(&hadc1, 10);
		adc_value[0] = HAL_ADC_GetValue(&hadc1);
		HAL_ADC_Start(&hadc1);
		HAL_ADC_PollForConversion(&hadc1, 10);
		adc_value[1] = HAL_ADC_GetValue(&hadc1);
		if(JOY_U)
			joystick = up;
		else if(JOY_D)
			joystick = down;
		else if(JOY_R)
			joystick = right;
		else if(JOY_L)
			joystick = left;
		else
			joystick = nomal;
	}

}
uint8_t joystick_sw(){//조이?��?�� ?���?????? ?��?��
   if(!HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_2)){
      while(!HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_2)) HAL_Delay(50);
      return 1;
   }
   return 0;
}

void PutsXY(int x, int y, char* str, int color){
   SSD1306_GotoXY(x*6, y*8);
   SSD1306_Puts(str, &Font_6x8, color);
}
void ud_joystick(uint8_t *sel, uint16_t max, uint8_t rotate, uint8_t min, uint8_t sort){
   if(joystick){
	if(joy_old==RESET){
        if(sort){
           if(*sel < max+min && joystick == up)
              *sel += 1;
           else if(*sel > min && joystick == down)
              *sel -= 1;
           else if(*sel == max+min && joystick == up && rotate)
              *sel = min;
           else if(*sel == 0 && joystick == down && rotate)
              *sel = max;
        }
        else{
           if(*sel > min && joystick == up)
              *sel -= 1;
           else if(*sel < max+min && joystick == down)
              *sel += 1;
           else if(*sel == 0 && joystick == up && rotate)
              *sel = max;
           else if(*sel == max+min && joystick == down && rotate)
              *sel = min;
        }
        joy_delay1 = cnt;
	}
	else if(cnt - joy_delay1 > 200){
         if(sort){
            if(*sel < max+min && joystick == up)
               *sel += 1;
            else if(*sel > min && joystick == down)
               *sel -= 1;
            else if(*sel == max+min && joystick == up && rotate)
               *sel = min;
            else if(*sel == 0 && joystick == down && rotate)
               *sel = max;
         }
         else{
            if(*sel > min && joystick == up)
               *sel -= 1;
            else if(*sel < max+min && joystick == down)
               *sel += 1;
            else if(*sel == 0 && joystick == up && rotate)
               *sel = max;
            else if(*sel == max+min && joystick == down && rotate)
               *sel = min;
         }
         joy_delay1 = cnt;
      }
   }
   joy_old = joystick;
}

void rl_joystick(uint8_t *les, uint8_t max, uint8_t rotate, uint8_t min, uint8_t sort){
	if(joystick){
		if(joy_old==RESET){
			if(sort){
				if(*les < max+min && joystick == right)
				*les += 1;
				else if(*les > min && joystick == left)
				*les -= 1;
				else if(*les == max+min && joystick == right && rotate)
				*les = min;
				else if(*les == 0 && joystick == left && rotate)
				*les = max;
			}
			else{
				if(*les > min && joystick == right)
				*les -= 1;
				else if(*les < max+min && joystick == left)
				*les += 1;
				else if(*les == 0 && joystick == right && rotate)
				*les = max;
				else if(*les == max+min && joystick == left && rotate)
				*les = min;
			}
			joy_delay2 = cnt;
		}
		else if(cnt - joy_delay2 > 200){
			if(sort){
				if(*les < max+min && joystick == right)
				*les += 1;
				else if(*les > min && joystick == left)
				*les -= 1;
				else if(*les == max+min && joystick == right && rotate)
				*les = min;
				else if(*les == 0 && joystick == left && rotate)
				*les = max;
			}
			else{
				if(*les > min && joystick == right)
				*les -= 1;
				else if(*les < max+min && joystick == left)
				*les += 1;
				else if(*les == 0 && joystick == right && rotate)
				*les = max;
				else if(*les == max+min && joystick == left && rotate)
				*les = min;
			}
			joy_delay2 = cnt;
		}
	}
	joy_old = joystick;
}
void pati_save_(){
	strcat(pati_save[room_sel].name, keyboard_mem);
	pati_save[room_sel].Disease = dis_sel;
	pati_save[room_sel].Presc = Cursor_P;
	pati_save[room_sel].store = md_now[Cursor_P];
	pati_save[room_sel].use = use;
	if(room_sel==1){
		for(int i=0; i<6; i++){
			pati_save[i].time101 = set_time[i];
		}
	}
	if(room_sel==2){
		for(int i=0; i<6; i++){
			pati_save[i].time104 = set_time[i];
		}
	}
	if(room_sel==3){
		for(int i=0; i<6; i++){
			pati_save[i].time104 = set_time[i];
		}
	}
	if(room_sel==4){
		for(int i=0; i<6; i++){
			pati_save[i].time104 = set_time[i];
		}
	}

}
void KEYBOARD(){
	for(int j = 0; j < 12; j++){
		keyboard_mem[j]=0;
	}
	while(1){
		static uint8_t updown_sel = 0, line_sel, count = 0;
		SSD1306_Fill(0);
		PutsXY(0, 0, "#KEYBOARD                ", 0);
		PutsXY(0, 1, ">", 1);
		PutsXY(1+count, 2, "^", 1);
	    PutsXY(1, 1, keyboard_mem, 1);
	    for(int i = 0; i < 4; i++){
	       for(int j = 0; j < 11; j++){
	          sprintf(bf, "%c", keyboard[i][j]);
	          SSD1306_GotoXY((j+2) * 6 + (j*4), (i+4) * 8);
	          if(i == updown_sel && j == line_sel)
	             SSD1306_Puts(bf, &Font_6x8, 0);
	          else
	             SSD1306_Puts(bf, &Font_6x8, 1);
	       }
	    }
	    SSD1306_UpdateScreen();

	    ud_joystick(&updown_sel, 3, 1, 0, 0);
	    if(updown_sel % 2 == 0)
	            rl_joystick(&line_sel, 10, 1, 0, 0);//0,2번째 �??? 10칸까�???
	         else
	            rl_joystick(&line_sel, 9, 1, 0, 0);//1,3번�?? �??? 9칸까�??? 커서

	    if(joystick_sw()){
	         if(line_sel == 10){
	            if(updown_sel == 0){
	               if(count > 0){
	                  count--;
	                  keyboard_mem[count] = 0;
	               }
	            }
	            else if(updown_sel == 2){
	               updown_sel = 0;
	               line_sel = 0;
	               count = 0;
	               return;
	            }
	         }
	         else {
	            if(count < 10){
	               keyboard_mem[count] = keyboard[updown_sel][line_sel];
	               count++;
	            }
	         }
	      }
	      if(count >= 10){
	         updown_sel = 0;
	         line_sel = 0;
	         count = 0;
	         return;
	      }
	}
}


void Init(){
	SSD1306_Fill(0);
	PutsXY(2, 3,"Patient Management", 1);
	PutsXY(8, 4,"system", 1);
	SSD1306_GotoXY(2*6, 2*8+init_block-3);
	SSD1306_Puts("                  ", &Font_6x8, 1);
	SSD1306_GotoXY(2*6, 5*8-init_block+2);
	SSD1306_Puts("                  ", &Font_6x8, 1);
	SSD1306_UpdateScreen();
	if(init_block==11){
		HAL_Delay(300);
		PutsXY(8, 3,"Welcome", 1);
		SSD1306_UpdateScreen();
		HAL_Delay(2000);
		//////////////////////////////////////////eeprom set(first run)//////////////////////////////////////////
		sys=time_set;
	}
}
void Time_set(){
	SSD1306_Fill(0);
	PutsXY(0, 0, "#TIME SETTING             ", 0);
	PutsXY(0, 3, "RTC Time Setting", 1);
	sprintf(bf, "%s=%d",TIME[time_C],time_C==0 ? set_time[time_C] + 2000 : set_time[time_C]);
	SSD1306_GotoXY(0, 5*8);
	SSD1306_Puts(bf, &Font_6x8, 1);
	if(joystick_sw()){
		DS3231_set_date(set_time[2], set_time[1], set_time[0]);
		DS3231_set_time(set_time[5], set_time[4], set_time[3]);
		DS3231_get_time(&set_time[5], &set_time[4], &set_time[3]);
		DS3231_get_date(&set_time[2], &set_time[1], &set_time[0]);
		for(int i=0; i<6; i++) set_time[i]=set_time[i];
		sys = main_menu;
	}
	rl_joystick(&time_C, 5, 0, 0, 0);

	if(time_C==year)
		ud_joystick(&set_time[time_C], 99, 0, 0, 1);
	if(time_C==month)
		ud_joystick(&set_time[time_C], 11, 0, 1, 1);
	if(time_C==day){
		if(set_time[time_C]>LASTDAY[set_time[month]]) set_time[time_C]=LASTDAY[set_time[month]]; //if value over -> max value
		ud_joystick(&set_time[time_C], LASTDAY[set_time[month]]-1, 0, 1, 1);
	}
	if(time_C==hour)
		ud_joystick(&set_time[time_C], 23, 0, 0, 1);
	if(time_C==min)
		ud_joystick(&set_time[time_C], 23, 0, 0, 1);
	if(time_C==sec)
		ud_joystick(&set_time[time_C], 59, 0, 0, 1);

	SSD1306_UpdateScreen();
}
void Main_menu(){
	SSD1306_Fill(0);
	PutsXY(0, 0, "#main menu            ", 0);
	PutsXY(1, 2, "pati Hospit", 1);
	PutsXY(1, 3, "Discharge", 1);
	PutsXY(1, 4, "MM", 1);
	PutsXY(1, 5, "PM", 1);
	PutsXY(1, 6, "Log", 1);
	sprintf(bf, "20%02d.%02d.%02d", set_time[0], set_time[1], set_time[2]);
	PutsXY(11, 4, bf, 1);
	sprintf(bf, "%02d:%02d:%02d", set_time[3], set_time[4], set_time[5]);
	PutsXY(13, 5, bf, 1);
	ud_joystick(&Cursor, 4, 0, 2, 0);
	PutsXY(0, Cursor, ">", 1);
	SSD1306_UpdateScreen();
	if(joystick_sw()){
		sys = Cursor+1; //////emergency call 만들 ?�� 조건 걸어줘야 ?�� ///////////////////
	}


}
void Hospit(){
	SSD1306_Fill(0);
	PutsXY(0, 0, "#Patient Hospit             ", 0);
	PutsXY(1, 2, "Disease:", 1);
	if(keyboard_mem[0] == 0)
		PutsXY(1, 3, "Name:(NONE)", 1);
	else{
		PutsXY(1, 3, "Name:", 1);
		PutsXY(6, 3, keyboard_mem, 1);
	}
	PutsXY(1, 4, "Presc:", 1);
	PutsXY(1, 5, "Store:", 1);
	PutsXY(1, 6, "Hospit room:", 1);
	PutsXY(1, 7, "Enter", 1);
	SSD1306_GotoXY(7*6, 4*8);
	SSD1306_Puts(PS[Cursor_P], &Font_6x8, 1);
	SSD1306_GotoXY(7*6, 5*8);
	sprintf(bf ,"(%d/%d)",md_now[Cursor_P],md_max[Cursor_P]);
	SSD1306_Puts(bf, &Font_6x8, 1);
	sprintf(bf,"USE:%d",use);
	SSD1306_GotoXY(14*6, 5*8);
	SSD1306_Puts(bf, &Font_6x8, 1);
	ud_joystick(&Cursor_H, 5, 0, 2, 0);
	PutsXY(0, Cursor_H, ">", 1);
	PutsXY(9, 2, DISEASE[dis_sel], 1);
	sprintf(bf,"<10%d>",room_sel);
	SSD1306_GotoXY(13*6, 6*8);
	SSD1306_Puts(bf, &Font_6x8, 1);
	if(room_sel==0){
		PutsXY(13, 6, "<non>", 1);
	}else{
		sprintf(bf,"<10%d>",room_sel);
		SSD1306_GotoXY(13*6, 6*8);
		SSD1306_Puts(bf, &Font_6x8, 1);
	}
	if(Cursor_H == 2){
		rl_joystick(&dis_sel, 4, 0, 0, 0);
	}else if(Cursor_H == 4){
		rl_joystick(&Cursor_P, 3, 0, 0, 0);
		if(md_now[Cursor_P]<use) use=md_now[Cursor_P];
	}else if(Cursor_H == 5){
		rl_joystick(&use, md_now[Cursor_P]-1, 0, 1, 0);
	}else if(Cursor_H == 6){
		rl_joystick(&room_sel, 4, 0, 0, 0);
	}
	SSD1306_UpdateScreen();
	if(room_sel==0)
		LED_101(0);
	if(room_sel==1){
		LED_101(1);
		LED_102(0);
	}
	if(room_sel==2){
		LED_102(1);
		LED_101(0);
		LED_103(0);
	}
	if(room_sel==3){
		LED_103(1);
		LED_102(0);
		LED_104(0);
	}
	if(room_sel==4){
		LED_104(1);
		LED_103(0);
	}
	for(int i=1; i<5; i++){
		if(LED_status[i]==1){
			if(i==1)
				LED_101(1);
			if(i==2)
				LED_102(1);
			if(i==3)
				LED_103(1);
			if(i==4)
				LED_104(1);
		}
	}

	if(joystick_sw()){
		if(Cursor_H==3) KEYBOARD();
		else if(Cursor_H==7){
			if(room_sel==0){
				//log
			}else if(room_sel==1){
				pati_save_();
			}else if(room_sel==2){
				pati_save_();
			}else if(room_sel==3){
				pati_save_();
			}else if(room_sel==4){
				pati_save_();
			}
			//variable reset//
			Cursor_H=2;
			LED_status[room_sel]=1;
			Cursor_P=use=dis_sel=room_sel=0;
			sys = main_menu;
		}
	}
}

void Discharge(){
	SSD1306_Fill(0);
	PutsXY(0, 0, "#Discharge              ", 0);
	PutsXY(0, 2, "Hospit room:10", 1);

	ud_joystick(&Cursor_H, 5, 0, 2, 0);
	PutsXY(0, Cursor_H, ">", 1);
	if(Cursor_H==2)
		rl_joystick(&room_sel, 4, 0, 1, 0);

	SSD1306_UpdateScreen();
}
void Medicine_manage(){
	SSD1306_Fill(0);
	PutsXY(0, 0, "#Medicine manage          ", 0);
	PutsXY(0, 2, " ", 1);
	SSD1306_UpdateScreen();
}
void Prescription_manage(){
	SSD1306_Fill(0);
	PutsXY(0, 0, "#Prescription manage         ", 0);

	SSD1306_UpdateScreen();
}
void Log(){
	SSD1306_Fill(0);
	PutsXY(0, 0, "#Log                    ", 0);

	SSD1306_UpdateScreen();
}
void Emergency_call(){

}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_I2C1_Init();
  MX_ADC1_Init();
  /* USER CODE BEGIN 2 */
  SSD1306_Init();

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
	    sys == init ? Init()
	  : sys == time_set ? Time_set()
	  : sys == main_menu ? Main_menu()
	  : sys == hospit ? Hospit()
	  : sys == discharge ? Discharge()
	  : sys == mm ? Medicine_manage()
	  : sys == pm ? Prescription_manage()
	  : sys == log? Log()
	  : Emergency_call();
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 64;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV2;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief ADC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC1_Init(void)
{

  /* USER CODE BEGIN ADC1_Init 0 */

  /* USER CODE END ADC1_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC1_Init 1 */

  /* USER CODE END ADC1_Init 1 */

  /** Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion)
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV2;
  hadc1.Init.Resolution = ADC_RESOLUTION_12B;
  hadc1.Init.ScanConvMode = ENABLE;
  hadc1.Init.ContinuousConvMode = DISABLE;
  hadc1.Init.DiscontinuousConvMode = ENABLE;
  hadc1.Init.NbrOfDiscConversion = 1;
  hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion = 2;
  hadc1.Init.DMAContinuousRequests = DISABLE;
  hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
  */
  sConfig.Channel = ADC_CHANNEL_0;
  sConfig.Rank = 1;
  sConfig.SamplingTime = ADC_SAMPLETIME_56CYCLES;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
  */
  sConfig.Channel = ADC_CHANNEL_1;
  sConfig.Rank = 2;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */

}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 400000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_16_9;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA2_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA2_Stream0_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA2_Stream0_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA2_Stream0_IRQn);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
/* USER CODE BEGIN MX_GPIO_Init_1 */
/* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(BUZ_GPIO_Port, BUZ_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, LED1_Pin|LED2_Pin|LED3_Pin|LED4_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : JOY_SW_Pin */
  GPIO_InitStruct.Pin = JOY_SW_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(JOY_SW_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : BUZ_Pin */
  GPIO_InitStruct.Pin = BUZ_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(BUZ_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : SW1_Pin SW2_Pin SW3_Pin SW4_Pin */
  GPIO_InitStruct.Pin = SW1_Pin|SW2_Pin|SW3_Pin|SW4_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : LED1_Pin LED2_Pin LED3_Pin LED4_Pin */
  GPIO_InitStruct.Pin = LED1_Pin|LED2_Pin|LED3_Pin|LED4_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
