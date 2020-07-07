#include "board.h"
#include "vs1053b.h"
#include "rtl876x_spi.h"
#include "rtl876x_pinmux.h"
#include "rtl876x_rcc.h"
#include "rtl876x_gpio.h"
#include "rtl876x_uart.h"
#include "trace.h"
#include "platform_utils.h"
#include <os_mem.h>

#if VS1053B_EN
/*============================================================================*
 *                         Macros
 *============================================================================*/
#define  delay_ms(n)  platform_delay_ms(n)
#define  delay_us(n)  platform_delay_us(n)

#define VS_DQ                      GPIO_ReadInputDataBit(GPIO_GetPin(VS_DQ_PIN))
#define VS_RST_RESET()             GPIO_WriteBit(GPIO_GetPin(VS_RST_PIN), (BitAction)(0))
#define VS_RST_SET()               GPIO_WriteBit(GPIO_GetPin(VS_RST_PIN), (BitAction)(1))

#define VS_XCS_RESET()             GPIO_WriteBit(GPIO_GetPin(VS_XCS_PIN), (BitAction)(0))
#define VS_XCS_SET()               GPIO_WriteBit(GPIO_GetPin(VS_XCS_PIN), (BitAction)(1))

#define VS_XDCS_RESET()            GPIO_WriteBit(GPIO_GetPin(VS_XDCS_PIN), (BitAction)(0))
#define VS_XDCS_SET()              GPIO_WriteBit(GPIO_GetPin(VS_XDCS_PIN), (BitAction)(1))

/*============================================================================*
 *                         Local Variables
 *============================================================================*/

T_VOICE_DRIVER_GLOBAL_DATA voice_driver_global_data = {0};
//VS10XX defalt parameters
_vs10xx_obj vsset =
{
    220,    //volume:220
    6,      //�������� 60Hz
    15,     //�������� 15dB
    10,     //�������� 10Khz
    15,     //�������� 10.5dB
    0,      //�ռ�Ч��
    1,      //��������Ĭ�ϴ�.
};

//VS1053 WAV recorder have bugs, need load this plugin
const uint16_t wav_plugin[40] = /* Compressed plugin */
{
    0x0007, 0x0001, 0x8010, 0x0006, 0x001c, 0x3e12, 0xb817, 0x3e14, /* 0 */
    0xf812, 0x3e01, 0xb811, 0x0007, 0x9717, 0x0020, 0xffd2, 0x0030, /* 8 */
    0x11d1, 0x3111, 0x8024, 0x3704, 0xc024, 0x3b81, 0x8024, 0x3101, /* 10 */
    0x8024, 0x3b81, 0x8024, 0x3f04, 0xc024, 0x2808, 0x4800, 0x36f1, /* 18 */
    0x9811, 0x0007, 0x0001, 0x8028, 0x0006, 0x0002, 0x2a00, 0x040e,
};

buffer_key_stg g_key_ready_data = {0};
/*============================================================================*
 *                         External Functions
 *============================================================================*/

extern void data_uart_send(uint8_t *pbuf, uint16_t length);
/*============================================================================*
*                         Local Functions
*============================================================================*/
uint16_t  VS_RD_Reg(uint8_t address);             //���Ĵ���
uint16_t  VS_WRAM_Read(uint16_t addr);            //��RAM
void VS_WRAM_Write(uint16_t addr, uint16_t val);  //дRAM
void VS_WR_Data(uint8_t data);               //д����
void VS_WR_Cmd(uint8_t address, uint16_t data);   //д����
uint8_t   VS_HD_Reset(void);                 //Ӳ��λ
void VS_Soft_Reset(void);               //��λ
uint16_t VS_Ram_Test(void);                  //RAM����
void VS_Sine_Test(void);                //���Ҳ���

uint8_t   VS_SPI_ReadWriteByte(uint8_t data);
void VS_SPI_SpeedLow(void);
void VS_SPI_SpeedHigh(void);
void VS_Init(void);                     //��ʼ��VS10XX
void VS_Set_Speed(uint8_t t);                //���ò����ٶ�
uint16_t  VS_Get_HeadInfo(void);             //�õ�������
uint32_t VS_Get_ByteRate(void);              //�õ��ֽ�����
uint16_t VS_Get_EndFillByte(void);           //�õ�����ֽ�
uint8_t   VS_Send_MusicData(uint8_t *buf);        //��VS10XX����32�ֽ�
void VS_Restart_Play(void);             //���¿�ʼ��һ�׸貥��
void VS_Reset_DecodeTime(void);         //�������ʱ��
uint16_t  VS_Get_DecodeTime(void);           //�õ�����ʱ��

void VS_Load_Patch(uint16_t *patch, uint16_t len); //�����û�patch
uint8_t   VS_Get_Spec(uint16_t *p);               //�õ���������
void VS_Set_Bands(uint16_t *buf, uint8_t bands);  //��������Ƶ��
void VS_Set_Vol(uint8_t volx);               //����������
void VS_Set_Bass(uint8_t bfreq, uint8_t bass, uint8_t tfreq, uint8_t treble); //���øߵ���
void VS_Set_Effect(uint8_t eft);             //������Ч
void VS_SPK_Set(uint8_t sw);                 //��������������ؿ���
void VS_Set_All(void);


/*******************************************************
*                 SPI Drivers
*******************************************************/
void spi0_pinmux_config(void)
{
    Pinmux_Deinit(SPI0_SCK_PIN);
    Pinmux_Deinit(SPI0_MOSI_PIN);
    Pinmux_Deinit(SPI0_MISO_PIN);
    //Pinmux_Deinit(SPI0_CS_PIN);
    Pinmux_Config(SPI0_SCK_PIN, SPI0_CLK_MASTER);
    Pinmux_Config(SPI0_MOSI_PIN, SPI0_MO_MASTER);
    Pinmux_Config(SPI0_MISO_PIN, SPI0_MI_MASTER);
}
/**
* @brief spi0 pins pad config
*
*/
void spi0_init_pad_config(void)
{
    Pad_Config(SPI0_SCK_PIN, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_ENABLE, PAD_OUT_HIGH);
    Pad_Config(SPI0_MOSI_PIN, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_ENABLE, PAD_OUT_HIGH);
    Pad_Config(SPI0_MISO_PIN, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_ENABLE, PAD_OUT_HIGH);
    //Pad_Config(SPI0_CS_PIN, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_ENABLE, PAD_OUT_HIGH);
}
/**
* @brief vs1053b module config
* @param none
*/
void vs1053b_driver_spi_init(void)
{
    RCC_PeriphClockCmd(APBPeriph_SPI0, APBPeriph_SPI0_CLOCK, ENABLE);

    spi0_pinmux_config();
    spi0_init_pad_config();

    SPI_InitTypeDef  SPI_InitStruct;
    SPI_StructInit(&SPI_InitStruct);

    SPI_InitStruct.SPI_Direction   = SPI_Direction_FullDuplex;
    SPI_InitStruct.SPI_Mode        = SPI_Mode_Master;
    SPI_InitStruct.SPI_DataSize    = SPI_DataSize_8b;
    SPI_InitStruct.SPI_CPOL        = SPI_CPOL_High;
    SPI_InitStruct.SPI_CPHA        = SPI_CPHA_2Edge;
    SPI_InitStruct.SPI_BaudRatePrescaler  = 16;
    /* SPI_Direction_EEPROM mode read data lenth. */
    SPI_InitStruct.SPI_RxThresholdLevel  = 1;/* Flash id lenth = 3*/
    SPI_InitStruct.SPI_NDF               = 0;/* Flash id lenth = 3*/
    SPI_InitStruct.SPI_FrameFormat = SPI_Frame_Motorola;
    SPI_Init(VS1053B_SPIx, &SPI_InitStruct);

    SPI_Cmd(VS1053B_SPIx, ENABLE);
}

void vs1053b_gpio_init(void)
{
    /*0. pad config*/
    Pad_Config(VS_DQ_PIN, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_LOW);
    Pad_Config(VS_RST_PIN, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE, PAD_OUT_HIGH);
    Pad_Config(VS_XCS_PIN, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE, PAD_OUT_HIGH);
    Pad_Config(VS_XDCS_PIN, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE, PAD_OUT_HIGH);

    /*1. pinmux config*/
    Pinmux_Config(VS_DQ_PIN, DWGPIO);
    Pinmux_Config(VS_RST_PIN,  DWGPIO);
    Pinmux_Config(VS_XCS_PIN,  DWGPIO);
    Pinmux_Config(VS_XDCS_PIN, DWGPIO);

    /*2. clock*/
    RCC_PeriphClockCmd(APBPeriph_GPIO, APBPeriph_GPIO_CLOCK, ENABLE);

    /*3. gpio init*/
    GPIO_InitTypeDef GPIO_InitStruct;
    GPIO_StructInit(&GPIO_InitStruct);

    GPIO_InitStruct.GPIO_Pin    = GPIO_GetPin(VS_DQ_PIN);
    GPIO_InitStruct.GPIO_Mode   = GPIO_Mode_IN;
    GPIO_InitStruct.GPIO_ITCmd  = DISABLE;
    GPIO_Init(&GPIO_InitStruct);

    GPIO_StructInit(&GPIO_InitStruct);
    GPIO_InitStruct.GPIO_Pin    = GPIO_GetPin(VS_RST_PIN);
    GPIO_InitStruct.GPIO_Mode   = GPIO_Mode_OUT;
    GPIO_InitStruct.GPIO_ITCmd  = DISABLE;
    GPIO_Init(&GPIO_InitStruct);

    GPIO_StructInit(&GPIO_InitStruct);
    GPIO_InitStruct.GPIO_Pin    = GPIO_GetPin(VS_XCS_PIN);
    GPIO_InitStruct.GPIO_Mode   = GPIO_Mode_OUT;
    GPIO_InitStruct.GPIO_ITCmd  = DISABLE;
    GPIO_Init(&GPIO_InitStruct);

    GPIO_StructInit(&GPIO_InitStruct);
    GPIO_InitStruct.GPIO_Pin    = GPIO_GetPin(VS_XDCS_PIN);
    GPIO_InitStruct.GPIO_Mode   = GPIO_Mode_OUT;
    GPIO_InitStruct.GPIO_ITCmd  = DISABLE;
    GPIO_Init(&GPIO_InitStruct);

}



uint8_t vs1053b_spi_write_read(uint8_t addr)
{
    uint8_t write_value;
    uint8_t len = 0;
    uint8_t retry = 0;
    uint8_t ret_value = 0;

    //Pad_Config(SPI0_CS_PIN, PAD_SW_MODE, PAD_IS_PWRON,  PAD_PULL_NONE, PAD_OUT_ENABLE, PAD_OUT_LOW);
    write_value = (addr & 0xff);
    SPI_SendBuffer(VS1053B_SPIx, &write_value, 1);
    while (SPI_GetFlagState(VS1053B_SPIx, SPI_FLAG_BUSY))
    {
        retry++;
        if (retry > 200) { return 0; }
    }

    len = SPI_GetRxFIFOLen(VS1053B_SPIx);
    while (len--)
    {
        ret_value = SPI_ReceiveData(VS1053B_SPIx);
    }

    return ret_value;
}

void vs1053b_SetSpeed(uint8_t SpeedSet)
{
    SPI_Change_CLK(VS1053B_SPIx, SpeedSet);
}



/*******************************************************
*                 Vs1053b module config
*******************************************************/
uint8_t VS_SPI_ReadWriteByte(uint8_t data)
{
    return vs1053b_spi_write_read(data);
}

void VS_SPI_SpeedLow(void)
{
    vs1053b_SetSpeed(SPI_BaudRatePrescaler_32);//���õ�����ģʽ
}

void VS_SPI_SpeedHigh(void)
{
    vs1053b_SetSpeed(SPI_BaudRatePrescaler_8);//���õ�����ģʽ
}

/*
* software reset vs1053b
*/
void VS_Soft_Reset(void)
{
    uint8_t retry = 0;
    while (VS_DQ == 0);                 //wait software reset end
    VS_SPI_ReadWriteByte(0Xff);         //wait for transfer
    retry = 0;
    while (VS_RD_Reg(SPI_MODE) != 0x0800) // software reset, new mode
    {
        VS_WR_Cmd(SPI_MODE, 0x0804);    //sw reset, new mode
        delay_ms(2);//wait at least 1.35ms
        if (retry++ > 100) { break; }
    }
    while (VS_DQ == 0); //wait sw reset end
    retry = 0;
    while (VS_RD_Reg(SPI_CLOCKF) != 0X9800) //set vs1053 clock, 3 frequency doubling, 1.5xADD
    {
        VS_WR_Cmd(SPI_CLOCKF, 0X9800);  //set vs1053 clock, 3 frequency doubling, 1.5xADD
        if (retry++ > 100) { break; }
    }
    delay_ms(20);
}
uint8_t VS_HD_Reset(void)
{
    uint8_t retry = 0;
    VS_RST_RESET();
    delay_ms(200);
    VS_XDCS_SET(); //cancel data transfer
    VS_XCS_SET(); //cancel data transfer
    VS_RST_SET();
    while (VS_DQ == 0 && retry < 200) //wait DREQ high
    {
        retry++;
        delay_us(50);
    };
    delay_ms(200);
    if (retry >= 200) { return 1; }
    else { return 0; }
}

//���Ҳ���
void VS_Sine_Test(void)
{
    VS_HD_Reset();
    VS_WR_Cmd(0x0b, 0X2020);  //��������
    VS_WR_Cmd(SPI_MODE, 0x0820); //����VS10XX�Ĳ���ģʽ
    while (VS_DQ == 0);  //�ȴ�DREQΪ��
    //printf("mode sin:%x\n",VS_RD_Reg(SPI_MODE));
    //��VS10XX�������Ҳ������0x53 0xef 0x6e n 0x00 0x00 0x00 0x00
    //����n = 0x24, �趨VS10XX�����������Ҳ���Ƶ��ֵ��������㷽����VS10XX��datasheet
    VS_SPI_SpeedLow();//����
    VS_XDCS_RESET(); //ѡ�����ݴ���
    VS_SPI_ReadWriteByte(0x53);
    VS_SPI_ReadWriteByte(0xef);
    VS_SPI_ReadWriteByte(0x6e);
    VS_SPI_ReadWriteByte(0x24);
    VS_SPI_ReadWriteByte(0x00);
    VS_SPI_ReadWriteByte(0x00);
    VS_SPI_ReadWriteByte(0x00);
    VS_SPI_ReadWriteByte(0x00);
    delay_ms(400);
    VS_XDCS_SET();
    //�˳����Ҳ���
    VS_XDCS_RESET(); //ѡ�����ݴ���
    VS_SPI_ReadWriteByte(0x45);
    VS_SPI_ReadWriteByte(0x78);
    VS_SPI_ReadWriteByte(0x69);
    VS_SPI_ReadWriteByte(0x74);
    VS_SPI_ReadWriteByte(0x00);
    VS_SPI_ReadWriteByte(0x00);
    VS_SPI_ReadWriteByte(0x00);
    VS_SPI_ReadWriteByte(0x00);
    delay_ms(200);
    VS_XDCS_SET();

    //�ٴν������Ҳ��Բ�����nֵΪ0x44���������Ҳ���Ƶ������Ϊ�����ֵ
    VS_XDCS_RESET(); //ѡ�����ݴ���
    VS_SPI_ReadWriteByte(0x53);
    VS_SPI_ReadWriteByte(0xef);
    VS_SPI_ReadWriteByte(0x6e);
    VS_SPI_ReadWriteByte(0x44);
    VS_SPI_ReadWriteByte(0x00);
    VS_SPI_ReadWriteByte(0x00);
    VS_SPI_ReadWriteByte(0x00);
    VS_SPI_ReadWriteByte(0x00);
    delay_ms(200);
    VS_XDCS_SET();
    //�˳����Ҳ���
    VS_XDCS_RESET(); //ѡ�����ݴ���
    VS_SPI_ReadWriteByte(0x45);
    VS_SPI_ReadWriteByte(0x78);
    VS_SPI_ReadWriteByte(0x69);
    VS_SPI_ReadWriteByte(0x74);
    VS_SPI_ReadWriteByte(0x00);
    VS_SPI_ReadWriteByte(0x00);
    VS_SPI_ReadWriteByte(0x00);
    VS_SPI_ReadWriteByte(0x00);
    delay_ms(200);
    VS_XDCS_SET();


}

//��VS10XXд����
//address:�����ַ
//data:��������
void VS_WR_Cmd(uint8_t address, uint16_t data)
{
    //DBG_DIRECT("003");
    while (VS_DQ == 0); //�ȴ�����
    //DBG_DIRECT("004");
    VS_SPI_SpeedLow();//����
    VS_XDCS_SET();
    VS_XCS_RESET();
    VS_SPI_ReadWriteByte(VS_WRITE_COMMAND);//����VS10XX��д����
    VS_SPI_ReadWriteByte(address);  //��ַ
    VS_SPI_ReadWriteByte(data >> 8); //���͸߰�λ
    VS_SPI_ReadWriteByte(data);     //�ڰ�λ
    VS_XCS_SET();
    VS_SPI_SpeedHigh();             //����
}
//��VS10XXд����
//data:Ҫд�������
void VS_WR_Data(uint8_t data)
{
    VS_SPI_SpeedHigh();//����,��VS1003B,���ֵ���ܳ���36.864/4Mhz����������Ϊ9M
    VS_XDCS_RESET();
    VS_SPI_ReadWriteByte(data);
    VS_XDCS_SET();
}
//��VS10XX�ļĴ���
//address���Ĵ�����ַ
//����ֵ��������ֵ
//ע�ⲻҪ�ñ��ٶ�ȡ,�����
uint16_t VS_RD_Reg(uint8_t address)
{
    uint16_t temp = 0;
    while (VS_DQ == 0); //�ǵȴ�����״̬
    VS_SPI_SpeedLow();//����
    VS_XDCS_SET();
    VS_XCS_RESET();
    VS_SPI_ReadWriteByte(VS_READ_COMMAND);  //����VS10XX�Ķ�����
    VS_SPI_ReadWriteByte(address);          //��ַ
    temp = VS_SPI_ReadWriteByte(0xff);      //��ȡ���ֽ�
    temp = temp << 8;
    temp += VS_SPI_ReadWriteByte(0xff);     //��ȡ���ֽ�
    VS_XCS_SET();
    VS_SPI_SpeedHigh();//����
    return temp;
}
//��ȡVS10xx��RAM
//addr��RAM��ַ
//����ֵ��������ֵ
uint16_t VS_WRAM_Read(uint16_t addr)
{
    uint16_t res;
    VS_WR_Cmd(SPI_WRAMADDR, addr);
    res = VS_RD_Reg(SPI_WRAM);
    return res;
}
//дVS10xx��RAM
//addr��RAM��ַ
//val:Ҫд���ֵ
void VS_WRAM_Write(uint16_t addr, uint16_t val)
{
    VS_WR_Cmd(SPI_WRAMADDR, addr);  //дRAM��ַ
    while (VS_DQ == 0);             //�ȴ�����
    VS_WR_Cmd(SPI_WRAM, val);       //дRAMֵ
}
//���ò����ٶȣ���VS1053��Ч��
//t:0,1,�����ٶ�;2,2���ٶ�;3,3���ٶ�;4,4����;�Դ�����
void VS_Set_Speed(uint8_t t)
{
    VS_WRAM_Write(0X1E04, t);       //д�벥���ٶ�
}
//FOR WAV HEAD0 :0X7761 HEAD1:0X7665
//FOR MIDI HEAD0 :other info HEAD1:0X4D54
//FOR WMA HEAD0 :data speed HEAD1:0X574D
//FOR MP3 HEAD0 :data speed HEAD1:ID
//������Ԥ��ֵ,�ײ�III
const uint16_t bitrate[2][16] =
{
    {0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160, 0},
    {0, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 0}
};
//����Kbps�Ĵ�С
//����ֵ���õ�������
uint16_t VS_Get_HeadInfo(void)
{
    unsigned int HEAD0;
    unsigned int HEAD1;
    HEAD0 = VS_RD_Reg(SPI_HDAT0);
    HEAD1 = VS_RD_Reg(SPI_HDAT1);
    //printf("(H0,H1):%x,%x\n",HEAD0,HEAD1);
    switch (HEAD1)
    {
    case 0x7665://WAV��ʽ
    case 0X4D54://MIDI��ʽ
    case 0X4154://AAC_ADTS
    case 0X4144://AAC_ADIF
    case 0X4D34://AAC_MP4/M4A
    case 0X4F67://OGG
    case 0X574D://WMA��ʽ
    case 0X664C://FLAC��ʽ
        {
            ////printf("HEAD0:%d\n",HEAD0);
            HEAD1 = HEAD0 * 2 / 25; //�൱��*8/100
            if ((HEAD1 % 10) > 5) { return HEAD1 / 10 + 1; } //��С�����һλ��������
            else { return HEAD1 / 10; }
        }
    default://MP3��ʽ,�����˽ײ�III�ı�
        {
            HEAD1 >>= 3;
            HEAD1 = HEAD1 & 0x03;
            if (HEAD1 == 3) { HEAD1 = 1; }
            else { HEAD1 = 0; }
            return bitrate[HEAD1][HEAD0 >> 12];
        }
    }
}
//�õ�ƽ���ֽ���
//����ֵ��ƽ���ֽ����ٶ�
uint32_t VS_Get_ByteRate(void)
{
    return VS_WRAM_Read(0X1E05);//ƽ��λ��
}
//�õ���Ҫ��������
//����ֵ:��Ҫ��������
uint16_t VS_Get_EndFillByte(void)
{
    return VS_WRAM_Read(0X1E06);//����ֽ�
}
//����һ����Ƶ����
//�̶�Ϊ32�ֽ�
//����ֵ:0,���ͳɹ�
//       1,VS10xx��ȱ����,��������δ�ɹ�����
uint8_t VS_Send_MusicData(uint8_t *buf)
{
    uint8_t n;
    if (VS_DQ != 0) //�����ݸ�VS10XX
    {
        VS_XDCS_RESET();
        for (n = 0; n < 32; n++)
        {
            VS_SPI_ReadWriteByte(buf[n]);
        }
        VS_XDCS_SET();
    }
    else { return 1; }
    return 0;//�ɹ�������
}
//�и�
//ͨ���˺����и裬��������л���������
void VS_Restart_Play(void)
{
    uint16_t temp;
    uint16_t i;
    uint8_t n;
    uint8_t vsbuf[32];
    for (n = 0; n < 32; n++) { vsbuf[n] = 0; } //����
    temp = VS_RD_Reg(SPI_MODE); //��ȡSPI_MODE������
    temp |= 1 << 3;             //����SM_CANCELλ
    temp |= 1 << 2;             //����SM_LAYER12λ,������MP1,MP2
    VS_WR_Cmd(SPI_MODE, temp);  //����ȡ����ǰ����ָ��
    for (i = 0; i < 2048;)      //����2048��0,�ڼ��ȡSM_CANCELλ.���Ϊ0,���ʾ�Ѿ�ȡ���˵�ǰ����
    {
        if (VS_Send_MusicData(vsbuf) == 0) //ÿ����32���ֽں���һ��
        {
            i += 32;                    //������32���ֽ�
            temp = VS_RD_Reg(SPI_MODE); //��ȡSPI_MODE������
            if ((temp & (1 << 3)) == 0) { break; } //�ɹ�ȡ����
        }
    }
    if (i < 2048) //SM_CANCEL����
    {
        temp = VS_Get_EndFillByte() & 0xff; //��ȡ����ֽ�
        for (n = 0; n < 32; n++) { vsbuf[n] = temp; } //����ֽڷ�������
        for (i = 0; i < 2052;)
        {
            if (VS_Send_MusicData(vsbuf) == 0) { i += 32; } //���
        }
    }
    else { VS_Soft_Reset(); }       //SM_CANCEL���ɹ�,�����,��Ҫ��λ
    temp = VS_RD_Reg(SPI_HDAT0);
    temp += VS_RD_Reg(SPI_HDAT1);
    if (temp)                   //��λ,����û�гɹ�ȡ��,��ɱ���,Ӳ��λ
    {
        VS_HD_Reset();          //Ӳ��λ
        VS_Soft_Reset();        //��λ
    }
}
//�������ʱ��
void VS_Reset_DecodeTime(void)
{
    VS_WR_Cmd(SPI_DECODE_TIME, 0x0000);
    VS_WR_Cmd(SPI_DECODE_TIME, 0x0000); //��������
}
//�õ�mp3�Ĳ���ʱ��n sec
//����ֵ������ʱ��
uint16_t VS_Get_DecodeTime(void)
{
    uint16_t dt = 0;
    dt = VS_RD_Reg(SPI_DECODE_TIME);
    return dt;
}
//vs10xxװ��patch.
//patch��patch�׵�ַ
//len��patch����
void VS_Load_Patch(uint16_t *patch, uint16_t len)
{
    uint16_t i;
    uint16_t addr, n, val;
    for (i = 0; i < len;)
    {
        addr = patch[i++];
        n    = patch[i++];
        if (n & 0x8000U) //RLE run, replicate n samples
        {
            n  &= 0x7FFF;
            val = patch[i++];
            while (n--) { VS_WR_Cmd(addr, val); }
        }
        else  //copy run, copy n sample
        {
            while (n--)
            {
                val = patch[i++];
                VS_WR_Cmd(addr, val);
            }
        }
    }
}
//�趨VS10XX���ŵ������͸ߵ���
//volx:������С(0~254)
void VS_Set_Vol(uint8_t volx)
{
    uint16_t volt = 0;           //�ݴ�����ֵ
    volt = 254 - volx;      //ȡ��һ��,�õ����ֵ,��ʾ���ı�ʾ
    volt <<= 8;
    volt += 254 - volx;     //�õ��������ú��С
    VS_WR_Cmd(SPI_VOL, volt); //������
}
//�趨�ߵ�������
//bfreq:��Ƶ����Ƶ��    2~15(��λ:10Hz)
//bass:��Ƶ����         0~15(��λ:1dB)
//tfreq:��Ƶ����Ƶ��    1~15(��λ:Khz)
//treble:��Ƶ����       0~15(��λ:1.5dB,С��9��ʱ��Ϊ����)
void VS_Set_Bass(uint8_t bfreq, uint8_t bass, uint8_t tfreq, uint8_t treble)
{
    uint16_t bass_set = 0; //�ݴ������Ĵ���ֵ
    signed char temp = 0;
    if (treble == 0) { temp = 0; }      //�任
    else if (treble > 8) { temp = treble - 8; }
    else { temp = treble - 9; }
    bass_set = temp & 0X0F;         //�����趨
    bass_set <<= 4;
    bass_set += tfreq & 0xf;        //��������Ƶ��
    bass_set <<= 4;
    bass_set += bass & 0xf;         //�����趨
    bass_set <<= 4;
    bass_set += bfreq & 0xf;        //��������
    VS_WR_Cmd(SPI_BASS, bass_set);  //BASS
}
//�趨��Ч
//eft:0,�ر�;1,��С;2,�е�;3,���.
void VS_Set_Effect(uint8_t eft)
{
    uint16_t temp;
    temp = VS_RD_Reg(SPI_MODE); //��ȡSPI_MODE������
    if (eft & 0X01) { temp |= 1 << 4; } //�趨LO
    else { temp &= ~(1 << 5); }     //ȡ��LO
    if (eft & 0X02) { temp |= 1 << 7; } //�趨HO
    else { temp &= ~(1 << 7); }     //ȡ��HO
    VS_WR_Cmd(SPI_MODE, temp);  //�趨ģʽ
}
//�������ȿ�/�����ú���.
//ս��V3������HT6872����,ͨ��VS1053��GPIO4(36��),�����乤��/�ر�.
//GPIO4=1,HT6872��������.
//GPIO4=0,HT6872�ر�(Ĭ��)
//sw:0,�ر�;1,����.
void VS_SPK_Set(uint8_t sw)
{
    VS_WRAM_Write(GPIO_DDR, 1 << 4); //VS1053��GPIO4���ó����
    VS_WRAM_Write(GPIO_ODATA, sw << 4); //����VS1053��GPIO4���ֵ(0/1)
}
///////////////////////////////////////////////////////////////////////////////
//��������,��Ч��.
void VS_Set_All(void)
{
    VS_Set_Vol(vsset.mvol);         //��������
    VS_Set_Bass(vsset.bflimit, vsset.bass, vsset.tflimit, vsset.treble);
    VS_Set_Effect(vsset.effect);    //���ÿռ�Ч��
    VS_SPK_Set(vsset.speakersw);    //���ư�������״̬
}

//inactive PCM record mode
void recoder_enter_rec_mode(uint16_t agc)
{
    VS_WR_Cmd(SPI_BASS, 0x0000);
    VS_WR_Cmd(SPI_AICTRL0, 16000);
    VS_WR_Cmd(SPI_AICTRL1, agc);
    VS_WR_Cmd(SPI_AICTRL2, 0);
    VS_WR_Cmd(SPI_AICTRL3, 6);
    VS_WR_Cmd(SPI_CLOCKF, 0X2000);
    //VS_WR_Cmd(SPI_MODE, 0x1804);
    VS_WR_Cmd(SPI_MODE, 0x5804);
    delay_ms(20);
    VS_Load_Patch((uint16_t *)wav_plugin, 40);// load patch
}
//init wav header
void recoder_wav_init(__WaveHeader *wavhead) //��ʼ��WAVͷ
{
    wavhead->riff.ChunkID = 0X46464952; //"RIFF"
    wavhead->riff.ChunkSize = 0;
    wavhead->riff.Format = 0X45564157;  //"WAVE"
    wavhead->fmt.ChunkID = 0X20746D66;  //"fmt "
    wavhead->fmt.ChunkSize = 16;        //16 bytes size
    wavhead->fmt.AudioFormat = 0X01;    //0X01,PCM;   0X01, IMA ADPCM
    wavhead->fmt.NumOfChannels = 1;     //mono
    wavhead->fmt.SampleRate = 8000;     //8Khz
    wavhead->fmt.ByteRate = wavhead->fmt.SampleRate * 2; //16bits
    wavhead->fmt.BlockAlign = 2;        //block size, 2bytes per block
    wavhead->fmt.BitsPerSample = 16;    //16 bits pcm
    wavhead->data.ChunkID = 0X61746164; //"data"
    wavhead->data.ChunkSize = 0;        //data size
}

void vs1053b_recoder_start(uint8_t recagc)
{
    recoder_enter_rec_mode(1024 * recagc);
}

uint16_t vs1053b_recoder_check_buffer_length()
{
    return (uint16_t)VS_RD_Reg(SPI_HDAT1);
}

void vs1053b_read_buffer(uint16_t num)
{
    uint16_t data = 0;

    for (uint16_t index = 0; index < num; index ++)
    {
        data = VS_RD_Reg(SPI_HDAT0);

        //data_uart_send(&data, sizeof(data));

        key_queue_in(data & 0xff);

        key_queue_in(((data & 0xff00) >> 8) & 0xff);
    }


}

uint8_t recoder_play(void)
{
    uint8_t rval = 0;
    //uint8_t recbuf[512] = {0};                     //�����ڴ�
    uint16_t w;
    uint8_t recagc = 4;                  //Ĭ������Ϊ4

    if (rval == 0)                              //�ڴ�����OK
    {
        //DBG_DIRECT("001");
        recoder_enter_rec_mode(1024 * recagc);
        //while (VS_RD_Reg(SPI_HDAT1));       //�ȵ�buf ��Ϊ�����ٿ�ʼ
        //DBG_DIRECT("002");
        while (1)
        {
            w = VS_RD_Reg(SPI_HDAT1);
            if (w >= 256)
            {
                //while (idx < 512) //һ�ζ�ȡ512�ֽ�
                {
                    w = VS_RD_Reg(SPI_HDAT0);

                    data_uart_send((uint8_t *)&w, sizeof(w));
                }
                //res = f_write(f_rec, recbuf, 512, &bw); //д���ļ�
            }
        }
    }
    return rval;
}

void data_uart_send(uint8_t *pbuf, uint16_t length)
{
    uint16_t blk_cnt, remainder;
    uint8_t *p_buf = pbuf;

    blk_cnt = (length) / UART_TX_FIFO_SIZE;
    remainder = (length) % UART_TX_FIFO_SIZE;
    /* send voice data through uart */
    for (int i = 0; i < blk_cnt; i++)
    {
        /* 1. max send 16 bytes(Uart tx and rx fifo size is 16) */
        UART_SendData(UART, p_buf, 16);
        /* wait tx fifo empty */
        while (UART_GetFlagState(UART, UART_FLAG_THR_EMPTY) != SET);
        p_buf += 16;
    }

    /* send left bytes */
    UART_SendData(UART, p_buf, remainder);
    /* wait tx fifo empty */
    while (UART_GetFlagState(UART, UART_FLAG_THR_EMPTY) != SET);
}

/*=============================================================
*                buffer voice data queue
*==============================================================*/
/**
* @brief   check key queue full
* @param   none
* @return  bool true full, false not full
*/
bool is_key_queue_full(void)
{
    if (((g_key_ready_data.head + 1) % VOICE_DATA_BUFFER_LENGTH) == g_key_ready_data.tail)
    {
#if 1//KEY_BUFFER_LOG_EN
        APP_PRINT_INFO0("[is_key_queue_full] is full");
#endif
        return true;
    }
    else
    {
#if KEY_BUFFER_LOG_EN
        //APP_PRINT_INFO0("[is_key_queue_full] not full");
#endif
        return false;
    }
}
/**
* @brief   check key queue empty
* @param   none
* @return  bool true empty, false not empty
*/
bool is_key_queue_empty(void)
{
    if (g_key_ready_data.head == g_key_ready_data.tail)
    {
#if 1//KEY_BUFFER_LOG_EN
        APP_PRINT_INFO0("[is_key_queue_empty] is empty");
#endif
        return true;
    }
    else
    {
#if KEY_BUFFER_LOG_EN
        //APP_PRINT_INFO0("[is_key_queue_empty] not empty");
#endif
        return false;
    }
}

/**
* @brief   buffer key in queue
* @param   uint8_t key_index value in queue
* @return  bool true inqueue success, else false
*/
bool key_queue_in(uint8_t key_index)
{
    if (is_key_queue_full())
    {
#if KEY_BUFFER_LOG_EN
        //full queue
        APP_PRINT_INFO0("[key_queue_in] drop the oldest data, full");
#endif
        g_key_ready_data.tail = (g_key_ready_data.tail + 1) % VOICE_DATA_BUFFER_LENGTH;;
    }

    if (is_key_queue_full())
    {
#if KEY_BUFFER_LOG_EN
        APP_PRINT_INFO0("[key_queue_in] queue in failed, full");
#endif
        return false;
    }


    g_key_ready_data.buf[g_key_ready_data.head] = key_index;
    g_key_ready_data.head = (g_key_ready_data.head + 1) % VOICE_DATA_BUFFER_LENGTH;
#if KEY_BUFFER_LOG_EN
    APP_PRINT_INFO1("[key_queue_in] queue in success, key_index = %d", key_index);
    APP_PRINT_INFO2("[key_queue_in] queue, g_key_ready_data.head = %d, g_key_ready_data.tail = %d",
                    g_key_ready_data.head, g_key_ready_data.tail);
#endif

    return true;
}

/**
* @brief   buffer key out queue
* @param   none
* @return  uint8_t value out queue
*/
bool key_queue_out(uint8_t *poBuf, uint16_t length)
{
    if (poBuf == NULL)
    {
        return false;
    }

    if (length == 0)
    {
        return false;
    }

    //uint8_t key_index = VK_NC;
    if (is_key_queue_empty())
    {
#if KEY_BUFFER_LOG_EN
        //empty
        APP_PRINT_INFO0("[key_queue_out] queue out failed, empty.");
#endif
        return false;
    }
    else
    {
#if KEY_BUFFER_LOG_EN
        APP_PRINT_INFO2("[key_queue_out]before, head = %d, tail = %d",
                        g_key_ready_data.head, g_key_ready_data.tail);
#endif

        for (uint16_t index = 0; index < (length); index ++)
        {
            *(poBuf + index) = g_key_ready_data.buf[(g_key_ready_data.tail)];
            g_key_ready_data.tail = (g_key_ready_data.tail + 1) % VOICE_DATA_BUFFER_LENGTH;
        }
#if KEY_BUFFER_LOG_EN
        APP_PRINT_INFO2("[key_queue_out] after, head = %d, tail = %d",
                        g_key_ready_data.head, g_key_ready_data.tail);
#endif
    }

    return true;
}
/**
* @brief   clear key buffer queue
* @param   none
* @return  none
* @note    noly clear when triggle reconnect adv
*/
bool key_queue_clear(void)
{
#if KEY_BUFFER_LOG_EN
    APP_PRINT_INFO0("[key_queue_clear] key queue clear.");
#endif
    g_key_ready_data.head = g_key_ready_data.tail;

    memset((void *)&g_key_ready_data, 0, sizeof(g_key_ready_data));
    return true;
}

bool key_queue_print(uint8_t idx)
{
#if KEY_BUFFER_LOG_EN
    APP_PRINT_INFO3("[key_queue_print (%d)] queue, head = %d, tail = %d",
                    idx, g_key_ready_data.head, g_key_ready_data.tail);
#endif

    for (uint8_t index = g_key_ready_data.tail; index != (g_key_ready_data.head);)
    {
        APP_PRINT_INFO2("[key_queue_print] g_key_ready_data[%d] = %d", index, g_key_ready_data.buf[index]);
        index = (index + 1) % VOICE_DATA_BUFFER_LENGTH;
    }

    return true;
}

bool key_queue_check_level(uint16_t level_value)
{
    if (is_key_queue_empty())
    {
//            APP_PRINT_INFO2("[key_queue_out0] false key_queue_check_level, head = %d, tail = %d",
//                                      g_key_ready_data.head, g_key_ready_data.tail);
        return false;
    }


    if (g_key_ready_data.tail < g_key_ready_data.head)
    {
        if (g_key_ready_data.tail + level_value < g_key_ready_data.head)
        {
//                      APP_PRINT_INFO2("[key_queue_out2] true key_queue_check_level, head = %d, tail = %d",
//                                      g_key_ready_data.head, g_key_ready_data.tail);
            return true;
        }
        else
        {
//                     APP_PRINT_INFO2("[key_queue_out1] false key_queue_check_level, head = %d, tail = %d",
//                                      g_key_ready_data.head, g_key_ready_data.tail);
            return false;
        }
    }
    else
    {
        if ((g_key_ready_data.tail + level_value) < (g_key_ready_data.head + VOICE_DATA_BUFFER_LENGTH))
        {
//                    APP_PRINT_INFO2("[key_queue_out2] true key_queue_check_level, head = %d, tail = %d",
//                                      g_key_ready_data.head, g_key_ready_data.tail);
            return true;
        }
        else
        {
//                    APP_PRINT_INFO2("[key_queue_out2] false key_queue_check_level, head = %d, tail = %d",
//                                      g_key_ready_data.head, g_key_ready_data.tail);
            return false;
        }
    }
}

#endif
