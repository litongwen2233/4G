#include"EG800AK.h"

static _4G_Data_Send platformSendBuffFunction = NULL;
//指向发送缓冲区的指针
static uint8_t *platformSendBuff = NULL;
//指向接收缓冲区的指针
static uint8_t *platformReadBuff = NULL;
//指向当前模块状态
static _4G_EG800AK_Status EG800AK_Status = EG800AK_Connent;
//所有指令的最后的截止标志
static const char * const end_command = "\r\n";
//标记接收到恢复数据的标志位
volatile uint8_t EG800AK_Read_Flag = 0;
//标记数据发送的标志位
volatile uint8_t EG800AK_Write_Flag = 1;
//接收中断中的接收数据长度
volatile uint16_t EG800AK_Read_rxLen = 0;
//用于将中断缓冲区中的数据转移出来
static uint8_t responseBuff[128] = {0};
// 发送一个AT指令, 响应的所有数据的长度
volatile static uint16_t responseLen = 0;  
//PDP上下文配置
static _4G_EG800AK_PDP_t PDP_data;   
//AT指令对应表
// static const _4G_AT_CmdEntry_t g_atCmdTable[] = {
//     {"CGPADDR",    AT_TYPE_CGPADDR},
//     {"CGSN",       AT_TYPE_CGSN},
//     {"CIMI",       AT_TYPE_CIMI},
//     {"CSQ",        AT_TYPE_CSQ},
//     // 添加更多指令...
// };
//用于接收不同指令的响应参数
// static char CPIN_ANSWER_CODE[12] = {0};
// static _4G_EG800AK_ANSWER_CREG CREGG_ANSWER_type;
// static uint16_t EG800SK_buad_config(uint16_t buad)
// {
//     const char * const buad_command = "AT+IPR=";
//     uint16_t len =  sprintf((char *)platformSendBuff,"%s%d%s",buad_command,buad,end_command);
//     platformSendBuffFunction(len);
// }
/**
 * @brief 拼接出来一个完整的指令
 * 
 * @param string 
 * @return char* 
 */
static uint16_t String_Joint(const char *string)
{
    return sprintf(platformSendBuff,"%s%s",string,end_command);
}
/**
 * @brief 4G模块发送处理函数
 * 
 * @param drvice_read_status 
 * @return uint8_t 
 */
uint8_t EG800SK_Config_Status(void)
{
    uint16_t len = 0;
    static uint8_t TimeOut = 3; 
    const char * AT_Command[] = {"AT","ATE0","AT+CPIN?","AT+CREG?","AT+CGREG?","AT+CSQ?","AT+CGATT=1","AT+CGATT?"};
    if((EG800AK_Write_Flag == 0) && (TimeOut--))
        return 0;
    EG800AK_Write_Flag = 0;
    TimeOut = 3;
    switch (EG800AK_Status)
    {
        case EG800AK_Connent:
        case EG800AK_ATE0:
        case EG800AK_PIN_CHECK:
        case EG800AK_SIM_CHECK_C:
        case EG800AK_SIM_CHECK_P:
        case EG800AK_SIGNAL_CHECK:
        case EG800AK_CGATT:
        case EG800AK_CGATT_CHECK:
        {
            len = String_Joint(AT_Command[EG800AK_Status]);
            break;
        }
        case EG800AK_PDP_CHECK:
        {
            const char *type = PDP_data.pdp_type ? PDP_data.pdp_type : "IP";
            len= sprintf(platformSendBuff, "AT+CGDCONT=%u,\"%s\",\"%s\"%s",
                     (unsigned)PDP_data.cid, type, PDP_data.apn,end_command);
            break;
        }
        case EG800AK_PDP_ACTIVATE:
        {
            len = sprintf(platformSendBuff, "AT+CGACT=1,%u%s", (unsigned)PDP_data.cid,end_command);
            break;
        }
        case CG800AK_PDP_ACTIVATE_CHECK:
        {
            len = String_Joint("AT+CGACT?");
            break;
        }
        case EG800AK_ERROR:
        {
            // len = String_Joint("AT+CPIN?");
            return 0;
            break;
        }
        default:
            break;
    }
    platformSendBuffFunction(len);
    return 1;
}
/**
 * @brief 4G模块初始化函数
 * 
 * @param _4G_date 
 * @return uint8_t 
 */
uint8_t EG800SK_Init(_4G_EG800AK_Handle *_4G_date)
{
    if(_4G_date->platfromSendbuffFunction == NULL)
        return 0;
    platformSendBuffFunction = _4G_date->platfromSendbuffFunction;
    platformSendBuff         = _4G_date->platformSendBuff;
    platformReadBuff         = _4G_date->platformReadBuff;
    _4G_date->Status         = &EG800AK_Status;
    PDP_data.cid = 1;
    PDP_data.apn = "CMNET";          /* 按运营商修改：移动 CMNET / 联通 UNINET / 电信 CTNET */
    PDP_data.username = "";
    PDP_data.password = "",
    PDP_data.pdp_type = "IP";
    // for(int i=0;i<10;i++)
    // {
    //     EG800SK_Config_Status();
    //     EG800AK_Status++;
    // }
    return 1;
}
static uint8_t EG800SK_CREG_handle(const char *cmd)
{
    char *Plus_addr = NULL;//'+'的位置
    char *colon_addr = NULL;//':'的位置
    int n, stat;
    Plus_addr = strstr((char *)responseBuff, cmd);
    if (!Plus_addr) 
        EG800AK_Status = EG800AK_ERROR;
    colon_addr = Plus_addr + strlen(cmd);
    if (sscanf(colon_addr, ":%d,%d", &n, &stat) == 2) 
    {
        if((stat == 1 || stat == 5))
            return 1;
        else 
            return 0;
    }
}
/**
 * @brief 接收数据解析函数
 * 
 * @return _4G_Read_t 
 */
void EG800SK_Readdata_Handle(uint16_t rxLen)
{
    // if(EG800AK_Read_Flag == 0)
    //     return ;
    // EG800AK_Read_Flag = 0;
    memcpy(&responseBuff[responseLen], platformReadBuff, rxLen);
    responseLen += rxLen;
    rxLen = 0;
    //OK和ERROR都代表着数据接收完整
    if(strstr((char *)responseBuff,"OK") != NULL)
    {
        EG800AK_Write_Flag = 1;
        responseLen = 0 ;
        if(EG800AK_Status == EG800AK_PIN_CHECK)
        {
            if(strstr((char *)responseBuff,"READY") != NULL)
                EG800AK_Status++;
            else
                EG800AK_Status = EG800AK_ERROR;
        }
        else if(EG800AK_Status == EG800AK_SIM_CHECK_C)
        {
            if(EG800SK_CREG_handle("+CREG") == 1)
                EG800AK_Status++;
            else
                EG800AK_Status = EG800AK_ERROR;
        }
        else if(EG800AK_Status == EG800AK_SIM_CHECK_P)
        {
            if(EG800SK_CREG_handle("+CGREG") == 1)
                EG800AK_Status++;
            else
                EG800AK_Status = EG800AK_ERROR;
        }
        else
            EG800AK_Status++;
        return ;
    }
    else if(strstr((char *)responseBuff,"ERROR") != NULL)
    {
        EG800AK_Write_Flag = 1;
        EG800AK_Status = EG800AK_ERROR;
        responseLen = 0 ;
    }
}
/**
 * @brief 4G通讯处理函数
 * 
 * @return uint8_t 
 */
uint8_t EG800SK_Data_Handle(uint16_t rxLen)
{
    EG800SK_Readdata_Handle(rxLen);//接收函数
    EG800SK_Config_Status();//发送函数
    if(EG800AK_Status == CG800AK_PDP_ACTIVATE_CHECK)
        return 1;
    return 0;  
}
void EG800SK_Send(const char *string)
{
    platformSendBuffFunction(String_Joint(string));
}
