/*!
 * \file      sx1261dvk1bas-board.c
 *
 * \brief     Target board SX1261DVK1BAS shield driver implementation
 *
 * \copyright Revised BSD License, see section \ref LICENSE.
 *
 * \code
 *                ______                              _
 *               / _____)             _              | |
 *              ( (____  _____ ____ _| |_ _____  ____| |__
 *               \____ \| ___ |    (_   _) ___ |/ ___)  _ \
 *               _____) ) ____| | | || |_| ____( (___| | | |
 *              (______/|_____)_|_|_| \__)_____)\____)_| |_|
 *              (C)2013-2017 Semtech
 *
 * \endcode
 *
 * \author    Miguel Luis ( Semtech )
 *
 * \author    Gregory Cristian ( Semtech )
 */
#include <project.h>
//#include "asr_project.h"
#include <stdlib.h>
#include "utilities.h"
#include "board-config.h"
#include "board.h"
#include "delay.h"
#include "timer.h"
#include "debug.h"


#define         ID1                                 ( 0x1FF80050 )
#define         ID2                                 ( 0x1FF80054 )
#define         ID3                                 ( 0x1FF80064 )

/*!
 * Antenna switch GPIO pins objects
 */
Gpio_t AntPow;
Gpio_t DeviceSel;
LOG_LEVEL g_log_level = LL_DEBUG;

#ifdef CONFIG_LORA_USE_TCXO
bool UseTCXO = true;
#else
bool UseTCXO = false;
#endif
uint8_t gPaOptSetting = 0;
static uint32_t gBaudRate = STDIO_UART_BAUDRATE;
char gChipId[17];

void BoardDisableIrq( void )
{
    CyGlobalIntDisable;    
}

void BoardEnableIrq( void )
{
    CyGlobalIntEnable;
}

void DelayMsMcu( uint32_t ms )
{
    CyDelay(ms);
}

static char *olds = NULL;
extern void *rawmemchr (__const void *__s, int __c);
char * strtok_l (char *s, const char *delim)
{
    char *token;

    if (s == NULL) s = olds;

    s += strspn (s, delim);
    if (*s == '\0') {
        olds = s;
        return NULL;
    }

    token = s;
    s = strpbrk (token, delim);  
    if (s == NULL)
        olds = rawmemchr (token, '\0');
    else {      
        *s = '\0';        
        olds = s + 1;
    }
    return token;
}


static const double huge = 1.0e300;
#define __HI(x) *(1+(int*)&x)
#define __LO(x) *(int*)&x

static const double TWO52[2]={
  4.50359962737049600000e+15, /* 0x43300000, 0x00000000 */
 -4.50359962737049600000e+15, /* 0xC3300000, 0x00000000 */
};

extern uint32_t systime;


void boardInitMcu( void )
{
    SpiInit();
    Asr_Timer_Init();
    RtcInit();
    systime = millis();
#if defined(CubeCell_Board)||defined(CubeCell_Capsule)||defined(CubeCell_BoardPlus)||defined(CubeCell_GPS)||defined(CubeCell_HalfAA)
    pinMode(Vext,OUTPUT);
    digitalWrite(Vext,HIGH);

    /*
     * Board, BoardPlus, Capsule, GPS and HalfAA variants
     * have external 10K VDD pullup resistor
     * connected to GPIO7 (USER_KEY / VBAT_ADC_CTL) pin
     */
    pinMode(VBAT_ADC_CTL, INPUT);
#endif
}

void DBG_LogLevelSet(int level)
{
    g_log_level = level;
}

int DBG_LogLevelGet()
{
    return g_log_level;
}

char *HW_Get_MFT_ID(void)
{
    return CONFIG_MANUFACTURER;
}

char *HW_Get_MFT_Model(void)
{
    return CONFIG_DEVICE_MODEL;   
}

char *HW_Get_MFT_Rev(void)
{
    return CONFIG_VERSION;
}

char *HW_Get_MFT_SN(void)
{
    uint32_t id[2];
    CyGetUniqueId(id);
    sprintf(gChipId, "%08X%08X", (unsigned int)id[0], (unsigned int)id[1]);
    return gChipId;
}

bool HW_Set_MFT_Baud(uint32_t baud)
{   
    uint32_t div = (float)CYDEV_BCLK__HFCLK__HZ/baud/UART_1_UART_OVS_FACTOR + 0.5 - 1;
    UART_1_SCBCLK_DIV_REG = div<<8;
    UART_1_SCBCLK_CMD_REG = 0x8000FF41u;
    
    gBaudRate = baud;   
    return true;
}

uint32_t HW_Get_MFT_Baud(void)
{
    return gBaudRate;
}

void HW_Reset(int mode)
{
    if (mode == 0) {
	    CySoftwareReset();
    } else if (mode == 1) {
        Bootloadable_1_Load();
    } 
}

uint32_t BoardGetRandomSeed( void )
{
    return ( ( *( uint32_t* )ID1 ) ^ ( *( uint32_t* )ID2 ) ^ ( *( uint32_t* )ID3 ) );
}



