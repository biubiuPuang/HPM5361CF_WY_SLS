/*
 * Copyright (c) 2023 HPMicro
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

/*
 * Note:
 *  PY and PZ IOs: if any SOC pin function needs to be routed to these IOs,
 *  besides of IOC, PIOC/BIOC needs to be configured SOC_GPIO_X_xx, so that
 *  expected SoC function can be enabled on these IOs.
 *
 */
#include "pinmux.h"
#include "board.h"
#include "hpm_iomux.h"
#include "hpm_gpio_drv.h"
#include "hpm_gpiom_drv.h"


void init_xtal_pins(void)
{
    /* Package QFN32 should be set PA30 and PA31 pins as analog type to enable xtal. */
    HPM_IOC->PAD[IOC_PAD_PA30].FUNC_CTL = IOC_PAD_FUNC_CTL_ANALOG_MASK;
    HPM_IOC->PAD[IOC_PAD_PA31].FUNC_CTL = IOC_PAD_FUNC_CTL_ANALOG_MASK;
}

/**
 * @brief  JTAG 
 * 
 *  JTAG  TCKTDITDOTMS  TRST 
 * 
 */
void init_jtag_pins(void)
{
    //  PA06  JTAG TCK 
    HPM_IOC->PAD[IOC_PAD_PA06].FUNC_CTL = IOC_PA06_FUNC_CTL_JTAG_TCK;
    //  PA05  JTAG TDI 
    HPM_IOC->PAD[IOC_PAD_PA05].FUNC_CTL = IOC_PA05_FUNC_CTL_JTAG_TDI;
    //  PA04  JTAG TDO 
    HPM_IOC->PAD[IOC_PAD_PA04].FUNC_CTL = IOC_PA04_FUNC_CTL_JTAG_TDO;
    //  PA07  JTAG TMS 
    HPM_IOC->PAD[IOC_PAD_PA07].FUNC_CTL = IOC_PA07_FUNC_CTL_JTAG_TMS;
    //  PA08  JTAG TRST 
    HPM_IOC->PAD[IOC_PAD_PA08].FUNC_CTL = IOC_PA08_FUNC_CTL_JTAG_TRST;
}

void init_py_pins_as_pgpio(void)
{
    /* Set PY00-PY05 default function to PGPIO */
    HPM_PIOC->PAD[IOC_PAD_PY00].FUNC_CTL = IOC_PY00_FUNC_CTL_PGPIO_Y_00;
    HPM_PIOC->PAD[IOC_PAD_PY01].FUNC_CTL = IOC_PY01_FUNC_CTL_PGPIO_Y_01;
    HPM_PIOC->PAD[IOC_PAD_PY02].FUNC_CTL = IOC_PY02_FUNC_CTL_PGPIO_Y_02;
    HPM_PIOC->PAD[IOC_PAD_PY03].FUNC_CTL = IOC_PY03_FUNC_CTL_PGPIO_Y_03;
    HPM_PIOC->PAD[IOC_PAD_PY04].FUNC_CTL = IOC_PY04_FUNC_CTL_PGPIO_Y_04;
    HPM_PIOC->PAD[IOC_PAD_PY05].FUNC_CTL = IOC_PY05_FUNC_CTL_PGPIO_Y_05;
}

void init_uart_pins(UART_Type *ptr)
{
    if (ptr == HPM_UART0) {// 第一路485 监听
        HPM_IOC->PAD[IOC_PAD_PA00].FUNC_CTL = IOC_PA00_FUNC_CTL_UART0_TXD;
        HPM_IOC->PAD[IOC_PAD_PA01].FUNC_CTL = IOC_PA01_FUNC_CTL_UART0_RXD;
    }
    if (ptr == HPM_UART2) {// 第二路485 控制
        HPM_IOC->PAD[IOC_PAD_PB08].FUNC_CTL = IOC_PB08_FUNC_CTL_UART2_TXD;
        HPM_IOC->PAD[IOC_PAD_PB09].FUNC_CTL = IOC_PB09_FUNC_CTL_UART2_RXD;
    }
    if (ptr == HPM_UART4) { // 调试板子485接口
        HPM_IOC->PAD[IOC_PAD_PA16].FUNC_CTL = IOC_PA16_FUNC_CTL_UART4_TXD;
        HPM_IOC->PAD[IOC_PAD_PA17].FUNC_CTL = IOC_PA17_FUNC_CTL_UART4_RXD;
    }    
    if (ptr == HPM_UART6) { // 调试板子485接口
        HPM_IOC->PAD[IOC_PAD_PA24].FUNC_CTL = IOC_PA24_FUNC_CTL_UART6_TXD;
        HPM_IOC->PAD[IOC_PAD_PA25].FUNC_CTL = IOC_PA25_FUNC_CTL_UART6_RXD;
    }
}

void init_led_pins(void)
{
    HPM_IOC->PAD[IOC_PAD_PA15].FUNC_CTL = IOC_PA15_FUNC_CTL_GPIO_A_15;

}

void init_gpio_pins(void)
{
    /* PWM_IO引脚配置 */
    HPM_IOC->PAD[IOC_PAD_PA11].FUNC_CTL = IOC_PA11_FUNC_CTL_GPIO_A_11;
    gpio_set_pin_output(HPM_GPIO0, GPIO_OE_GPIOA, 11);
    gpio_write_pin(HPM_GPIO0, GPIO_DO_GPIOA, 11, 1);
    gpio_write_pin(HPM_GPIO0, GPIO_DO_GPIOA, 11, 0);


    // HPM_IOC->PAD[IOC_PAD_PA28].FUNC_CTL = IOC_PA28_FUNC_CTL_GPIO_A_28;

    // //gpiom_set_pin_controller(HPM_GPIOM, GPIOM_ASSIGN_GPIOA, 28, gpiom_soc_gpio0);
    // gpio_set_pin_output(HPM_GPIO0, GPIO_OE_GPIOA, 28);
    // gpio_write_pin(HPM_GPIO0, GPIO_DO_GPIOA, 28, 0);

    // HPM_IOC->PAD[IOC_PAD_PA29].FUNC_CTL = IOC_PA29_FUNC_CTL_GPIO_A_29;

    // //gpiom_set_pin_controller(HPM_GPIOM, GPIOM_ASSIGN_GPIOA, 29, gpiom_soc_gpio0);
    // gpio_set_pin_output(HPM_GPIO0, GPIO_OE_GPIOA, 29);
    // gpio_write_pin(HPM_GPIO0, GPIO_DO_GPIOA, 29, 0);

    // /* ADC引脚配置为模拟输入 */
    // HPM_IOC->PAD[IOC_PAD_PB08].FUNC_CTL = IOC_PAD_FUNC_CTL_ANALOG_MASK;
    // HPM_IOC->PAD[IOC_PAD_PB09].FUNC_CTL = IOC_PAD_FUNC_CTL_ANALOG_MASK;
    // HPM_IOC->PAD[IOC_PAD_PB13].FUNC_CTL = IOC_PAD_FUNC_CTL_ANALOG_MASK;
    // HPM_IOC->PAD[IOC_PAD_PB14].FUNC_CTL = IOC_PAD_FUNC_CTL_ANALOG_MASK;

    // HPM_IOC->PAD[IOC_PAD_PB08].FUNC_CTL = IOC_PAD_FUNC_CTL_ANALOG_MASK;

    // HPM_IOC->PAD[IOC_PAD_PB09].FUNC_CTL = IOC_PAD_FUNC_CTL_ANALOG_MASK;


    // HPM_IOC->PAD[IOC_PAD_PB13].FUNC_CTL = IOC_PAD_FUNC_CTL_ANALOG_MASK;

    // HPM_IOC->PAD[IOC_PAD_PB14].FUNC_CTL = IOC_PAD_FUNC_CTL_ANALOG_MASK;


}

void init_spi_pins(SPI_Type *ptr)
{
    if (ptr == HPM_SPI0) {
        HPM_IOC->PAD[IOC_PAD_PA25].FUNC_CTL = IOC_PA25_FUNC_CTL_SPI1_CS_1;
        HPM_IOC->PAD[IOC_PAD_PA26].FUNC_CTL = IOC_PA26_FUNC_CTL_SPI1_CS_0;
        HPM_IOC->PAD[IOC_PAD_PA27].FUNC_CTL = IOC_PA27_FUNC_CTL_SPI1_SCLK | IOC_PAD_FUNC_CTL_LOOP_BACK_SET(1);
        HPM_IOC->PAD[IOC_PAD_PA28].FUNC_CTL = IOC_PA28_FUNC_CTL_SPI1_MISO;

        /* set max frequency slew rate(200M) */
        HPM_IOC->PAD[IOC_PAD_PA25].PAD_CTL = IOC_PAD_PAD_CTL_SR_MASK | IOC_PAD_PAD_CTL_SPD_SET(3);
        HPM_IOC->PAD[IOC_PAD_PA26].PAD_CTL = IOC_PAD_PAD_CTL_SR_MASK | IOC_PAD_PAD_CTL_SPD_SET(3);
        HPM_IOC->PAD[IOC_PAD_PA27].PAD_CTL = IOC_PAD_PAD_CTL_SR_MASK | IOC_PAD_PAD_CTL_SPD_SET(3);
        HPM_IOC->PAD[IOC_PAD_PA28].PAD_CTL = IOC_PAD_PAD_CTL_SR_MASK | IOC_PAD_PAD_CTL_SPD_SET(3);
    }
}

void init_spi_pins_with_gpio_as_cs(SPI_Type *ptr)
{
    if (ptr == HPM_SPI1)
    {
        HPM_IOC->PAD[IOC_PAD_PA25].FUNC_CTL = IOC_PA25_FUNC_CTL_GPIO_A_25;
        HPM_IOC->PAD[IOC_PAD_PA26].FUNC_CTL = IOC_PA26_FUNC_CTL_GPIO_A_26;
        HPM_IOC->PAD[IOC_PAD_PA27].FUNC_CTL = IOC_PA27_FUNC_CTL_SPI1_SCLK | IOC_PAD_FUNC_CTL_LOOP_BACK_SET(1);
        HPM_IOC->PAD[IOC_PAD_PA28].FUNC_CTL = IOC_PA28_FUNC_CTL_SPI1_MISO;
        HPM_IOC->PAD[IOC_PAD_PA29].FUNC_CTL = IOC_PA29_FUNC_CTL_SPI1_MOSI;
    }
}





void init_butn_pins(void)
{
    /* configure pad setting: pull enable and pull up, schmitt trigger enable */
    /* enable schmitt trigger to eliminate jitter of pin used as button */

    /* Button */
    uint32_t pad_ctl = IOC_PAD_PAD_CTL_PE_SET(1) | IOC_PAD_PAD_CTL_PS_SET(1) | IOC_PAD_PAD_CTL_HYS_SET(1);
    HPM_IOC->PAD[IOC_PAD_PA09].FUNC_CTL = IOC_PA09_FUNC_CTL_GPIO_A_09;
    HPM_IOC->PAD[IOC_PAD_PA09].PAD_CTL = pad_ctl;
}
void init_key_pins(void)
{
    /* KEY1 */
    uint32_t pad_ctl = IOC_PAD_PAD_CTL_PE_SET(1) | IOC_PAD_PAD_CTL_PS_SET(0) | IOC_PAD_PAD_CTL_HYS_SET(1);
    HPM_IOC->PAD[IOC_PAD_PA09].FUNC_CTL = IOC_PA09_FUNC_CTL_GPIO_A_09;
    HPM_IOC->PAD[IOC_PAD_PA09].PAD_CTL = pad_ctl;
    /* KEY2 */
    HPM_IOC->PAD[IOC_PAD_PA14].FUNC_CTL = IOC_PA14_FUNC_CTL_GPIO_A_14;
    HPM_IOC->PAD[IOC_PAD_PA14].PAD_CTL = pad_ctl;
}

void init_acmp_pins(void)
{
    /* configure to ACMP_COMP_1(ALT16) function */
    HPM_IOC->PAD[IOC_PAD_PB09].FUNC_CTL = IOC_PB09_FUNC_CTL_ACMP_COMP_1;
    /* configure to CMP1_INN4 function */
    HPM_IOC->PAD[IOC_PAD_PB11].FUNC_CTL = IOC_PAD_FUNC_CTL_ANALOG_MASK;
}

void init_pwm_pins(PWM_Type *ptr)
{
    if (ptr == HPM_PWM0)
    {
        HPM_IOC->PAD[IOC_PAD_PA10].FUNC_CTL = IOC_PA10_FUNC_CTL_PWM0_P_6;

    }
}

//void init_adc_pins(void)
//{
//#if GET_ENCODER_INTERFACE(ENCODER_TYPE) == ENCODER_INTERFACE_SPI
//    HPM_IOC->PAD[IOC_PAD_PB08].FUNC_CTL = IOC_PAD_FUNC_CTL_ANALOG_MASK;
//    HPM_IOC->PAD[IOC_PAD_PB09].FUNC_CTL = IOC_PAD_FUNC_CTL_ANALOG_MASK;
//    HPM_IOC->PAD[IOC_PAD_PB10].FUNC_CTL = IOC_PAD_FUNC_CTL_ANALOG_MASK;
//#else
//    HPM_IOC->PAD[IOC_PAD_PB08].FUNC_CTL = IOC_PAD_FUNC_CTL_ANALOG_MASK;
//    HPM_IOC->PAD[IOC_PAD_PB09].FUNC_CTL = IOC_PAD_FUNC_CTL_ANALOG_MASK;
//    HPM_IOC->PAD[IOC_PAD_PB10].FUNC_CTL = IOC_PAD_FUNC_CTL_ANALOG_MASK;
//#if GET_ENCODER_INTERFACE(ENCODER_TYPE) == ENCODER_INTERFACE_LINEAR_HALL
//    HPM_IOC->PAD[IOC_PAD_PB12].FUNC_CTL = IOC_PAD_FUNC_CTL_ANALOG_MASK;
//    HPM_IOC->PAD[IOC_PAD_PB13].FUNC_CTL = IOC_PAD_FUNC_CTL_ANALOG_MASK;
//#elif GET_ENCODER_INTERFACE(ENCODER_TYPE) == ENCODER_INTERFACE_ANALOG
//    HPM_IOC->PAD[IOC_PAD_PB12].FUNC_CTL = IOC_PAD_FUNC_CTL_ANALOG_MASK;
//#endif
//#endif
//}


void init_usb_pins(void)
{
    /* Package QFN48 and LQFP64 should be set PA24 and PA25 pins as analog type to enable USB_P and USB_N. */
    HPM_IOC->PAD[IOC_PAD_PA24].FUNC_CTL = IOC_PAD_FUNC_CTL_ANALOG_MASK;
    HPM_IOC->PAD[IOC_PAD_PA25].FUNC_CTL = IOC_PAD_FUNC_CTL_ANALOG_MASK;
}

void init_can_pins(MCAN_Type *ptr)
{
    if (ptr == HPM_MCAN0) {
        /* PB00: MCAN0_TXD */
        HPM_IOC->PAD[IOC_PAD_PB00].FUNC_CTL = IOC_PB00_FUNC_CTL_MCAN0_TXD;
        /* PB01: MCAN0_RXD */
        HPM_IOC->PAD[IOC_PAD_PA01].FUNC_CTL = IOC_PB01_FUNC_CTL_MCAN0_RXD;
    } else if (ptr == HPM_MCAN3) { 
        /* PB15: MCAN3_TXD */
        HPM_IOC->PAD[IOC_PAD_PA15].FUNC_CTL = IOC_PB15_FUNC_CTL_MCAN3_TXD; 
         /* PB14: MCAN3_RXD */
        HPM_IOC->PAD[IOC_PAD_PB14].FUNC_CTL = IOC_PB14_FUNC_CTL_MCAN3_RXD;      
    }
}



void init_dac_pins(DAC_Type *ptr)
{
    if (ptr == HPM_DAC0)
    {
        HPM_IOC->PAD[IOC_PAD_PB08].FUNC_CTL = IOC_PAD_FUNC_CTL_ANALOG_MASK; /* DAC0.OUT */
    }
    else if (ptr == HPM_DAC1)
    {
        HPM_IOC->PAD[IOC_PAD_PB09].FUNC_CTL = IOC_PAD_FUNC_CTL_ANALOG_MASK; /* DAC1.OUT */
    }
}
void init_adc_pins(uint8_t channel)
{
   
         if (channel == 2)
    {
        HPM_IOC->PAD[IOC_PAD_PB10].FUNC_CTL = IOC_PAD_FUNC_CTL_ANALOG_MASK;//PB10_ADC0.IN2 电机电压
    }
         else if (channel == 3)
    {
        HPM_IOC->PAD[IOC_PAD_PB11].FUNC_CTL = IOC_PAD_FUNC_CTL_ANALOG_MASK;//PB11_ADC0.IN3 电机电流
    }  
    else if (channel == 4)
    {
        HPM_IOC->PAD[IOC_PAD_PB12].FUNC_CTL = IOC_PAD_FUNC_CTL_ANALOG_MASK;//PB12_ADC0.IN4 母线电压
    }
}








void init_qeiv2_uvw_pins(QEIV2_Type *ptr)
{
    if (ptr == HPM_QEI1)
    {
        HPM_IOC->PAD[IOC_PAD_PA10].FUNC_CTL = IOC_PA10_FUNC_CTL_QEI1_A;
        HPM_IOC->PAD[IOC_PAD_PA11].FUNC_CTL = IOC_PA11_FUNC_CTL_QEI1_B;
        HPM_IOC->PAD[IOC_PAD_PA12].FUNC_CTL = IOC_PA12_FUNC_CTL_QEI1_Z;
    }
}

void init_qeiv2_ab_pins(QEIV2_Type *ptr)
{
    if (ptr == HPM_QEI1)
    {
        HPM_IOC->PAD[IOC_PAD_PA10].FUNC_CTL = IOC_PA10_FUNC_CTL_QEI1_A;
        HPM_IOC->PAD[IOC_PAD_PA11].FUNC_CTL = IOC_PA11_FUNC_CTL_QEI1_B;
    }
}

void init_qeiv2_abz_pins(QEIV2_Type *ptr)
{
    if (ptr == HPM_QEI1)
    {
        HPM_IOC->PAD[IOC_PAD_PA10].FUNC_CTL = IOC_PA10_FUNC_CTL_QEI1_A;
        HPM_IOC->PAD[IOC_PAD_PA11].FUNC_CTL = IOC_PA11_FUNC_CTL_QEI1_B;
        HPM_IOC->PAD[IOC_PAD_PA12].FUNC_CTL = IOC_PA12_FUNC_CTL_QEI1_Z;
    }
}

void init_opamp_pins(void)
{
    HPM_IOC->PAD[IOC_PAD_PB00].FUNC_CTL = IOC_PAD_FUNC_CTL_ANALOG_MASK;
    HPM_IOC->PAD[IOC_PAD_PB04].FUNC_CTL = IOC_PAD_FUNC_CTL_ANALOG_MASK;
}

void init_i2c_pins(I2C_Type *ptr)
{
    if (ptr == HPM_I2C0) {
        HPM_IOC->PAD[IOC_PAD_PB02].FUNC_CTL = IOC_PB02_FUNC_CTL_I2C0_SCL | IOC_PAD_FUNC_CTL_LOOP_BACK_MASK;
        HPM_IOC->PAD[IOC_PAD_PB03].FUNC_CTL = IOC_PB03_FUNC_CTL_I2C0_SDA | IOC_PAD_FUNC_CTL_LOOP_BACK_MASK;
        
        /* Open-drain and pull-up */
        HPM_IOC->PAD[IOC_PAD_PB00].PAD_CTL = IOC_PAD_PAD_CTL_OD_SET(1) | IOC_PAD_PAD_CTL_PE_SET(1) | IOC_PAD_PAD_CTL_PS_SET(1);
        HPM_IOC->PAD[IOC_PAD_PB01].PAD_CTL = IOC_PAD_PAD_CTL_OD_SET(1) | IOC_PAD_PAD_CTL_PE_SET(1) | IOC_PAD_PAD_CTL_PS_SET(1);
    } else if (ptr == HPM_I2C1) {
        HPM_IOC->PAD[IOC_PAD_PB06].FUNC_CTL = IOC_PB06_FUNC_CTL_I2C1_SDA | IOC_PAD_FUNC_CTL_LOOP_BACK_MASK;
        HPM_IOC->PAD[IOC_PAD_PB07].FUNC_CTL = IOC_PB07_FUNC_CTL_I2C1_SCL | IOC_PAD_FUNC_CTL_LOOP_BACK_MASK;
        
        /* Open-drain and pull-up */
        HPM_IOC->PAD[IOC_PAD_PB06].PAD_CTL = IOC_PAD_PAD_CTL_OD_SET(1) | IOC_PAD_PAD_CTL_PE_SET(1) | IOC_PAD_PAD_CTL_PS_SET(1);
        HPM_IOC->PAD[IOC_PAD_PB07].PAD_CTL = IOC_PAD_PAD_CTL_OD_SET(1) | IOC_PAD_PAD_CTL_PE_SET(1) | IOC_PAD_PAD_CTL_PS_SET(1);
    }
}