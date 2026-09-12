#include"EG800AK/EG800AK.h"
#include"EG800AK/MQTT_client.h"

uint8_t Buff[256]   = {0};
uint8_t R_Buff[256] = {0};

_4G_EG800AK_Handle _4G_data;
mqtt_client_t mqtt_data;
char ok[] = "OK";
char Ready[] = "+CPIN:READY\r\nOK";
char CS[] = "+CREG:1,1\r\nOK";
char PS[] = "+CGREG:1,1\r\nOK";
char CSQ[] = "+CSQ:101,1\r\nOK";
char CGATT[] = "+CGATT:1\r\nOK";
char CGDCONT[] = "+CGDCONT:1,\"IP\",,,0,0\r\nOK";

#define PRINTF_TYPE 1   //0 以16进制方式输出 1 以字符串输出

void LCD_Printf(uint16_t length)
{
#if PRINTF_TYPE == 0
    printf("当前内容:");
    for(uint16_t i = 0; i < length; i++)
    {
        printf("%02X ", Buff[i]);  // 大写的16进制，每个字节占2位，空格分隔
    }
    printf("\n");
#elif PRINTF_TYPE == 1
    printf("当前发送内容:%s\n",Buff);
#endif
}
void LCD_scanf(char *buff,uint16_t len)
{
    memcpy(R_Buff,buff,len);
    printf("当前回复内容：%s\n",R_Buff);
    mqtt_Read_Flag(len);
}
int main()
{
    // mqtt_data._4G_date.
    mqtt_data._4G_date.platfromSendbuffFunction = LCD_Printf;
    mqtt_data._4G_date.platformSendBuff = Buff;
    mqtt_data._4G_date.platformReadBuff = R_Buff;
    mqtt_register(&mqtt_data);
    mqtt_Handle();
    LCD_scanf(ok,sizeof(ok));
    mqtt_Handle();
    LCD_scanf(ok,sizeof(ok));
    mqtt_Handle();
    LCD_scanf(Ready,sizeof(Ready));
    mqtt_Handle();
    LCD_scanf(CS,sizeof(CS));
    mqtt_Handle();
    LCD_scanf(PS,sizeof(PS));
    mqtt_Handle();
    LCD_scanf(CSQ,sizeof(CSQ));
    mqtt_Handle();
    LCD_scanf(CGATT,sizeof(CGATT));
    mqtt_Handle();
    LCD_scanf(CGDCONT,sizeof(CGDCONT));
    mqtt_Handle();
    mqtt_Handle();
    mqtt_Handle();
    // while(1);
}