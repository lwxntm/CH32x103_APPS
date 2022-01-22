/********************************** (C) COPYRIGHT *******************************
 * File Name          : main.c
 * Author             : WCH
 * Version            : V1.0.0
 * Date               : 2019/10/15
 * Description        : Main program body.
 *******************************************************************************/

/*
 *@Note
 AHT21_读写数据
 */

#include "debug.h"

/*******************************************************************************
 * Function Name  : IIC_Init
 * Description    : Initializes the IIC peripheral.
 * Input          : None
 * Return         : None
 *******************************************************************************/
void IIC_Init(u32 bound, u16 address) {
    GPIO_InitTypeDef GPIO_InitStructure;
    I2C_InitTypeDef I2C_InitTSturcture;

    RCC_APB2PeriphClockCmd( RCC_APB2Periph_GPIOB, ENABLE);
    RCC_APB1PeriphClockCmd( RCC_APB1Periph_I2C1, ENABLE);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_OD;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init( GPIOB, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_OD;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init( GPIOB, &GPIO_InitStructure);

    I2C_InitTSturcture.I2C_ClockSpeed = bound;
    I2C_InitTSturcture.I2C_Mode = I2C_Mode_I2C;
    I2C_InitTSturcture.I2C_DutyCycle = I2C_DutyCycle_16_9;
    I2C_InitTSturcture.I2C_OwnAddress1 = address;
    I2C_InitTSturcture.I2C_Ack = I2C_Ack_Enable;
    I2C_InitTSturcture.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
    I2C_Init( I2C1, &I2C_InitTSturcture);

    I2C_Cmd( I2C1, ENABLE);
    I2C_AcknowledgeConfig( I2C1, ENABLE);

}

/*******************************************************************************
 * Function Name  : I2C_WriteDatas
 * Description    : 向I2C设备写入数据
 * Input          : SlaveAddress_8bit: 从机地址，格式是8位.
 *                  datas: 要写入的数据数组.
 *                  length: 数据长度
 * Return         : None
 *******************************************************************************/
void I2C_WriteDatas(uint8_t SlaveAddress_8bit, uint8_t datas[], uint32_t length) {

    u8 i = 0;
    while( I2C_GetFlagStatus( I2C1, I2C_FLAG_BUSY ) != RESET );

    I2C_GenerateSTART( I2C1, ENABLE);

    while( !I2C_CheckEvent( I2C1, I2C_EVENT_MASTER_MODE_SELECT ) );
    I2C_Send7bitAddress( I2C1, SlaveAddress_8bit,
    I2C_Direction_Transmitter);

    while( !I2C_CheckEvent( I2C1, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED ) );

    while(length>i) {
        while( I2C_GetFlagStatus( I2C1, I2C_FLAG_TXE ) == RESET );

        I2C_SendData( I2C1, datas[i++]);
    }
    while( !I2C_CheckEvent( I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED ) );
    I2C_GenerateSTOP( I2C1, ENABLE);

}
/*******************************************************************************
 * Function Name  : I2C_ReadDatas
 * Description    : 向I2C设备写入数据
 * Input          : SlaveAddress_8bit: 从机地址，格式是8位.
 *                  datas: 读取出的数据存放的数组.
 *                  length: 数据长度
 * Return         : None
 *******************************************************************************/
void I2C_ReadDatas(uint8_t SlaveAddress_8bit, uint8_t datas[], uint32_t length) {
    u8 i = 0;

    while( I2C_GetFlagStatus( I2C1, I2C_FLAG_BUSY ) != RESET );
    I2C_GenerateSTART( I2C1, ENABLE);

    while( !I2C_CheckEvent( I2C1, I2C_EVENT_MASTER_MODE_SELECT ) );
    I2C_Send7bitAddress( I2C1, SlaveAddress_8bit, I2C_Direction_Receiver);

    while( !I2C_CheckEvent( I2C1, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED ) );
    while( I2C_GetFlagStatus( I2C1, I2C_FLAG_RXNE ) == RESET )
    I2C_AcknowledgeConfig( I2C1, ENABLE );
    while(length>i) {
        while(I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_RECEIVED) != SUCCESS);
        datas[i++] = I2C_ReceiveData( I2C1);
    }

    I2C_GenerateSTOP( I2C1, ENABLE);

}

void AHT21_Adjust() {
    printf("正在重新校准传感器内部数据！！\r\n");
    uint8_t datas[] = { 0xbe, 0x08, 0x00 };
    I2C_WriteDatas(0x70, datas, 3);
}

void AHT21_SendMeaSure() {
    uint8_t datas[] = { 0xac, 0x33, 0x00 };
    I2C_WriteDatas(0x70, datas, 3);

}

void AHT21_ReadDatas2Buf(uint8_t bufs[]) {
    I2C_ReadDatas(0x71, bufs, 6);
}

void AHT21_PrintInfo() {
    uint8_t bufs[6] = { 0 };
    AHT21_SendMeaSure();
    Delay_Ms(200);
    AHT21_ReadDatas2Buf(bufs);
    if ((bufs[0] & (1 << 3)) == 0) {
        printf("传感器未校准，请先校准");
        return;
    }
    uint32_t humidity_raw = (uint32_t) bufs[1] << 12 | (uint32_t) bufs[2] << 4
            | (uint32_t) bufs[3] >> 4;
    uint32_t humidity = humidity_raw * 100 / 1048576;
    printf("湿度： %d \r\n", humidity);
    uint32_t temp_raw = (bufs[3] & 0x0f) << 16 | bufs[4] << 8 | bufs[5];
    float temp = (float) temp_raw * 200.0 / 1048576.0 - 50;
    printf("温度： %f \r\n\r\n", temp);
}

/*******************************************************************************
 * Function Name  : main
 * Description    : Main program.
 * Input          : None
 * Return         : None
 *******************************************************************************/
int main(void) {

    Delay_Init();
    USART_Printf_Init(115200);
    printf("SystemClk:%d\r\n", SystemCoreClock);

    printf("IIC 读取AHT21\r\n");
    IIC_Init(400000, 0x00);
    AHT21_Adjust();
    Delay_Ms(2000);

    while(1) {
        AHT21_PrintInfo();
        Delay_Ms(2000);
    }
}

