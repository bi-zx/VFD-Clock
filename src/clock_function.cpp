#include <Arduino.h>
#include <cstdio>
#include <ctime>
#include <cstring>
#include <cmath>
#include <sys/time.h>
#include <cstdlib>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "sys/unistd.h"

#include "13ST84GINK.h"
#include "buzzer_driver.h"
#include "clock_function.h"
#include "configuration.h"
#include "FontLibrary.h"
#include "fs_info_RW.h"
#include "http_request.h"
#include "iic_driver.h"
#include "key_driver.h"
#include "measuring_lightIntensity.h"
#include "wifi_control.h"


static const char* TAG = "clock function";
EventGroupHandle_t KeyEventHandle = NULL; //按键事件消息队列句柄

/* 粗体0-9 ：5*7点阵取模数据*/
unsigned char numberFont[13][5] = {
    {0x7F, 0x41, 0x41, 0x7F, 0x7F}, // 0
    {0x00, 0x02, 0x7F, 0x7F, 0x00}, // 1
    {0x7B, 0x79, 0x49, 0x49, 0x6F}, // 2
    {0x63, 0x49, 0x49, 0x7F, 0x7F}, // 3
    {0x1E, 0x18, 0x18, 0x7F, 0x7F}, // 4
    {0x67, 0x45, 0x45, 0x7D, 0x7D}, // 5
    {0x7F, 0x79, 0x49, 0x49, 0x7B}, // 6
    {0x03, 0x01, 0x79, 0x7F, 0x07}, // 7
    {0x7F, 0x49, 0x49, 0x7F, 0x7F}, // 8
    {0x6F, 0x49, 0x49, 0x7F, 0x7F}, // 9
    {0x00, 0x36, 0x36, 0x00, 0x00}, //粗：
    {0x18, 0x18, 0x18, 0x18, 0x18}, //粗-
    {0x00, 0x00, 0x00, 0x00, 0x00}, //
};
/*{0x7F, 0x49, 0x45, 0x7F, 0x7F},// 0*/

/* G1 圆圈动画取模数据
	4帧 S3S10  S5S8 S6S7  S4S9 成对顺时针转圈
	S1常亮 S2闪烁
*/
unsigned char G1AnimationFont[4][5] = {
    {0x00, 0x08, 0x02, 0x02, 0x02}, // S1 S3 S10
    {0x00, 0x04, 0x02, 0x00, 0x04}, // S1 S2 S5 S8
    {0x00, 0x00, 0x06, 0x06, 0x00}, // S1 S6 S7
    {0x0c, 0x00, 0x02, 0x00, 0x00}, // S1 S2 S4 S9
};
unsigned char G1AnimationTemp[2][5]; //G1显示缓存

//G1以及AD标 的 显示动画函数随机数缓存
unsigned char randomNumNow[5];
unsigned char randomNumLast[5];

//设置数据
char StartEinstellenBuf[10] = {0}; //设置存储

//时区补偿数据
char time_zone_set;

//时间结构体
time_t now;
struct tm timeinfo;

//0显示温湿度  1显示日期
bool THDATE = 0;
char str[6]; //时间日期显示缓存
unsigned char monTemp; //月缓存

//温湿度 电池电压缓存数据
signed short td;
unsigned short hd;
unsigned char bd;
bool recflag = false; //接收到温湿度传感器数据 标志

bool alarm_signal = false; //警报信号

//蜂鸣器任务发生指令缓存
unsigned char bz_dat;

//VFD显示亮度 数据
bool automaticLuminanceFlag = false; //自动调节亮度标志位
unsigned char luminance; //屏的亮度
unsigned char luminanceLevel; //屏的亮度等级

unsigned char ADbuff[13]; //AD寄存器数据缓存

//滚动刷新时间 数据
unsigned char refreshData[160] = {0}; //显示数据
unsigned char dispalyTemp[7][5] = {0}; //时间显示缓存
unsigned char lastTimeTemp[6] = {0}; //上一次刷新的时间缓存
unsigned char nowTimeTemp[6] = {0}; //当前刷新的时间缓存
bool RefreshDirection = 1; //时间滚动动画刷新方向 1向下滚动  0向上滚动
bool RefreshDirectionSetFlag; //时间滚动动画刷新方向 设置标志位

bool dotBlinkFlag = true; //时间显示的：闪烁标志位
unsigned char calibrationTime = TCALIBRATION;

//夜间待机关显示 数据
bool standbyAtNightFlag = false; //进入夜间待机标志位
bool standbyAtNightENFlag = false; //开启夜间待机标志位
unsigned char offHour = STANDBYHOUR; //进入夜间待机时间 小时
unsigned char offMin = STANDBYMIN; //进入夜间待机时间 分钟
unsigned char onHour = OUTSTANDBYHOUR; //退出夜间待机时间 小时
unsigned char onMin = OUTSTANDBYMIN; //退出夜间待机时间 分钟


//进入设置 缓存数据
bool setMenuFlag = 0; //进入设置界面标志位
bool setLightFlag = 0; //亮度设置标志位
bool setNightOfFlag = 0; //夜间关显示设置标志位
bool setTimeZoneFlag = 0; //时区设置标志位
bool blinkAnimationOverturnFlag = 0; //数值设置闪烁翻转标志位
signed char setListNow; //当前显示的设置项目
signed char setListLast; //上一个显示的设置项目
unsigned char blinkAnimationTempBit = 0; //闪烁的设置位
unsigned char setBuffer[6] = {0}; //设置缓存buff
signed char setValueTemp = 0; //设置值 缓存
signed char setBitCnt = 0; //数值设置位切换，用来记录切到哪个显示位

/* 把刷新缓存写入 6个 CGRAM中 */
void Write6CGRAM(unsigned char x)
{
    unsigned char i, j;

    j = x + 40;
    digitalWrite(PIN_NUM_CS, 0); //CS拉低
    usleep(1);
    VFDWriteData(0x40 + 0); //地址寄存器起始位置
    for (i = x; i < j; i++)
    {
        VFDWriteData(refreshData[i]);
    }
    digitalWrite(PIN_NUM_CS, 1); //CS拉高
    usleep(2);
}

/* 显示消失动画 */
void DisappearingAnimation()
{
    unsigned char i;

    for (i = 1; i < 13; i++)
    {
        VFDWriteStrAndShow(i, " ");
        vTaskDelay(50 / portTICK_PERIOD_MS);
    }
}

/* 开机动画 */
void BootAnimation()
{
    unsigned char cnt, i;
    unsigned char G1Temp[5][5] = {
        {0x03, 0x0b, 0x0b, 0x03, 0x03}, //
        {0x03, 0x07, 0x0b, 0x01, 0x05}, //
        {0x03, 0x03, 0x0f, 0x07, 0x01}, //
        {0x0f, 0x03, 0x0b, 0x01, 0x01}, //
        {0x00, 0x00, 0x00, 0x00, 0x00}, //
    };

    i = 0;
    memset(ADbuff, 0xff, 13);
    //不显示点
    ADbuff[6] &= 0xfe;
    ADbuff[7] &= 0xfe;
    ADbuff[8] &= 0xfe;
    ADbuff[9] &= 0xfe;
    ADbuff[10] &= 0xfe;
    ADbuff[11] &= 0xfe;
    VFDWriteAllADRAMAndShow(ADbuff);
    //BootSound();
    for (cnt = 1; cnt < 13; cnt++)
    {
        VFDWriteStrAndShow(cnt, ">");
        WriteCGRAM(6, &G1Temp[i][0]);
        VFDWriteOneDIYCharAndShow(0, 6); //把CGRAM 6的缓存显示到G1上
        i++;
        if (i >= 4) i = 0;
        vTaskDelay(50 / portTICK_PERIOD_MS);
    }
    //DDiDi();

    vTaskDelay(200 / portTICK_PERIOD_MS);
    DisappearingAnimation(); //显示消失动画

    /*
    VFDWriteStrAndShow(0, " ");
    //WriteCGRAM(6,&G1AnimationTemp[0][0],4);//关闭G1显示
    //VFDWriteOneDIYCharAndShow(0,6);//把CGRAM 6的缓存显示到G1上
    vTaskDelay(20 / portTICK_PERIOD_MS);

    //关闭AD显示
    memset(ADbuff,0x00,13);
    for (cnt = 0; cnt < 13; cnt++)
    {
        VFDWriteOneADRAMAndShow(cnt,ADbuff[cnt]);
        vTaskDelay(20 / portTICK_PERIOD_MS);
    }
    */
}

/* 点显示或消失 */
void DotBlink(bool blinkFlag)
{
    if (blinkFlag)
    {
        //显示点
        ADbuff[8] |= 0x01;
        ADbuff[9] |= 0x01;
        ADbuff[10] |= 0x01;
        ADbuff[11] |= 0x01;
    }
    else
    {
        //不显示点
        ADbuff[8] &= 0xfe;
        ADbuff[9] &= 0xfe;
        ADbuff[10] &= 0xfe;
        ADbuff[11] &= 0xfe;
    }
    VFDWriteOneADRAMAndShow(8, ADbuff[8]);
    VFDWriteOneADRAMAndShow(9, ADbuff[9]);
    VFDWriteOneADRAMAndShow(10, ADbuff[10]);
    VFDWriteOneADRAMAndShow(11, ADbuff[11]);
}

/* 点闪烁 */
void DotBlinking()
{
    static bool dotBlink;
    static unsigned char dotBlinkCnt;

    if (dotBlink)
    {
        if (dotBlinkCnt >= 3)
        {
            dotBlinkCnt = 0;
            dotBlink = 0;
            DotBlink(dotBlink);
        }
        dotBlinkCnt++;
    }
    else
    {
        if (dotBlinkCnt >= 1)
        {
            dotBlinkCnt = 0;
            dotBlink = 1;
            DotBlink(dotBlink);
        }
        dotBlinkCnt++;
    }
}

/* 滚动动画0-6帧刷新 */
void FrameRefresh(unsigned char num, unsigned char mCnt1, unsigned char mCnt2, bool directionFlag)
{
    dispalyTemp[num][0] = (numberFont[lastTimeTemp[num]][0] << mCnt1) | (numberFont[nowTimeTemp[num]][0] >> mCnt2);
    dispalyTemp[num][1] = (numberFont[lastTimeTemp[num]][1] << mCnt1) | (numberFont[nowTimeTemp[num]][1] >> mCnt2);
    dispalyTemp[num][2] = (numberFont[lastTimeTemp[num]][2] << mCnt1) | (numberFont[nowTimeTemp[num]][2] >> mCnt2);
    dispalyTemp[num][3] = (numberFont[lastTimeTemp[num]][3] << mCnt1) | (numberFont[nowTimeTemp[num]][3] >> mCnt2);
    dispalyTemp[num][4] = (numberFont[lastTimeTemp[num]][4] << mCnt1) | (numberFont[nowTimeTemp[num]][4] >> mCnt2);

    WriteCGRAM(num, &dispalyTemp[num][0]);
    VFDWriteOneDIYCharAndShow(num + 7, num);
}

/* 滚动动画最后一帧刷新 */
void LastFrameRefresh(unsigned char num)
{
    dispalyTemp[num][0] = numberFont[nowTimeTemp[num]][0];
    dispalyTemp[num][1] = numberFont[nowTimeTemp[num]][1];
    dispalyTemp[num][2] = numberFont[nowTimeTemp[num]][2];
    dispalyTemp[num][3] = numberFont[nowTimeTemp[num]][3];
    dispalyTemp[num][4] = numberFont[nowTimeTemp[num]][4];

    WriteCGRAM(num, &dispalyTemp[num][0]);
    // WriteCGRAM(num, dispalyTemp, num);
    VFDWriteOneDIYCharAndShow(num + 7, num);
}

/* 秒滚动刷新动画 */
void SecDispalyRefresh()
{
    unsigned char ti, ci;

    //滚动动画前6次刷新
    for (ti = 1; ti < 6; ti++)
    {
        ci = 7 - ti;
        if (lastTimeTemp[4] != nowTimeTemp[4]) //秒十位
        {
            FrameRefresh(4, ti, ci, RefreshDirection);
        }
        if (lastTimeTemp[5] != nowTimeTemp[5]) //秒个位
        {
            FrameRefresh(5, ti, ci, RefreshDirection);
        }
        vTaskDelay(50 / portTICK_PERIOD_MS);
    }
    //滚动动画最后一次数据刷新
    if (lastTimeTemp[4] != nowTimeTemp[4]) //秒十位
    {
        LastFrameRefresh(4);
    }
    if (lastTimeTemp[5] != nowTimeTemp[5]) //秒个位
    {
        LastFrameRefresh(5);
    }
}

/* 时间滚动刷新动画 */
void TimeDispalyRefresh()
{
    unsigned char ti, ci, di;

    di = 0;

    //滚动动画前6次刷新
    for (ti = 1; ti < 6; ti++)
    {
        ci = 7 - ti;
        if (lastTimeTemp[0] != nowTimeTemp[0]) //小时十位
        {
            FrameRefresh(0, ti, ci, RefreshDirection);
            di++;
        }
        if (lastTimeTemp[1] != nowTimeTemp[1]) //小时个位
        {
            FrameRefresh(1, ti, ci, RefreshDirection);
            di++;
        }
        if (lastTimeTemp[2] != nowTimeTemp[2]) //分钟十位
        {
            FrameRefresh(2, ti, ci, RefreshDirection);
            di++;
        }
        if (lastTimeTemp[3] != nowTimeTemp[3]) //分钟个位
        {
            FrameRefresh(3, ti, ci, RefreshDirection);
            di++;
        }
        if (lastTimeTemp[4] != nowTimeTemp[4]) //秒十位
        {
            FrameRefresh(4, ti, ci, RefreshDirection);
            di++;
        }
        if (lastTimeTemp[5] != nowTimeTemp[5]) //秒个位
        {
            FrameRefresh(5, ti, ci, RefreshDirection);
            di++;
        }
        di = 50 - di * 2;
        vTaskDelay(di / portTICK_PERIOD_MS);
        di = 0;
    }
    //滚动动画最后一次数据刷新
    if (lastTimeTemp[0] != nowTimeTemp[0]) //小时十位
    {
        LastFrameRefresh(0);
    }
    if (lastTimeTemp[1] != nowTimeTemp[1]) //小时个位
    {
        LastFrameRefresh(1);
    }
    if (lastTimeTemp[2] != nowTimeTemp[2]) //分钟十位
    {
        LastFrameRefresh(2);
    }
    if (lastTimeTemp[3] != nowTimeTemp[3]) //分钟个位
    {
        LastFrameRefresh(3);
    }
    if (lastTimeTemp[4] != nowTimeTemp[4]) //秒十位
    {
        LastFrameRefresh(4);
    }
    if (lastTimeTemp[5] != nowTimeTemp[5]) //秒个位
    {
        LastFrameRefresh(5);
    }
}

/* 把显示数据写入刷新数据 */
void write_frame(unsigned char x, char* str)
{
    unsigned char i, strbuf;

    while (*str)
    {
        strbuf = *str - 0x20;
        for (i = 0; i < 5; i++)
        {
            refreshData[x * 5 + i] = ascii_table_5x7[strbuf][i]; //
        }
        x++;
        str++;
    }
}

/* 刷新时间的显示 */
void RefreshTimeShow(struct tm* time)
{
    unsigned char tempHour, tempMin, tempSec;
    unsigned char i, j;

    tempHour = (unsigned char)time->tm_hour;
    tempMin = (unsigned char)time->tm_min;
    tempSec = (unsigned char)time->tm_sec;

    nowTimeTemp[0] = tempHour / 10 % 10; //小时十位time_zone_hour
    nowTimeTemp[1] = tempHour % 10; //小时个位
    nowTimeTemp[2] = tempMin / 10 % 10; //分钟十位
    nowTimeTemp[3] = tempMin % 10; //分钟个位
    nowTimeTemp[4] = tempSec / 10 % 10; //秒钟十位
    nowTimeTemp[5] = tempSec % 10; //秒钟个位

    //小时第一位为0不显示  numberFont[12]为5*7点阵不显示
    if (nowTimeTemp[0] == 0) nowTimeTemp[0] = 12;

    memcpy(&dispalyTemp[0][0], numberFont[nowTimeTemp[0]], 5);
    memcpy(&dispalyTemp[1][0], numberFont[nowTimeTemp[1]], 5);
    memcpy(&dispalyTemp[2][0], numberFont[nowTimeTemp[2]], 5);
    memcpy(&dispalyTemp[3][0], numberFont[nowTimeTemp[3]], 5);
    memcpy(&dispalyTemp[4][0], numberFont[nowTimeTemp[4]], 5);
    memcpy(&dispalyTemp[5][0], numberFont[nowTimeTemp[5]], 5);

    //把数据写入CGRAM
    digitalWrite(PIN_NUM_CS, 0); //CS拉低
    usleep(1);
    VFDWriteData(0x40 + 0); //地址寄存器起始位置
    for (i = 0; i < 6; i++)
    {
        for (j = 0; j < 5; j++)
        {
            VFDWriteData(dispalyTemp[i][j]);
        }
    }
    digitalWrite(PIN_NUM_CS, 1); //CS拉高
    usleep(2);

    //显示写入CGRAM内的数据
    digitalWrite(PIN_NUM_CS, 0); //CS拉低
    usleep(1);
    VFDWriteData(0x20 + 7); //地址寄存器起始位置
    for (i = 0; i < 6; i++)
    {
        VFDWriteData(i);
    }
    digitalWrite(PIN_NUM_CS, 1); //CS拉高
    usleep(2);
    WriteDisplayCMD(); //显示

    //显示：：
    ADbuff[8] = 0x01;
    ADbuff[9] = 0x01;
    ADbuff[10] = 0x01;
    ADbuff[11] = 0x01;
    VFDWriteOneADRAMAndShow(8, ADbuff[8]);
    VFDWriteOneADRAMAndShow(9, ADbuff[9]);
    VFDWriteOneADRAMAndShow(10, ADbuff[10]);
    VFDWriteOneADRAMAndShow(11, ADbuff[11]);

    lastTimeTemp[0] = nowTimeTemp[0];
    lastTimeTemp[1] = nowTimeTemp[1];
    lastTimeTemp[2] = nowTimeTemp[2];
    lastTimeTemp[3] = nowTimeTemp[3];
    lastTimeTemp[4] = nowTimeTemp[4];
    lastTimeTemp[5] = nowTimeTemp[5];
}

/* G1动画 */
void G1Animation()
{
    static unsigned char tcnt = 0;
    static unsigned char acnt = 0;

    tcnt++;
    if (tcnt >= 2)
    {
        tcnt = 0;
        //把G1动画取模传给G1的显示缓存
        G1AnimationTemp[0][0] = dispalyTemp[6][0] | G1AnimationFont[acnt][0];
        G1AnimationTemp[0][1] = dispalyTemp[6][1] | G1AnimationFont[acnt][1];
        G1AnimationTemp[0][2] = dispalyTemp[6][2] | G1AnimationFont[acnt][2];
        G1AnimationTemp[0][3] = dispalyTemp[6][3] | G1AnimationFont[acnt][3];
        G1AnimationTemp[0][4] = dispalyTemp[6][4] | G1AnimationFont[acnt][4];

        WriteCGRAM(6, &G1AnimationTemp[0][0]); //把G1显示缓存写入CGRAM
        VFDWriteOneDIYCharAndShow(0, 6); //把CGRAM 6的缓存显示到G1上
        acnt++;
        if (acnt >= 4) acnt = 0;
    }
}

/*G1以及AD标 的 显示动画*/
void G1AndADAnimation()
{
    static unsigned char tt = 0;
    static unsigned char at = 0;
    static unsigned char bt = 0;

    /*每8次进入函数 生成5个随机数来确定G1上排几个标 以及AD标的不显示
    * 通过5个来设置灭显示数据    0以及 超过15的随机数舍弃
    */
    tt++;
    if (tt >= 2)
    {
        tt = 0;
        bt++;
        if (bt >= 4)
        {
            bt = 0;
            //显示上一次进入灭显示的  AD标 或者给G1标
            for (; tt < 5; tt++)
            {
                switch (randomNumLast[tt])
                {
                case 1:
                    dispalyTemp[6][1] |= 0x01; //显示SAT标
                    break;
                case 2:
                    dispalyTemp[6][2] |= 0x01; //显示REC标
                    break;
                case 3:
                    ADbuff[1] = 0x02; //显示TIME标
                    VFDWriteOneADRAMAndShow(1, ADbuff[1]);
                    break;
                case 4:
                    ADbuff[2] = 0x02; //显示SHIFT标 以下以此类推
                    VFDWriteOneADRAMAndShow(2, ADbuff[2]);
                    break;
                case 5:
                    ADbuff[3] = 0x02;
                    VFDWriteOneADRAMAndShow(2, ADbuff[2]);
                    break;
                case 6:
                    ADbuff[4] = 0x02;
                    VFDWriteOneADRAMAndShow(4, ADbuff[4]);
                    break;
                case 7:
                    ADbuff[5] = 0x02;
                    VFDWriteOneADRAMAndShow(5, ADbuff[5]);
                    break;
                case 8:
                    ADbuff[6] = 0x02;
                    VFDWriteOneADRAMAndShow(6, ADbuff[6]);
                    break;
                case 9:
                    ADbuff[7] = 0x02;
                    VFDWriteOneADRAMAndShow(7, ADbuff[7]);
                    break;
                case 10:
                    ADbuff[8] |= 0x02;
                    VFDWriteOneADRAMAndShow(8, ADbuff[8]);
                    break;
                case 11:
                    ADbuff[9] |= 0x02;
                    VFDWriteOneADRAMAndShow(9, ADbuff[9]);
                    break;
                case 12:
                    ADbuff[10] |= 0x02;
                    VFDWriteOneADRAMAndShow(10, ADbuff[10]);
                    break;
                case 13:
                    ADbuff[11] |= 0x02;
                    VFDWriteOneADRAMAndShow(11, ADbuff[11]);
                    break;
                case 14:
                    ADbuff[12] = 0x02;
                    VFDWriteOneADRAMAndShow(12, ADbuff[12]);
                    break;
                case 15:
                    dispalyTemp[6][2] |= 0x08;
                    break;
                default:
                    break;
                }
            }
            tt = 0;

            //生成3个随机数 0-19
            randomNumNow[0] = rand() % 20;
            randomNumNow[1] = rand() % 20;
            randomNumNow[2] = rand() % 20;
            randomNumNow[3] = rand() % 20;
            randomNumNow[4] = rand() % 20;

            //灭显示AD标 或者给G1显示缓存赋值
            for (; tt < 5; tt++)
            {
                switch (randomNumNow[tt])
                {
                case 1:
                    dispalyTemp[6][1] &= 0xfe; //灭显示SAT标
                    break;
                case 2:
                    dispalyTemp[6][2] &= 0xfe; //灭显示REC标
                    break;
                case 3:
                    ADbuff[1] = 0x00; //灭显示TIME标
                    VFDWriteOneADRAMAndShow(1, ADbuff[1]);
                    break;
                case 4:
                    ADbuff[2] = 0x00; //灭显示SHIFT标 以下以此类推
                    VFDWriteOneADRAMAndShow(2, ADbuff[2]);
                    break;
                case 5:
                    ADbuff[3] = 0x00;
                    VFDWriteOneADRAMAndShow(2, ADbuff[2]);
                    break;
                case 6:
                    ADbuff[4] = 0x00;
                    VFDWriteOneADRAMAndShow(4, ADbuff[4]);
                    break;
                case 7:
                    ADbuff[5] = 0x00;
                    VFDWriteOneADRAMAndShow(5, ADbuff[5]);
                    break;
                case 8:
                    ADbuff[6] = 0x00;
                    VFDWriteOneADRAMAndShow(6, ADbuff[6]);
                    break;
                case 9:
                    ADbuff[7] = 0x00;
                    VFDWriteOneADRAMAndShow(7, ADbuff[7]);
                    break;
                case 10:
                    ADbuff[8] &= 0xfd;
                    VFDWriteOneADRAMAndShow(8, ADbuff[8]);
                    break;
                case 11:
                    ADbuff[9] &= 0xfd;
                    VFDWriteOneADRAMAndShow(9, ADbuff[9]);
                    break;
                case 12:
                    ADbuff[10] &= 0xfd;
                    VFDWriteOneADRAMAndShow(10, ADbuff[10]);
                    break;
                case 13:
                    ADbuff[11] &= 0xfd;
                    VFDWriteOneADRAMAndShow(11, ADbuff[11]);
                    break;
                case 14:
                    ADbuff[12] = 0x00;
                    VFDWriteOneADRAMAndShow(12, ADbuff[12]);
                    break;
                case 15:
                    dispalyTemp[6][2] &= 0xf7;
                    break;
                default:
                    break;
                }
            }
            tt = 0;

            //把这次显示的传给  下次进来需要灭显示
            randomNumLast[0] = randomNumNow[0];
            randomNumLast[1] = randomNumNow[1];
            randomNumLast[2] = randomNumNow[2];
            randomNumLast[3] = randomNumNow[3];
            randomNumLast[4] = randomNumNow[4];
        }

        //把G1动画取模传给G1的显示缓存
        G1AnimationTemp[0][0] = dispalyTemp[6][0] | G1AnimationFont[at][0];
        G1AnimationTemp[0][1] = dispalyTemp[6][1] | G1AnimationFont[at][1];
        G1AnimationTemp[0][2] = dispalyTemp[6][2] | G1AnimationFont[at][2];
        G1AnimationTemp[0][3] = dispalyTemp[6][3] | G1AnimationFont[at][3];
        G1AnimationTemp[0][4] = dispalyTemp[6][4] | G1AnimationFont[at][4];

        WriteCGRAM(6, &G1AnimationTemp[0][0]); //把G1显示缓存写入CGRAM
        VFDWriteOneDIYCharAndShow(0, 6); //把CGRAM 6的缓存显示到G1上
        at++;
        if (at >= 4) at = 0;
    }
}

/* 显示温湿度数据 或者 日期*/
void THDATEdisplay()
{
    static unsigned char tcnt = 0;
    static bool disflag = false;
    double distemp;

    if (THDATE) //温湿度显示
    {
        if (recflag)
        {
            tcnt++;
            if (tcnt >= 10)
            {
                tcnt = 0;

                VFDWriteStrAndShow(1, "      ");
                if (disflag)
                {
                    disflag = false;
                    distemp = td;
                    distemp = distemp / 100;
                    sprintf(str, "%.1fC", distemp);
                    VFDWriteStrAndShow(1, str);
                }
                else
                {
                    disflag = true;
                    distemp = hd;
                    distemp = distemp / 100;
                    sprintf(str, "%.1f%%", distemp);
                    VFDWriteStrAndShow(1, str);
                }
            }
        }
        else VFDWriteStrAndShow(1, "Scan  "); //未扫描到广播数据时显示 扫描
    }
    else //日期显示
    {
        //分钟为0时刷新日期显示
        if (timeinfo.tm_min == 0)
        {
            monTemp = timeinfo.tm_mon + 1; //0代表1月 所以月+1
            sprintf(str, "%d/%d", monTemp, timeinfo.tm_mday);
            VFDWriteStrAndShow(1, str);
        }
    }
}

/* 时区设置 */
void timeZone_set(signed char tz)
{
    switch (tz)
    {
    case 0: setenv("TZ", "CST-0:00", 1); //0时区
        break;
    case 1: setenv("TZ", "CST-1:00", 1); //UTC +1时区
        break;
    case 2: setenv("TZ", "CST-2:00", 1); //UTC +2时区
        break;
    case 3: setenv("TZ", "CST-3:00", 1); //UTC +3时区
        break;
    case 4: setenv("TZ", "CST-3:30", 1); //UTC +3.5时区
        break;
    case 5: setenv("TZ", "CST-4:00", 1); //UTC +4时区
        break;
    case 6: setenv("TZ", "CST-5:00", 1); //UTC +5时区
        break;
    case 7: setenv("TZ", "CST-5:30", 1); //UTC +5.5时区
        break;
    case 8: setenv("TZ", "CST-6:00", 1); //UTC +6时区
        break;
    case 9: setenv("TZ", "CST-7:00", 1); //UTC +7时区
        break;
    case 10: setenv("TZ", "CST-8:00", 1); //UTC +8时区
        break;
    case 11: setenv("TZ", "CST-9:00", 1); //UTC +9时区
        break;
    case 12: setenv("TZ", "CST-10:00", 1); //UTC +10时区
        break;
    case 13: setenv("TZ", "CST-11:00", 1); //UTC +11时区
        break;
    case 14: setenv("TZ", "CST-12:00", 1); //UTC +12时区
        break;
    case 15: setenv("TZ", "CST+1:00", 1); //UTC -1时区
        break;
    case 16: setenv("TZ", "CST+2:00", 1); //UTC -2时区
        break;
    case 17: setenv("TZ", "CST+3:00", 1); //UTC -3时区
        break;
    case 18: setenv("TZ", "CST+4:00", 1); //UTC -4时区
        break;
    case 19: setenv("TZ", "CST+5:00", 1); //UTC -5时区
        break;
    case 20: setenv("TZ", "CST+6:00", 1); //UTC -6时区
        break;
    case 21: setenv("TZ", "CST+7:00", 1); //UTC -7时区
        break;
    case 22: setenv("TZ", "CST+8:00", 1); //UTC -8时区
        break;
    case 23: setenv("TZ", "CST+9:00", 1); //UTC -9时区
        break;
    case 24: setenv("TZ", "CST+10:00", 1); //UTC -10时区
        break;
    case 25: setenv("TZ", "CST+11:00", 1); //UTC -11时区
        break;
    case 26: setenv("TZ", "CST+12:00", 1); //UTC -12时区
        break;
    default:
        break;
    }
    tzset();
}

/* 时区显示 */
void timeZone_set_view(signed char tz)
{
    switch (tz)
    {
    case 0: VFDWriteStrAndShow(0, "UTC/GTM ");
        break;
    case 1: VFDWriteStrAndShow(0, "UTC +1  ");
        break;
    case 2: VFDWriteStrAndShow(0, "UTC +2  ");
        break;
    case 3: VFDWriteStrAndShow(0, "UTC +3  ");
        break;
    case 4: VFDWriteStrAndShow(0, "UTC +3.5");
        break;
    case 5: VFDWriteStrAndShow(0, "UTC +4  ");
        break;
    case 6: VFDWriteStrAndShow(0, "UTC +5  ");
        break;
    case 7: VFDWriteStrAndShow(0, "UTC +5.5");
        break;
    case 8: VFDWriteStrAndShow(0, "UTC +6  ");
        break;
    case 9: VFDWriteStrAndShow(0, "UTC +7  ");
        break;
    case 10: VFDWriteStrAndShow(0, "UTC +8  ");
        break;
    case 11: VFDWriteStrAndShow(0, "UTC +9  ");
        break;
    case 12: VFDWriteStrAndShow(0, "UTC +10 ");
        break;
    case 13: VFDWriteStrAndShow(0, "UTC +11 ");
        break;
    case 14: VFDWriteStrAndShow(0, "UTC +12 ");
        break;
    case 15: VFDWriteStrAndShow(0, "UTC -1  ");
        break;
    case 16: VFDWriteStrAndShow(0, "UTC -2  ");
        break;
    case 17: VFDWriteStrAndShow(0, "UTC -3  ");
        break;
    case 18: VFDWriteStrAndShow(0, "UTC -4  ");
        break;
    case 19: VFDWriteStrAndShow(0, "UTC -5  ");
        break;
    case 20: VFDWriteStrAndShow(0, "UTC -6  ");
        break;
    case 21: VFDWriteStrAndShow(0, "UTC -7  ");
        break;
    case 22: VFDWriteStrAndShow(0, "UTC -8  ");
        break;
    case 23: VFDWriteStrAndShow(0, "UTC -9  ");
        break;
    case 24: VFDWriteStrAndShow(0, "UTC -10 ");
        break;
    case 25: VFDWriteStrAndShow(0, "UTC -11 ");
        break;
    case 26: VFDWriteStrAndShow(0, "UTC -12 ");
        break;
    default:
        break;
    }
}

/* 开机设置 */
void StartEinstellen()
{
    memset(ADbuff, 0x00, 13); //初始化AD显示缓存
    memset(randomNumNow, 0x00, 5); //初始化G1以及AD标 的 显示动画函数随机数缓存
    memset(randomNumLast, 0x00, 5); //初始化G1以及AD标 的 显示动画函数随机数缓存

    //判断是否为第一次启动  否 则写入初始设置
    if (fs_StartEinstellen_information_read(StartEinstellenBuf, 10) != ESP_OK)
    {
        StartEinstellenBuf[0] = FS_StartEinstellen_INFO_SAVE; //写入开机设置标志位
        StartEinstellenBuf[1] = UTC8; //时区设置 默认UTC+8
        StartEinstellenBuf[2] = 7; //亮度设置值 1-6分级亮度  7自动亮度
        StartEinstellenBuf[3] = 0; //夜间关显示状态  0关 1开
        StartEinstellenBuf[4] = 0; //1温湿度显示   0日期显示
        StartEinstellenBuf[5] = 0; //未使用
        StartEinstellenBuf[6] = 0; //未使用
        StartEinstellenBuf[7] = 0; //未使用
        fs_StartEinstellen_information_write(StartEinstellenBuf, 10); //写入设置
    }

    //时区设置
    time_zone_set = StartEinstellenBuf[1];
    timeZone_set(time_zone_set);

    //判断是否开启夜间休眠
    if (StartEinstellenBuf[3] == 1)
    {
        //点亮夜间待机标志
        dispalyTemp[6][0] |= 0x01; //开关图标
        standbyAtNightENFlag = true;
    }
    else standbyAtNightENFlag = false;

    //判断显示日期 还是 温湿度
    if (StartEinstellenBuf[4] == 1) THDATE = true;
    else THDATE = false;

    //初始化设置亮度开启VFD
    luminanceLevel = StartEinstellenBuf[2];
    if (luminanceLevel == 7) //检测是否开启自动亮度调节
    {
        luminance = measure_brightness();
        VFDInit(luminance);

        //点亮自动亮度标志
        dispalyTemp[6][0] |= 0x02; //小电视图标
        automaticLuminanceFlag = true;
    }
    else VFDInit(luminanceLevel * 40); //初始化设置VFD亮度
}

/* 校准时间 */
void time_calibration()
{
    DisappearingAnimation(); //显示消失动画
    VFDWriteStrAndShow(0, "Get time");
    wifi_sta_start(); //开启sta
    http_time_get(); //获取时间
    //时间显示
    time(&now); //获取时间戳
    localtime_r(&now, &timeinfo); //时间戳转成时间结构体
    RefreshTimeShow(&timeinfo);
}

/* 判断是否警报 */
void ifAlarm()
{
    //温度高于60℃报警
    if (td >= 6000)
    {
        if (!alarm_signal) //警报响起
        {
            alarm_signal = true;
            bz_dat = CONDI;
            xQueueSend(bz_queue, &bz_dat, 0); //给蜂鸣器任务
        }
    }
    else
    {
        if (alarm_signal) //解除警报
        {
            alarm_signal = false;
            bz_dat = EXITDI;
            xQueueSend(bz_queue, &bz_dat, 0); //给蜂鸣器任务
        }
    }

    //湿度高于95%报警
    if (hd >= 9500)
    {
        if (!alarm_signal) //警报响起
        {
            bz_dat = DIDI;
            xQueueSend(bz_queue, &bz_dat, 0); //给蜂鸣器任务
        }
    }
}

/* 按键1动作  亮度设置 */
void key1_Action()
{
    bz_dat = DI;
    xQueueSend(bz_queue, &bz_dat, 0); //给蜂鸣器任务

    luminanceLevel++;
    if (luminanceLevel > 9) luminanceLevel = 1;

    StartEinstellenBuf[2] = luminanceLevel;
    fs_StartEinstellen_information_write(StartEinstellenBuf, 10); //写入设置

    //设置亮度
    if (luminanceLevel == 9)
    {
        //点亮自动亮度标志
        dispalyTemp[6][0] |= 0x02; //小电视图标
        automaticLuminanceFlag = true;
    }
    else
    {
        automaticLuminanceFlag = false;
        //灭 自动亮度标志
        dispalyTemp[6][0] &= 0xfd; //小电视图标
        SetLuminance(luminanceLevel * 30); // More gradual steps: 30, 60, 90, 120, 150, 180, 210, 240
    }
}

/* 按键2动作  夜间待机 开 关 设置*/
void key2_Action()
{
    bz_dat = DI;
    xQueueSend(bz_queue, &bz_dat, 0); //给蜂鸣器任务

    if (standbyAtNightENFlag)
    {
        StartEinstellenBuf[3] = 0;
        standbyAtNightENFlag = false;

        //灭夜间待机标志
        dispalyTemp[6][0] &= 0xfe; //开关图标
    }
    else
    {
        StartEinstellenBuf[3] = 1;
        standbyAtNightENFlag = true;

        //点亮夜间待机标志
        dispalyTemp[6][0] |= 0x01; //开关图标
    }

    fs_StartEinstellen_information_write(StartEinstellenBuf, 10); //写入设置
}

/* 按键3动作 切换温湿度日期的显示/解除报警 */
void key3_Action()
{
    if (alarm_signal)
    {
        bz_dat = EXITDI;
        xQueueSend(bz_queue, &bz_dat, 0); //给蜂鸣器任务
        alarm_signal = false;
    }
    else
    {
        bz_dat = DI;
        xQueueSend(bz_queue, &bz_dat, 0); //给蜂鸣器任务

        if (THDATE)
        {
            VFDWriteStrAndShow(1, "      ");
            //显示日期 0代表1月 所以月+1
            monTemp = timeinfo.tm_mon + 1;
            sprintf(str, "%d/%d", monTemp, timeinfo.tm_mday);
            VFDWriteStrAndShow(1, str);
            StartEinstellenBuf[4] = 0;
            THDATE = false;
        }
        else
        {
            StartEinstellenBuf[4] = 1;
            recflag = false;
            THDATE = true;
        }

        fs_StartEinstellen_information_write(StartEinstellenBuf, 10); //写入设置
    }
}

void clock_funtion_task(void* parameter)
{
    unsigned char nowSec, lastSec;
    unsigned char adv_dat_rev[5];
    int ret_q;
    EventBits_t k_Event;

    ADCInit(); //ADC初始化
    iic_init(); //IIC总线初始化
    key_init(); //按键初始化
    buzzer_init(); //蜂鸣器初始化
    bz_dat = DDIDI;
    xQueueSend(bz_queue, &bz_dat, 0); //给蜂鸣器任务
    StartEinstellen(); //开机设置
    BootAnimation(); //开机动画

    //把设置的标志位显示
    WriteCGRAM(6, &dispalyTemp[6][0]); //把G1显示缓存写入CGRAM
    VFDWriteOneDIYCharAndShow(0, 6); //把CGRAM 6的缓存显示到G1上


    //创建按键事件消息队列
    KeyEventHandle = xEventGroupCreate();
    if (KeyEventHandle == NULL)
        ESP_LOGE(TAG, "Key event create fail!");

    if (!standbyAtNightFlag) VFDWriteStrAndShow(1, "connect WIFI"); //夜间关显示待机下不显示连接wifi
    wifi_sta_start(); //开启wifi连接路由器
    http_time_get(); //获取时间并校准时间
    DisappearingAnimation(); //显示消失动画

    //时间显示
    time(&now); //获取时间戳
    localtime_r(&now, &timeinfo); //时间戳转成时间结构体
    if (!standbyAtNightFlag) RefreshTimeShow(&timeinfo); //夜间关显示待机下不刷新时间显示
    lastSec = timeinfo.tm_sec;

    if (!THDATE)
    {
        //显示日期 0代表1月 所以月+1
        monTemp = timeinfo.tm_mon + 1;
        sprintf(str, "%d/%d", monTemp, timeinfo.tm_mday);
        VFDWriteStrAndShow(1, str);
    }
    while (1)
    {
        //按键 事件处理
        k_Event = xEventGroupWaitBits(KeyEventHandle, 0x0f,pdTRUE,pdFALSE, 10 / portTICK_PERIOD_MS);
        if (k_Event & key1_event) key1_Action();
        else if (k_Event & key2_event) key2_Action();
        else if (k_Event & key3_event) key3_Action();

        //读取 AHT10 传感器数据
        float temperature, humidity;
        if (aht10_read(&temperature, &humidity))
        {
            // 转换为原有格式, 温度和湿度都放大100倍以保持精度
            td = (signed short)(temperature * 100);
            hd = (unsigned short)(humidity * 100);

            ifAlarm();
            recflag = true;
        }
        else
        {
            recflag = false;
        }

        //时间显示 : : 闪烁
        if ((!standbyAtNightFlag) & dotBlinkFlag)
        {
            DotBlinking(); //:闪烁
        }

        //G1 以及 AD动画显示
        if (!standbyAtNightFlag) G1AndADAnimation();

        //自动调节亮度  夜间关显示待机下不刷新
        if (automaticLuminanceFlag & (!standbyAtNightFlag))
        {
            luminance = measure_brightness();
            SetLuminance(luminance);
        }

        //判断进入时间显示 还是 夜间待机
        if (!standbyAtNightFlag)
        {
            time(&now); //获取时间戳
            localtime_r(&now, &timeinfo); //时间戳转成时间结构体
            nowSec = timeinfo.tm_sec;
            if (lastSec != nowSec)
            {
                if (nowSec == 0x00) //秒为00时刷新全部时间显示
                {
                    time(&now); //获取时间戳
                    localtime_r(&now, &timeinfo); //时间戳转成时间结构体
                    nowTimeTemp[0] = timeinfo.tm_hour / 10 % 10;
                    nowTimeTemp[1] = timeinfo.tm_hour % 10;
                    nowTimeTemp[2] = timeinfo.tm_min / 10 % 10;
                    nowTimeTemp[3] = timeinfo.tm_min % 10;
                    nowTimeTemp[4] = timeinfo.tm_sec / 10 % 10;
                    nowTimeTemp[5] = timeinfo.tm_sec % 10;

                    //小时第一位为0不显示  numberFont[12]为5*7点阵不显示
                    if (nowTimeTemp[0] == 0) nowTimeTemp[0] = 12;
                    TimeDispalyRefresh(); //滚动刷新动画

                    //把刷新的时间传给lastTimeTemp
                    lastTimeTemp[0] = nowTimeTemp[0];
                    lastTimeTemp[1] = nowTimeTemp[1];
                    lastTimeTemp[2] = nowTimeTemp[2];
                    lastTimeTemp[3] = nowTimeTemp[3];
                    lastTimeTemp[4] = nowTimeTemp[4];
                    lastTimeTemp[5] = nowTimeTemp[5];

                    //检测是否 开启夜间关闭显示待机
                    if (standbyAtNightENFlag)
                    {
                        if ((timeinfo.tm_hour == offHour) && (timeinfo.tm_min == offMin)) //检测到规定时间关闭显示
                        {
                            //进入夜间待机动画
                            DisappearingAnimation();
                            VFDWriteStrAndShow(2, "good night");
                            vTaskDelay(2000 / portTICK_PERIOD_MS);
                            DisappearingAnimation();
                            //VFDDisplaySwitch(0);//关闭VFD显示
                            VFDDStandby(1); //VFD屏进入待机
                            VFDPowerCtrl(0); //关闭VFD电源

                            ESP_LOGI(TAG, "get into standby at night.");
                            standbyAtNightFlag = true; //夜间关显示待机标志置为真 接下来循环进入夜间待机循环
                        }
                    }

                    //检测是否到时间 联网校时
                    if (timeinfo.tm_sec == calibrationTime)
                    {
                        http_time_get(); //获取时间并校准时间
                        time(&now); //获取时间戳
                        localtime_r(&now, &timeinfo); //时间戳转成时间结构体
                        RefreshTimeShow(&timeinfo); //刷新时间显示
                    }
                }
                else //刷新 秒 显示
                {
                    nowTimeTemp[4] = nowSec / 10 % 10;
                    nowTimeTemp[5] = nowSec % 10;
                    SecDispalyRefresh(); //滚动刷新秒动画
                    lastTimeTemp[4] = nowTimeTemp[4];
                    lastTimeTemp[5] = nowTimeTemp[5];
                }
                lastSec = nowSec;
            }
        }
        else
        {
            time(&now); //获取时间戳
            localtime_r(&now, &timeinfo); //时间戳转成时间结构体
            nowSec = timeinfo.tm_sec;
            if (lastSec != nowSec)
            {
                if (nowSec == 0x00) //秒为00时刷新全部时间显示
                {
                    //检测到规定时间打开显示并校时
                    if ((timeinfo.tm_hour == onHour) && (timeinfo.tm_min == onMin))
                    {
                        VFDPowerCtrl(1); //打开VFD电源
                        VFDDStandby(0); //VFD屏退出待机

                        ESP_LOGI(TAG, "exit standby at night.");
                        http_time_get(); //获取时间并校准时间
                        time(&now); //获取时间戳
                        localtime_r(&now, &timeinfo); //时间戳转成时间结构体
                        RefreshTimeShow(&timeinfo); //刷新时间显示

                        standbyAtNightFlag = false; //退出夜间待机
                    }
                }
                lastSec = nowSec;
            }
            //温湿度报警
        }

        //显示温湿度 或者 日期 夜间关显示待机下不刷新
        if (!standbyAtNightFlag) THDATEdisplay();

        vTaskDelay(200 / portTICK_PERIOD_MS);
    }
}
