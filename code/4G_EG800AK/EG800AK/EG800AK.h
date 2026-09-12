#ifndef __EG800AK_H__
#define __EG800AK_H__

#include"string.h"
#include"stdio.h"

#define LCD_DRIVER 0 //0 win 1 stm32

#if LCD_DRIVER == 0
#include <stdint.h>
#elif LCD_DRIVER == 1
#include "main.h"
#endif

#define COMMADN_ANSWER_SIZE 5 

typedef void (*_4G_Data_Send)(uint16_t length);

typedef enum
{
    EG800AK_Connent = 0,//测试链接
    EG800AK_ATE0,//关闭回显
    EG800AK_PIN_CHECK,//PIN码检查 确认SIM就绪
    EG800AK_SIM_CHECK_C,//SIM卡检查 验证SIM卡信息完整
    EG800AK_SIM_CHECK_P,
    EG800AK_SIGNAL_CHECK,//信号强度检查，确保有可用信号
    EG800AK_CGATT,//网络吸附
    EG800AK_CGATT_CHECK,//网络吸附检查
    EG800AK_PDP_CHECK,//配置PDP上下文参数
    EG800AK_PDP_ACTIVATE,//激活PDP，激活PDP上下文，获取IP地址
    CG800AK_PDP_ACTIVATE_CHECK,//激活检查
    // EG800AK_NET_CONFIG,//网卡配置
    // EG800AK_READY,//模块初始化完成，可进行数据通讯
    EG800AK_ERROR, //ERROR
    EG800AK_WAIT,//等待回复
}_4G_EG800AK_Status;

/* 4G 通信建立错误码（基于 Quectel LTE Standard(A) AT 命令手册 V1.3）。 */
typedef enum 
{
    FOURG_OK = 0,
    FOURG_ERR_AT,          /* 底层 AT 通信失败 */
    FOURG_ERR_SIM,         /* SIM 卡未就绪 / 需要 PIN */
    FOURG_ERR_REG,         /* 网络注册失败（CS/PS 域） */
    FOURG_ERR_ATTACH,      /* PS 域附着失败 */
    FOURG_ERR_PDP,         /* PDP 上下文定义/激活失败 */
    FOURG_ERR_TIMEOUT,     /* 等待超时 */
    FOURG_ERR_PARAM        /* 参数错误 */
}_4G_EG800AK_PDP_err_t;


// AT指令响应类型枚举
typedef enum {
    AT_TYPE_UNKNOWN = 0,
    AT_TYPE_OK,          // 纯 OK 响应
    AT_TYPE_ERROR,       // 纯 ERROR 响应
    AT_TYPE_CGPADDR,     // +CGPADDR: 响应
    AT_TYPE_CGSN,        // +CGSN: 响应
    AT_TYPE_CIMI,        // +CIMI: 响应
    AT_TYPE_CSQ,         // +CSQ: 响应
    // 可继续扩展...
}_4G_AT_Type_e;

/* CS域网络注册状态响应 */
typedef struct 
{
    uint8_t n;//是否启用网络注册相关URC 0禁用 1启用网络注册 URC +CREG: <stat> 2 启用带有位置信息的网络注册 URC：+CREG: <stat>[,<lac>,<ci>[,<AcT>]]
    uint8_t stat;//0 未注册 1 已注册，归属地网络 2 未注册 ME正在搜索要注册的营运商 3 注册被拒接 4 未知状态 5 已注册 漫游网络 
    char lac[2];//位置编号
    char ci[5];
    uint8_t Act;//网络制式 0 GSM 2 UTRAN 3 GSM W/EGPRS 4 UTRAN W/HSDPA 5 UTRAN W/HSUPA 6 UTRAN W/HSDPA and HSUPA 7 E-UTRAN 8 UTRAN HSPA+
}_4G_EG800AK_ANSWER_CREG;

/* PDP 上下文配置（对应 AT+CGDCONT / AT+CGACT）。 */
typedef struct 
{
    uint8_t  cid;          /* 上下文标识，范围 1~15，MQTT 默认使用 1 */
    const char *apn;       /* 接入点名称，如 "CMNET"/"UNINET"/"3gnet" */
    const char *username;  /* 认证用户名，无则填空 */
    const char *password;  /* 认证密码，无则填空 */
    const char *pdp_type;  /* "IP"(IPv4) / "IPV6" / "IPV4V6" */
}_4G_EG800AK_PDP_t;

// AT指令解析结果结构体
typedef struct 
{
    _4G_AT_Type_e type;              // 指令类型
    char params[32];         // 参数（最多5个，每个最多32字符）
    unsigned char param_cnt;     // 参数数量
    char raw_cmd_name[16];      // 原始的指令名称（如 "CGPADDR"）
}_4G_AT_Result_t;

// 指令识别表结构
typedef struct 
{
    const char *cmd_name;  // 指令名称（如 "CGPADDR"）
    _4G_AT_Type_e type;        // 对应的枚举类型
}_4G_AT_CmdEntry_t;

typedef struct 
{
    _4G_Data_Send platfromSendbuffFunction;
    uint8_t *platformSendBuff;
    uint8_t *platformReadBuff;
    const _4G_EG800AK_Status *Status;//在外部只允许被读取 不允许修改
    _4G_EG800AK_PDP_t PDP_data;
}_4G_EG800AK_Handle;



static inline EG800SK_Read_Flag(uint16_t Rx_len)
{ 
    extern volatile uint8_t  EG800AK_Read_Flag;
    extern volatile uint16_t EG800AK_Read_rxLen;
    EG800AK_Read_Flag = 1; 
    EG800AK_Read_rxLen = Rx_len;
}

uint8_t EG800SK_Init(_4G_EG800AK_Handle *_4G_date);
uint8_t EG800SK_Data_Handle(uint16_t rxLen);
void EG800SK_Send(const char *string);
uint8_t EG800SK_Config_Status(void);

#endif //__EG800AK_H__