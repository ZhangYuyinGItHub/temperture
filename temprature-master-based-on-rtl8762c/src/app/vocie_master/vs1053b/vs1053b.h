#ifndef _VS1053_HEAD_
#define _VS1053_HEAD_
#include "rtl876x.h"

#define  VS1053B_SPIx   SPI0


//RIFF��
typedef __packed struct
{
    uint32_t ChunkID;            //chunk id;����̶�Ϊ"RIFF",��0X46464952
    uint32_t ChunkSize ;         //���ϴ�С;�ļ��ܴ�С-8
    uint32_t Format;             //��ʽ;WAVE,��0X45564157
} ChunkRIFF ;
//fmt��
typedef __packed struct
{
    uint32_t ChunkID;            //chunk id;����̶�Ϊ"fmt ",��0X20746D66
    uint32_t ChunkSize ;         //�Ӽ��ϴ�С(������ID��Size);����Ϊ:20.
    uint16_t AudioFormat;        //��Ƶ��ʽ;0X10,��ʾ����PCM;0X11��ʾIMA ADPCM
    uint16_t NumOfChannels;      //ͨ������;1,��ʾ������;2,��ʾ˫����;
    uint32_t SampleRate;         //������;0X1F40,��ʾ8Khz
    uint32_t ByteRate;           //�ֽ�����;
    uint16_t BlockAlign;         //�����(�ֽ�);
    uint16_t BitsPerSample;      //�����������ݴ�С;4λADPCM,����Ϊ4
//  uint16_t ByteExtraData;      //���ӵ������ֽ�;2��; ����PCM,û���������
//  uint16_t ExtraData;          //���ӵ�����,�����������ݿ��С;0X1F9:505�ֽ�  ����PCM,û���������
} ChunkFMT;
//fact��
typedef __packed struct
{
    uint32_t ChunkID;            //chunk id;����̶�Ϊ"fact",��0X74636166;
    uint32_t ChunkSize ;         //�Ӽ��ϴ�С(������ID��Size);����Ϊ:4.
    uint32_t NumOfSamples;       //����������;
} ChunkFACT;
//data��
typedef __packed struct
{
    uint32_t ChunkID;            //chunk id;����̶�Ϊ"data",��0X61746164
    uint32_t ChunkSize ;         //�Ӽ��ϴ�С(������ID��Size);�ļ���С-60.
} ChunkDATA;

//wavͷ
typedef __packed struct
{
    ChunkRIFF riff; //riff��
    ChunkFMT fmt;   //fmt��
    //ChunkFACT fact;   //fact�� ����PCM,û������ṹ��
    ChunkDATA data; //data��
} __WaveHeader;

__packed typedef struct
{
    uint8_t mvol;        //������,��Χ:0~254
    uint8_t bflimit;     //����Ƶ���޶�,��Χ:2~15(��λ:10Hz)
    uint8_t bass;        //����,��Χ:0~15.0��ʾ�ر�.(��λ:1dB)
    uint8_t tflimit;     //����Ƶ���޶�,��Χ:1~15(��λ:Khz)
    uint8_t treble;      //����,��Χ:0~15(��λ:1.5dB)(ԭ����Χ��:-8~7,ͨ�������޸���);
    uint8_t effect;      //�ռ�Ч������.0,�ر�;1,��С;2,�е�;3,���.
    uint8_t speakersw;   //�������ȿ���,0,�ر�;1,��
    uint8_t saveflag;    //�����־,0X0A,�������;����,����δ����
} _vs10xx_obj;
#define VS_WRITE_COMMAND    0x02
#define VS_READ_COMMAND     0x03
//VS10XX�Ĵ�������
#define SPI_MODE            0x00
#define SPI_STATUS          0x01
#define SPI_BASS            0x02
#define SPI_CLOCKF          0x03
#define SPI_DECODE_TIME     0x04
#define SPI_AUDATA          0x05
#define SPI_WRAM            0x06
#define SPI_WRAMADDR        0x07
#define SPI_HDAT0           0x08
#define SPI_HDAT1           0x09

#define SPI_AIADDR          0x0a
#define SPI_VOL             0x0b
#define SPI_AICTRL0         0x0c
#define SPI_AICTRL1         0x0d
#define SPI_AICTRL2         0x0e
#define SPI_AICTRL3         0x0f
#define SM_DIFF             0x01
#define SM_JUMP             0x02
#define SM_RESET            0x04
#define SM_OUTOFWAV         0x08
#define SM_PDOWN            0x10
#define SM_TESTS            0x20
#define SM_STREAM           0x40
#define SM_PLUSV            0x80
#define SM_DACT             0x100
#define SM_SDIORD           0x200
#define SM_SDISHARE         0x400
#define SM_SDINEW           0x800
#define SM_ADPCM            0x1000
#define SM_ADPCM_HP         0x2000

#define I2S_CONFIG          0XC040
#define GPIO_DDR            0XC017
#define GPIO_IDATA          0XC018
#define GPIO_ODATA          0XC019


#define BIT_POOL_SIZE   28//14  /* BIT_POOL_SIZE is support to adjust */
#define VOICE_PCM_FRAME_CNT              2
#define VOICE_PCM_FRAME_SIZE             256
#define VOICE_FRAME_SIZE_AFTER_ENC       (2 * BIT_POOL_SIZE + 8)
#define VOICE_REPORT_FRAME_SIZE          (VOICE_FRAME_SIZE_AFTER_ENC * VOICE_PCM_FRAME_CNT)
#define VOICE_GDMA_FRAME_SIZE            (VOICE_PCM_FRAME_SIZE * VOICE_PCM_FRAME_CNT)
typedef struct
{
    uint8_t buf0[VOICE_GDMA_FRAME_SIZE];
    uint8_t buf1[VOICE_GDMA_FRAME_SIZE];
} T_GDMA_BUF_TYPE_DEF;

typedef struct
{
    bool is_allowed_to_enter_dlps;  /* to indicate whether to allow to enter dlps or not */
    bool is_voice_driver_working;  /* indicate whether voice driver is working or not */
    uint8_t current_bibuff_index;  /* indicate which buffer the voice using now */
    //T_GDMA_BUF_TYPE_DEF voice_buf_index;
} T_VOICE_DRIVER_GLOBAL_DATA;


/*buffer data num*/
#define  BUFFER_LEN      2048
#define  VOICE_DATA_BUFFER_LENGTH      (BUFFER_LEN + 1)

#define  KEY_BUFFER_LOG_EN 0
typedef struct
{
    uint16_t   head;
    uint16_t   tail;
    uint8_t   buf[VOICE_DATA_BUFFER_LENGTH];
} buffer_key_stg;

extern void vs1053b_gpio_init(void);
extern void vs1053b_driver_spi_init(void);
extern uint8_t recoder_play(void);
extern void vs1053b_read_buffer(uint16_t num);
extern void VS_Sine_Test(void);
extern void vs1053b_recoder_start(uint8_t recagc);
extern uint16_t vs1053b_recoder_check_buffer_length(void);

extern bool key_queue_in(uint8_t key_index);
extern bool key_queue_out(uint8_t *poBuf, uint16_t length);
extern bool key_queue_clear(void);
extern bool key_queue_print(uint8_t idx);
extern bool key_queue_check_level(uint16_t level_value);

#endif
