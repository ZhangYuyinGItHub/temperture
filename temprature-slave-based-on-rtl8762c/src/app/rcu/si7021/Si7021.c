#include "SI7021.h"
#include "trace.h"
#include "rtl876x_i2c.h"
#include "si7021_i2c_driver.h"

//�ṹ�嶨��
_si7021_value si7021;//���ݻ������ṹ��
_si7021_filter si7021_filter;//ƽ��ֵ�˲����ṹ��

//�������壬�˲�������ս������ʹ�ô��ڴ�ӡ
float TEMP_buf, Humi_buf;

#if 1
//�������ƣ�single_write_Si7021
//�������ܣ����ֽ�д�봫����
//����������
////�� �� ֵ��
//void single_write_Si7021(uint8_t REG_address)
//{
//  //IIC_Start();
//
//  IIC_Send_Byte((SLAVE_ADDR<<1)|0);
//  IIC_Wait_Ack();
//
//  IIC_Send_Byte(REG_address);
//  IIC_Wait_Ack();
//
//  IIC_Stop();
//}

//�������ƣ�Multiple_read_Si7021
//�������ܣ����ֽڶ�ȡ������
//����������
//�� �� ֵ��
void Multiple_read_Si7021(uint8_t REG_address, uint16_t *value)
{
    uint8_t Si7021_BUF[2] = {0};

    RS_Sensors_I2C_ReadRegister(0x40, REG_address, sizeof(Si7021_BUF), Si7021_BUF);

//  Si7021_BUF[0] = 0x67;
//  Si7021_BUF[1] = 0xFC;

    *value = (((uint16_t)(Si7021_BUF[0] << 8) & 0xff00) | (uint8_t)Si7021_BUF[1]);

}
#endif

//�������ƣ�measure_si7021
//�������ܣ�NO HOLD MASTERģʽ�¶�ȡ��ʪ��
//������������
//�� �� ֵ����
void measure_Si7021(float *pTemp, float *pHumi)
{
    //�����������
    uint16_t TEMP, HUMI;
    uint8_t curI;

    //��ȡ�¶�
    Multiple_read_Si7021(TEMP_NOHOLD_MASTER, &TEMP);
    si7021.temp = (((((float)TEMP) * 175.72f) / 65536.0f) -
                   46.85f); //��ԭʼ�¶����ݼ���Ϊʵ���¶����ݲ����ݸ�����������λ ��
//  TEMP_buf=(((((float)TEMP)*175.72f)/65536.0f) - 46.85f);

    Multiple_read_Si7021(HUMI_NOHOLD_MASTER, &HUMI);
    si7021.humi = (((((float)HUMI) * 125.0f) / 65535.0f) -
                   6.0f); //��ԭʼʪ�����ݼ���Ϊʵ��ʪ�����ݲ����ݸ�����������λ %RH
//  Humi_buf=(((((float)HUMI)*125.0f)/65535.0f) - 6.0f);

    //����Ϊƽ��ֵ�˲����룬ѭ������10�ε����ݣ�����һ��measure_Si7021()�ʹ�һ��
    if (MEAN_NUM > si7021_filter.curI) //��MEAN_NUM==10ʱ�����10�ζ�ȡ
    {
        si7021_filter.tBufs[si7021_filter.curI] = si7021.temp;
        si7021_filter.hBufs[si7021_filter.curI] = si7021.humi;

        si7021_filter.curI++;
    }
    else
    {
        si7021_filter.curI = 0;

        si7021_filter.tBufs[si7021_filter.curI] = si7021.temp;
        si7021_filter.hBufs[si7021_filter.curI] = si7021.humi;

        si7021_filter.curI++;
    }

    if (MEAN_NUM <= si7021_filter.curI)
    {
        si7021_filter.thAmount = MEAN_NUM;
    }

    //�ж��Ƿ����ѭ��
    if (0 == si7021_filter.thAmount)
    {
        //����ɼ���10������֮ǰ��ƽ��ֵ
        for (curI = 0; curI < si7021_filter.curI; curI++)
        {
            si7021.temp += si7021_filter.tBufs[curI];
            si7021.humi += si7021_filter.hBufs[curI];
        }

        si7021.temp = si7021.temp / si7021_filter.curI;
        si7021.humi = si7021.humi / si7021_filter.curI;

        *pTemp = si7021.temp;
        *pHumi = si7021.humi;
    }
    else if (MEAN_NUM == si7021_filter.thAmount)
    {
        //����ɼ���10������֮���ƽ��ֵ
        for (curI = 0; curI < si7021_filter.thAmount; curI++)
        {
            si7021.temp += si7021_filter.tBufs[curI];
            si7021.humi += si7021_filter.hBufs[curI];
        }

        si7021.temp = si7021.temp / si7021_filter.thAmount;
        si7021.humi = si7021.humi / si7021_filter.thAmount;

        *pTemp = si7021.temp;
        *pHumi = si7021.humi;
    }
}

//#include "swtimer.h"
//#include "os_timer.h"
//#define AUTO_TEST_TIMER_TICK 2000
//TimerHandle_t auto_test_timer;
//void anto_test_timer_callback(TimerHandle_t p_timer);
///**
// * @brief auto test init.
// */
//void auto_test_init(void)
//{
//    /* no_act_disconn_timer is used to disconnect after timeout if there is on action under connection */
//    if (false == os_timer_create(&auto_test_timer, "auto_test_timer",  1, \
//                                 AUTO_TEST_TIMER_TICK, false, anto_test_timer_callback))
//    {
//        APP_PRINT_INFO0("[sw_timer_init] init no_act_disconn_timer failed");
//    }
//    else
//    {
//        APP_PRINT_INFO0("[sw_timer_init] init success");
//    }

//    os_timer_restart(&auto_test_timer, AUTO_TEST_TIMER_TICK);
//}

//void anto_test_timer_callback(TimerHandle_t p_timer)
//{
//    os_timer_restart(&auto_test_timer, AUTO_TEST_TIMER_TICK);

//    measure_Si7021();

//    APP_PRINT_INFO2("TEMP_buf = %d, Humi_buf = %f", (uint16_t)(TEMP_buf * 10),
//                    (uint16_t)(Humi_buf * 10));
//}
