/*
    模块名称：报警、事件逻辑判断处理模块
    输入：字符串、判断规则、数据源、输出对象、输出内容
    输出：判断结果、阈值、当前值
    作者：yzq

    要求：
        忽略字串串中所有的空格；


    版本记录：
        2022-12-17 第一版本
*/

#include "alaeve.h"
#include "cJSON.h"
#include "stdio.h"
#include "string.h"
#include "stdlib.h"
#include "sys/stat.h"

/* 宏定义 */

/****end****/

/* 枚举 */
typedef enum {
    AE_OK = 0,
    AE_JSON_ERROR = -1,
}ae_eu_result_t;

typedef enum {
    AE_TYPE_NORMAL = 0,
    AE_TYPE_PAIA = 1,
}ae_eu_type_t;
/****end****/



/* typedef */
typedef struct ae_para_t {
    uint32_t period; //判断间隔 秒
    ae_eu_type_t type; //类型 枚举 配置中固定字符串
    char *para_string;
    uint8_t para_flag; //报警参数 bit0:开关
}ae_para_t;

typedef struct ae_rule_t {
    ae_para_t para; //属性
    char *src_data; //数据源
    char *judg; //判断
    char *out_opt; //输出
}ae_rule_t;

typedef struct ae_info_t {
    uint16_t num; //报警判断总数
    ae_rule_t * arr_rule; //报警事件结构体数组
}ae_info_t;

typedef struct ae_type_table_t {
    ae_eu_type_t sn;
    char *type_val;
}ae_type_table_t;
/****end****/

/* 变量 */
ae_info_t ae_info; // 模块全局信息记录
/****end****/

/* 报警类型与字符串映射表 */
#define AE_TYPE_TABLE_NUM 2
ae_type_table_t ae_type_table[AE_TYPE_TABLE_NUM] = {
    {AE_TYPE_NORMAL,    "normal"},
    {AE_TYPE_PAIA,      "pair"}
};



/* 字符串拷贝 申请动态内存 */
static char *ae_my_strdup(char *input)
{
    // We need strlen(src) + 1, since we have to account for '\0'
    int len = strlen(input);
    char *output = (char *)malloc(len + 1);
    if (output == NULL)
        return NULL;
    memset(output, 0, len + 1);
    memcpy(output, input, len);
    return output;
}




/* 将字符串解析成对应的类型枚举值 */
static ae_eu_type_t ae_type_parse(char * str) {
    for(int i=0; i<AE_TYPE_TABLE_NUM; i++) {
        if(strcmp(str, ae_type_table[i].type_val) == 0) {
            return ae_type_table[i].sn;
        }
    }
    return AE_TYPE_NORMAL; //找不到对应的类型，则返回普通类型
}

/* 将配置文件内容解析到内存的ae结构体中 */
static ae_eu_result_t ae_parse_config(cJSON * json_root) {
    ae_eu_result_t res = AE_OK;
    cJSON *rule, *rule_out, *rule_para, *json_temp, *json_temp_para;
    char tempbuf[64] = {0};
    
    rule = cJSON_GetObjectItem(json_root, "rule");
    rule_out = cJSON_GetObjectItem(json_root, "rule_out");
    rule_para = cJSON_GetObjectItem(json_root, "rule_para");

    /* 判断JSON格式是否解析成功 */
    if(rule==0 || rule_out==0 || rule_para==0) {
        return AE_JSON_ERROR;
    }

    if( !(cJSON_IsArray(rule) && cJSON_IsArray(rule_out) && cJSON_IsArray(rule_para)) ) {
        return AE_JSON_ERROR;
    }

    /* 获取报警事件配置总数量 */
    ae_info.num = cJSON_GetArraySize(rule);
    if(ae_info.num !=  cJSON_GetArraySize(rule_out)) {
        res = AE_JSON_ERROR;
        goto ERR_EXIT;
    }
    if(ae_info.num !=  cJSON_GetArraySize(rule_para)) {
        res = AE_JSON_ERROR;
        goto ERR_EXIT;
    }
    
    /* 给报警事件结构体申请动态内存 */
    ae_info.arr_rule = malloc(sizeof(ae_rule_t) * ae_info.num);
    memset(ae_info.arr_rule, 0, (sizeof(ae_rule_t) * ae_info.num));
    if(ae_info.arr_rule == NULL) {
        res = AE_JSON_ERROR;
        goto ERR_EXIT;
    }

    /* 将JSON格式种的数据解析到结构体中 */
    for(int i=0; i<ae_info.num; i++) {
        json_temp = cJSON_GetArrayItem(rule, i);
        ae_info.arr_rule[i].judg = ae_my_strdup(cJSON_GetStringValue(json_temp));
        json_temp = cJSON_GetArrayItem(rule_out, i);
        ae_info.arr_rule[i].out_opt = ae_my_strdup(cJSON_GetStringValue(json_temp));

        json_temp = cJSON_GetArrayItem(rule_para, i);
        json_temp_para = cJSON_GetObjectItem(json_temp, "switch");
        if(cJSON_IsTrue(json_temp_para)) ae_info.arr_rule[i].para.para_flag |= (1<<0);
        else ae_info.arr_rule[i].para.para_flag &= ~(1<<0);

        json_temp_para = cJSON_GetObjectItem(json_temp, "period");
        ae_info.arr_rule[i].para.period = cJSON_GetNumberValue(json_temp_para);

        json_temp_para = cJSON_GetObjectItem(json_temp, "type");
        ae_info.arr_rule[i].para.type = ae_type_parse(cJSON_GetStringValue(json_temp_para));

        /* 最多支持p1 p2 p3 */ 
        memset(tempbuf, 0, sizeof(tempbuf));
        json_temp_para = cJSON_GetObjectItem(json_temp, "p1");
        if(json_temp_para != NULL) {
            strcat(tempbuf, cJSON_GetStringValue(json_temp_para));
            strcat(tempbuf, ";");
        }
        json_temp_para = cJSON_GetObjectItem(json_temp, "p2");
        if(json_temp_para != NULL) {
            strcat(tempbuf, cJSON_GetStringValue(json_temp_para));
            strcat(tempbuf, ";");
        }
        json_temp_para = cJSON_GetObjectItem(json_temp, "p3");
        if(json_temp_para != NULL) {
            strcat(tempbuf, cJSON_GetStringValue(json_temp_para));
            strcat(tempbuf, ";");
        }
        ae_info.arr_rule[i].para.para_string = ae_my_strdup(tempbuf);
    }

    return AE_OK;

    ERR_EXIT:
    ae_info.num = 0;
    return res;

    OK_EXIT:
    return res;
}

/*******/
/* API */
/*******/

/* 报警时间模块初始化 */
int ae_init(void)
{
    memset(&ae_info, 0, sizeof(ae_info_t));
}

void main(char argc, char *agrv[])
{
    FILE *fp = NULL;
    char *fbuf = NULL;
    int rw = 0, file_size = 0;
    struct stat stat_temp;

    /* 读取文件 */
    stat(".\\RULE.json", &stat_temp);
    file_size = stat_temp.st_size;

    fbuf = malloc(file_size+1);
    memset(fbuf, 0, file_size+1);

    fp = fopen(".\\RULE.json", "r");
    if(fp == NULL) {
        return;
    }
    rw = fread(fbuf, 1, file_size, fp);
    fclose(fp);
    
    /* 解析JSON数据 */
    cJSON * root = cJSON_Parse(fbuf);
    free(fbuf);
    if(root == NULL) {
        goto EXIT;
    }
    ae_eu_result_t res = ae_parse_config(root);
    cJSON_Delete(root);

    EXIT:
    return;

}

