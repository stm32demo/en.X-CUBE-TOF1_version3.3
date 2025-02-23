/**
  ******************************************************************************
  * @file          : app_tof.c
  * @author        : IMG SW Application Team
  * @brief         : This file provides code for the configuration
  *                  of the STMicroelectronics.X-CUBE-TOF1.3.3.0 instances.
  ******************************************************************************
  *
  * @attention
  *
  * Copyright (c) 2022 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "app_tof.h"
#include "main.h"
#include <stdio.h>

#include "53l8a1_ranging_sensor.h"
#include "app_tof_pin_conf.h"
#include "stm32f4xx_nucleo.h"

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/
#define TIMING_BUDGET (30U) /* 5 ms < TimingBudget < 100 ms */
#define RANGING_FREQUENCY (30U) /* Ranging frequency Hz (shall be consistent with TimingBudget value) */
#define POLLING_PERIOD (1000U/RANGING_FREQUENCY) /* refresh rate for polling mode (milliseconds) */

/* Private variables ---------------------------------------------------------*/
static RANGING_SENSOR_Capabilities_t Cap;
static RANGING_SENSOR_ProfileConfig_t Profile;
static RANGING_SENSOR_Result_t Result;
static int32_t status = 0;
static volatile uint8_t PushButtonDetected = 0;
volatile uint8_t ToF_EventDetected = 0;

/* Private function prototypes -----------------------------------------------*/
static void MX_53L8A1_SimpleRanging_Init(void);
static void MX_53L8A1_SimpleRanging_Process(void);
static void print_result(RANGING_SENSOR_Result_t *Result);
static void print_result_myself(RANGING_SENSOR_Result_t *Result);
static void print_result_myself_distance(RANGING_SENSOR_Result_t *Result);
static void toggle_resolution(void);
static void toggle_signal_and_ambient(void);
static void clear_screen(void);
static void display_commands_banner(void);
static void handle_cmd(uint8_t cmd);
static uint8_t get_key(void);
static uint32_t com_has_data(void);

void MX_TOF_Init(void)
{
  /* USER CODE BEGIN SV */

  /* USER CODE END SV */

  /* USER CODE BEGIN TOF_Init_PreTreatment */

  /* USER CODE END TOF_Init_PreTreatment */

  /* Initialize the peripherals and the TOF components */

  MX_53L8A1_SimpleRanging_Init();

  /* USER CODE BEGIN TOF_Init_PostTreatment */

  /* USER CODE END TOF_Init_PostTreatment */
}

/*
 * LM background task
 */
void MX_TOF_Process(void)
{
  /* USER CODE BEGIN TOF_Process_PreTreatment */

  /* USER CODE END TOF_Process_PreTreatment */

  MX_53L8A1_SimpleRanging_Process();

  /* USER CODE BEGIN TOF_Process_PostTreatment */

  /* USER CODE END TOF_Process_PostTreatment */
}

static void MX_53L8A1_SimpleRanging_Init(void)
{
  /* Initialize Virtual COM Port */
  BSP_COM_Init(COM1);

  /* Initialize button */
  BSP_PB_Init(BUTTON_KEY, BUTTON_MODE_EXTI);

  status = VL53L8A1_RANGING_SENSOR_Init(VL53L8A1_DEV_CENTER);

  if (status != BSP_ERROR_NONE)
  {
    printf("VL53L8A1_RANGING_SENSOR_Init failed\n");
    printf("Check you're using ONLY the center device soldered on the shield, NO satellite shall be connected !\n");
    while(1);
  }
}

static void MX_53L8A1_SimpleRanging_Process(void)
{
  uint32_t Id;

  VL53L8A1_RANGING_SENSOR_ReadID(VL53L8A1_DEV_CENTER, &Id);
  VL53L8A1_RANGING_SENSOR_GetCapabilities(VL53L8A1_DEV_CENTER, &Cap);

  Profile.RangingProfile = RS_PROFILE_8x8_CONTINUOUS;
  Profile.TimingBudget = TIMING_BUDGET; /* 5 ms < TimingBudget < 100 ms */
  Profile.Frequency = RANGING_FREQUENCY; /* Ranging frequency Hz (shall be consistent with TimingBudget value) */
  Profile.EnableAmbient = 1; /* Enable: 1, Disable: 0 */
  Profile.EnableSignal = 1; /* Enable: 1, Disable: 0 */

  /* set the profile if different from default one */
  VL53L8A1_RANGING_SENSOR_ConfigProfile(VL53L8A1_DEV_CENTER, &Profile);

  status = VL53L8A1_RANGING_SENSOR_Start(VL53L8A1_DEV_CENTER, RS_MODE_ASYNC_CONTINUOUS);

  if (status != BSP_ERROR_NONE)
  {
    printf("VL53L8A1_RANGING_SENSOR_Start failed\n");
    while(1);
  }

  uint32_t last_time_ = 0;
  uint32_t now_time_ = 0;

  while (1)
  {
    /* polling mode */  
    status = VL53L8A1_RANGING_SENSOR_GetDistance(VL53L8A1_DEV_CENTER, &Result);
    if (status == BSP_ERROR_NONE)
    {
      now_time_ = HAL_GetTick();
      //printf("the target fre is %d ,the infact fre is %d \r\n",RANGING_FREQUENCY,1000/(now_time_ - last_time_));
      printf("%d %d \r\n",RANGING_FREQUENCY,1000/(now_time_ - last_time_));
      last_time_ = now_time_;

      //print_result(&Result);
      //print_result_myself(&Result);
      //print_result_myself_distance(&Result);
    }
    else
    {
      //printf("read data error\r\n");
    }

    // if (com_has_data())
    // {
    //   handle_cmd(get_key());
    // }

    HAL_Delay(5);
  }
}

static void print_result_myself(RANGING_SENSOR_Result_t *Result) {
  uint8_t zones_per_line;
  zones_per_line =
      ((Profile.RangingProfile == RS_PROFILE_8x8_AUTONOMOUS) || (Profile.RangingProfile == RS_PROFILE_8x8_CONTINUOUS))
          ? 8
          : 4;

  for (uint8_t i = 0; i < zones_per_line; i++) {
    for (uint8_t j = 0; j < zones_per_line; j++) {
      printf("%5d(%2d)(%5d/%5d) ",
      (long)Result->ZoneResult[i * zones_per_line + j].Distance[0],
      (long)Result->ZoneResult[i * zones_per_line + j].Status[0],
      (long)Result->ZoneResult[i * zones_per_line + j].Ambient[0],
      (long)Result->ZoneResult[i * zones_per_line + j].Signal[0]         
      );
    }
    printf("\r\n");
  }
  printf("\r\n");
}

  uint8_t left_arr[8][8];
  uint8_t right_arr[8][8];
  uint8_t up_arr[8][8];
//  static bool arr_tof_init = false;

  void output_arr_init(void) {

      uint8_t left_row_up = 1;
      uint8_t left_row_button = 6;
      uint8_t left_column = 4;

      uint8_t right_row_up = 1;
      uint8_t right_row_button = 6;
      uint8_t right_column = 3;

      uint8_t up_row = 5;
      uint8_t up_column_left = 1;
      uint8_t up_column_right = 6;

      for (uint8_t i = 0; i < 8; i++) {
        for (uint8_t j = 0; j < 8; j++) {
          if (i > left_row_up && i < left_row_button && j < left_column) {
            left_arr[i][j] = 1;
          } else {
            left_arr[i][j] = 0;
          }

          if (i > right_row_up && i < right_row_button && j > right_column) {
            right_arr[i][j] = 1;
          } else {
            right_arr[i][j] = 0;
          }

          if (i < up_row && j > up_column_left && j < up_column_right) {
            up_arr[i][j] = 1;
          } else {
            up_arr[i][j] = 0;
          }
        }
      }
  }

    void matrix_rotate(RANGING_SENSOR_Result_t *Result, uint32_t arr[][8],uint8_t zones_per_line) {
    uint32_t origin_arr[8][8];
    for (uint8_t i = 0; i < zones_per_line; i++) {
      for (uint8_t j = 0; j < zones_per_line; j++) {
        if (Result->ZoneResult[i * zones_per_line + j].Status[0] == 0) {
          origin_arr[i][j] = Result->ZoneResult[i * zones_per_line + j].Distance[0];
        } else {
          origin_arr[i][j] = 0;
        }
      }
    }
    for (uint8_t i = 0; i < zones_per_line; i++) {
      for (uint8_t j = 0; j < zones_per_line; j++) {
        arr[i][j] = origin_arr[zones_per_line - 1 - i][zones_per_line - 1 - j];
      }
    }
  }


static void print_result_myself_distance(RANGING_SENSOR_Result_t *Result) {
  uint8_t zones_per_line;
  zones_per_line =
      ((Profile.RangingProfile == RS_PROFILE_8x8_AUTONOMOUS) || (Profile.RangingProfile == RS_PROFILE_8x8_CONTINUOUS))
          ? 8
          : 4;

  uint32_t origin_arr[8][8];
  uint32_t new_arr[8][8];

  matrix_rotate(Result,new_arr,zones_per_line);
  output_arr_init();

  // for (uint8_t i = 0; i < zones_per_line; i++) {
  //   for (uint8_t j = 0; j < zones_per_line; j++) {
  //     origin_arr[i][j] = (long)Result->ZoneResult[i * zones_per_line + j].Distance[0];
  //   }
  // }
  // for (uint8_t i = 0; i < zones_per_line; i++) {
  //   for (uint8_t j = 0; j < zones_per_line; j++) {
  //     new_arr[i][j] = origin_arr[zones_per_line - 1 - i][zones_per_line - 1 - j];
  //   }
  // }

    for (uint8_t i = 0; i < zones_per_line; i++) {
    for (uint8_t j = 0; j < zones_per_line; j++) {
      new_arr[i][j] =  new_arr[i][j]*up_arr[i][j];
    } 
  }

  for (uint8_t i = 0; i < zones_per_line; i++) {
    for (uint8_t j = 0; j < zones_per_line; j++) {
      printf("%d ", new_arr[i][j]);
    }
    printf("\r\n");
  }
  printf("\r\n");
}

static void print_result(RANGING_SENSOR_Result_t *Result)
{
  int8_t i, j, k, l;
  uint8_t zones_per_line;

  zones_per_line = ((Profile.RangingProfile == RS_PROFILE_8x8_AUTONOMOUS) ||
         (Profile.RangingProfile == RS_PROFILE_8x8_CONTINUOUS)) ? 8 : 4;

  display_commands_banner();

  printf("Cell Format :\n\n");
  for (l = 0; l < RANGING_SENSOR_NB_TARGET_PER_ZONE; l++)
  {
    printf(" \033[38;5;10m%20s\033[0m : %20s\n", "Distance [mm]", "Status");
    if ((Profile.EnableAmbient != 0) || (Profile.EnableSignal != 0))
    {
      printf(" %20s : %20s\n", "Signal [kcps/spad]", "Ambient [kcps/spad]");
    }
  }

  printf("\n\n");

  for (j = 0; j < Result->NumberOfZones; j += zones_per_line)
  {
    for (i = 0; i < zones_per_line; i++) /* number of zones per line */
      printf(" -----------------");
    printf("\n");

    for (i = 0; i < zones_per_line; i++)
      printf("|                 ");
    printf("|\n");

    for (l = 0; l < RANGING_SENSOR_NB_TARGET_PER_ZONE; l++)
    {
      /* Print distance and status */
      for (k = (zones_per_line - 1); k >= 0; k--)
      {
        if (Result->ZoneResult[j+k].NumberOfTargets > 0)
          printf("| \033[38;5;10m%5ld\033[0m  :  %5ld ",
              (long)Result->ZoneResult[j+k].Distance[l],
              (long)Result->ZoneResult[j+k].Status[l]);
        else
          printf("| %5s  :  %5s ", "X", "X");
      }
      printf("|\n");

      if ((Profile.EnableAmbient != 0) || (Profile.EnableSignal != 0))
      {
        /* Print Signal and Ambient */
        for (k = (zones_per_line - 1); k >= 0; k--)
        {
          if (Result->ZoneResult[j+k].NumberOfTargets > 0)
          {
            if (Profile.EnableSignal != 0)
              printf("| %5ld  :  ", (long)Result->ZoneResult[j+k].Signal[l]);
            else
              printf("| %5s  :  ", "X");

            if (Profile.EnableAmbient != 0)
              printf("%5ld ", (long)Result->ZoneResult[j+k].Ambient[l]);
            else
              printf("%5s ", "X");
          }
          else
            printf("| %5s  :  %5s ", "X", "X");
        }
        printf("|\n");
      }
    }
  }

  for (i = 0; i < zones_per_line; i++)
    printf(" -----------------");
  printf("\n");
}

static void toggle_resolution(void)
{
  VL53L8A1_RANGING_SENSOR_Stop(VL53L8A1_DEV_CENTER);

  switch (Profile.RangingProfile)
  {
    case RS_PROFILE_4x4_AUTONOMOUS:
      Profile.RangingProfile = RS_PROFILE_8x8_AUTONOMOUS;
      break;

    case RS_PROFILE_4x4_CONTINUOUS:
      Profile.RangingProfile = RS_PROFILE_8x8_CONTINUOUS;
      break;

    case RS_PROFILE_8x8_AUTONOMOUS:
      Profile.RangingProfile = RS_PROFILE_4x4_AUTONOMOUS;
      break;

    case RS_PROFILE_8x8_CONTINUOUS:
      Profile.RangingProfile = RS_PROFILE_4x4_CONTINUOUS;
      break;

    default:
      break;
  }

  VL53L8A1_RANGING_SENSOR_ConfigProfile(VL53L8A1_DEV_CENTER, &Profile);
  VL53L8A1_RANGING_SENSOR_Start(VL53L8A1_DEV_CENTER, RS_MODE_BLOCKING_CONTINUOUS);
}

static void toggle_signal_and_ambient(void)
{
  VL53L8A1_RANGING_SENSOR_Stop(VL53L8A1_DEV_CENTER);

  Profile.EnableAmbient = (Profile.EnableAmbient) ? 0U : 1U;
  Profile.EnableSignal = (Profile.EnableSignal) ? 0U : 1U;

  VL53L8A1_RANGING_SENSOR_ConfigProfile(VL53L8A1_DEV_CENTER, &Profile);
  VL53L8A1_RANGING_SENSOR_Start(VL53L8A1_DEV_CENTER, RS_MODE_BLOCKING_CONTINUOUS);
}

static void clear_screen(void)
{
  printf("%c[2J", 27); /* 27 is ESC command */
}

static void display_commands_banner(void)
{
  /* clear screen */
  printf("%c[2H", 27);

  printf("53L8A1 Simple Ranging demo application\n");
  printf("--------------------------------------\n\n");

  printf("Use the following keys to control application\n");
  printf(" 'r' : change resolution\n");
  printf(" 's' : enable signal and ambient\n");
  printf(" 'c' : clear screen\n");
  printf("\n");
}

static void handle_cmd(uint8_t cmd)
{
  switch (cmd)
  {
    case 'r':
      toggle_resolution();
      clear_screen();
      break;

    case 's':
      toggle_signal_and_ambient();
      clear_screen();
      break;

    case 'c':
      clear_screen();
      break;

    default:
      break;
  }
}

static uint8_t get_key(void)
{
  uint8_t cmd = 0;

  HAL_UART_Receive(&hcom_uart[COM1], &cmd, 1, HAL_MAX_DELAY);

  return cmd;
}

static uint32_t com_has_data(void)
{
  return __HAL_UART_GET_FLAG(&hcom_uart[COM1], UART_FLAG_RXNE);;
}

void BSP_PB_Callback(Button_TypeDef Button)
{
  PushButtonDetected = 1;
}

#ifdef __cplusplus
}
#endif
