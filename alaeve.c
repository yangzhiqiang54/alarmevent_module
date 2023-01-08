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
#include "pthread.h"

/* 宏定义 */
#define ae_malloc   malloc
#define ae_free     free
#define ae_remalloc realloc

#define AE_JUDGE_UNIT_MAXNUM    8
/****end****/


/* 测试数据 */
typedef struct {
    char name[8];
    float val;
}test_data_t;
test_data_t test_data[3][10] = {
    {
        {"Ua", 220},{"Ub", 221},{"Uc", 221},
        {"Ia", 5},{"Ib", 6},{"Ic", 7},
        {"Pa", 0.1},{"Pb", 0.2},{"Pc", 0.3},{"P", 0.6}
    },
    {
        {"Ua", 220},{"Ub", 221},{"Uc", 221},
        {"Ia", 5},{"Ib", 6},{"Ic", 7},
        {"Pa", 0.1},{"Pb", 0.2},{"Pc", 0.3},{"P", 0.6}
    },
    {
        {"Ua", 220},{"Ub", 221},{"Uc", 221},
        {"Ia", 5},{"Ib", 6},{"Ic", 7},
        {"Pa", 0.1},{"Pb", 0.2},{"Pc", 0.3},{"P", 0.6}
    }
};
/****end****/



/* 枚举 */
typedef enum {
    AE_FALSE = 2,
    AE_TRUE = 1,
    AE_OK = 0,
    AE_JSON_ERROR = -1,
    AE_JUDGE_ERROR = -2,
    AE_MALLOC_ERROR = -3,
}ae_eu_result_t;

typedef enum {
    AE_TYPE_NORMAL = 0,
    AE_TYPE_PAIR = 1,
}ae_eu_type_t;
/****end****/

/* typedef */
typedef struct ae_judge_t {
    //struct ae_judge_t * next;
    void * val;
    int (* logic_judg_tfun)(void *);
    uint8_t logic_next; //0-NULL; 1-&&; 2-||
}ae_judge_t;

typedef struct ae_para_t {
    uint32_t period; //判断间隔 秒
    ae_eu_type_t type; //类型 枚举 配置中固定字符串
    char *para_string; //参数字符串指针
    uint8_t para_flag; //报警参数 bit0:开关
    uint8_t trans_id; //转发ID
}ae_para_t;

typedef struct ae_rule_t {
    ae_para_t para; //属性
    // char *src_data; //数据源
    // char *judg; //判断字符串
    ae_judge_t judge_unit[AE_JUDGE_UNIT_MAXNUM]; //判断字符串解析后的源数据和回调判断函数
    char *out_opt; //输出
}ae_rule_t;

typedef struct ae_info_t {
    uint16_t rule_num; //报警规则总数
    uint16_t dev_num; //设备报警总是
}ae_info_t;

typedef struct ae_type_table_t {
    ae_eu_type_t sn;
    char *type_val;
}ae_type_table_t;
/****end****/

/* 变量 */
ae_info_t ae_info; // 模块全局信息记录
ae_rule_t * ae_arr_rule = NULL; //报警事件结构体数组
/****end****/

/* 报警类型与字符串映射表 */
#define AE_TYPE_TABLE_NUM 2
ae_type_table_t ae_type_table[AE_TYPE_TABLE_NUM] = {
    {AE_TYPE_NORMAL,    "normal"},
    {AE_TYPE_PAIR,      "pair"}
};

/* 链表操作 */
/* 创建新头节点 */
// ae_judge_t * ae_list_new_head(void) {
//     ae_judge_t * plist = ae_malloc(sizeof(ae_judge_t));
//     if(plist) {
//         memset(plist, 0, sizeof(ae_judge_t));
//         return plist;
//     }
//     else {
//         return NULL;
//     }
// }

// /* 加入新的后一个节点 */
// ae_judge_t * ae_list_add_tail(ae_judge_t * cur_list) {
//     ae_judge_t * plist = ae_malloc(sizeof(ae_judge_t));
//     if(plist) {
//         memset(plist, 0, sizeof(ae_judge_t));
//         cur_list->next = plist;
//         return plist;
//     }
//     else {
//         return NULL;
//     }
// }
/* end 链表操作 */

/* 字符串拷贝 申请动态内存 */
static char *ae_my_strdup(char *input)
{
    int len = strlen(input);
    char *output = (char *)ae_malloc(len + 1); //加入一个结束符的长度
    if (output == NULL)
        return NULL;
    memset(output, 0, len + 1);
    memcpy(output, input, len);
    return output;
}

/* 将报警类型字符串解析成对应的类型枚举值 */
static ae_eu_type_t ae_type_parse(char * str) {
    for(int i=0; i<AE_TYPE_TABLE_NUM; i++) {
        if(strcmp(str, ae_type_table[i].type_val) == 0) {
            return ae_type_table[i].sn;
        }
    }
    return AE_TYPE_NORMAL; //找不到对应的类型，则返回普通类型
}

/* 逻辑判断函数 */

/* end 逻辑判断函数 */

/* 判断字符是否属于参量名称中的字符 */
static ae_eu_result_t ae_is_vname(char c) {
    if( (c >= '0' && c <= '9') ||
        (c >= 'A' && c <= 'Z') ||
        (c >= 'a' && c <= 'z') ||
        (c == '_') ) {
            return AE_TRUE;
        }
        else {
            return AE_FALSE;
        }
}

/* 
    将判断字符串解析成对应的数据源指针和逻辑判断回调函数，放入判断链表中 
    opt: 报警事件结构体指针
    st_judge: 规则字符串
*/
static ae_eu_result_t ae_parse_rule_to_list(ae_judge_t * junit, char * st_judge) {
    ae_eu_result_t res = AE_OK;
    char vname[12] = {0};
    int jnum = 0, unum = 0, stnum = 0, vnum = 0;

    for(jnum=0; jnum<AE_JUDGE_UNIT_MAXNUM; jnum++) {
        vnum = 0;
        while(ae_is_vname(st_judge[stnum]) == AE_TRUE) {
            vname[vnum++] = st_judge[stnum++];


        }
    }

    











    return res;
}

/* 将配置文件内容解析到内存的ae结构体中 */
static ae_eu_result_t ae_parse_config(cJSON * json_root) {
    ae_eu_result_t res = AE_OK;
    cJSON *rule, *rule_out, *rule_para, *rule_dev, *json_temp, *json_temp_para;
    char tempbuf[64] = {0};
    int rule_dev_obj_num = 0; 
    
    rule = cJSON_GetObjectItem(json_root, "rule");
    rule_out = cJSON_GetObjectItem(json_root, "rule_out");
    rule_dev = cJSON_GetObjectItem(json_root, "rule_dev");
    rule_para = cJSON_GetObjectItem(json_root, "rule_para");
    

    /* 判断JSON格式是否解析成功 */
    if(rule==0 || rule_out==0 || rule_para==0) {
        return AE_JSON_ERROR;
    }

    if( ! ( cJSON_IsArray(rule) && 
            cJSON_IsArray(rule_out) && 
            cJSON_IsArray(rule_dev)) &&
            cJSON_IsArray(rule_para) ) {
        return AE_JSON_ERROR;
    }

    /* 获取报警事件规则总数量 */
    ae_info.rule_num = cJSON_GetArraySize(rule);
    if(ae_info.rule_num !=  cJSON_GetArraySize(rule_out)) {
        res = AE_JSON_ERROR;
        goto ERR_EXIT;
    }
    if(ae_info.rule_num !=  cJSON_GetArraySize(rule_para)) {
        res = AE_JSON_ERROR;
        goto ERR_EXIT;
    }

    /* 获取判断设备总数量 */
    ae_info.dev_num = 0;
    rule_dev_obj_num = cJSON_GetArraySize(rule_dev);
    for(int i=0; i<rule_dev_obj_num; i++) {
        json_temp = cJSON_GetArrayItem(rule_dev, i);
        if(!cJSON_IsArray(json_temp)) {
            res = AE_JSON_ERROR;
            goto ERR_EXIT;
        }
        ae_info.dev_num += cJSON_GetArraySize(json_temp);
    }
    
    /* 给报警事件结构体申请动态内存 */
    ae_arr_rule = ae_malloc(sizeof(ae_rule_t) * ae_info.rule_num);
    memset(ae_arr_rule, 0, (sizeof(ae_rule_t) * ae_info.rule_num));
    if(ae_arr_rule == NULL) {
        res = AE_JSON_ERROR;
        goto ERR_EXIT;
    }

    /* 将JSON格式种的数据解析到结构体中 */
    for(int i=0; i<ae_info.rule_num; i++) {
        //解析判断规则
        json_temp = cJSON_GetArrayItem(rule, i);
        ae_parse_rule_to_list(ae_arr_rule[i].judge_unit, cJSON_GetStringValue(json_temp));

        //解析输出
        json_temp = cJSON_GetArrayItem(rule_out, i);
        ae_arr_rule[i].out_opt = ae_my_strdup(cJSON_GetStringValue(json_temp));

        //解析设备ID
        


        //解析参数
        json_temp = cJSON_GetArrayItem(rule_para, i);
        json_temp_para = cJSON_GetObjectItem(json_temp, "switch");
        if(cJSON_IsTrue(json_temp_para)) ae_arr_rule[i].para.para_flag |= (1<<0);
        else ae_arr_rule[i].para.para_flag &= ~(1<<0);

        json_temp_para = cJSON_GetObjectItem(json_temp, "tid");
        ae_arr_rule[i].para.trans_id = cJSON_GetNumberValue(json_temp_para);

        json_temp_para = cJSON_GetObjectItem(json_temp, "period");
        ae_arr_rule[i].para.period = cJSON_GetNumberValue(json_temp_para);

        json_temp_para = cJSON_GetObjectItem(json_temp, "type");
        ae_arr_rule[i].para.type = ae_type_parse(cJSON_GetStringValue(json_temp_para));

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
        ae_arr_rule[i].para.para_string = ae_my_strdup(tempbuf);
    }

    return AE_OK;

    ERR_EXIT:
    ae_info.rule_num = 0;
    return res;
}



/* 条件判断 */
static ae_eu_result_t ae_get_rule_res(ae_rule_t *prule)
{
    



    return AE_FALSE;
}


/*******/
/* API */
/*******/

/* 报警时间模块初始化 */
int ae_init(void)
{
    memset(&ae_info, 0, sizeof(ae_info_t));
}

/* 模块循环运行线程 */
void ae_loop(void)
{
    static int num_cnt = 0;
    ae_rule_t * p_rule_loop = NULL;
    ae_eu_result_t judg_res = AE_FALSE;

    if(ae_info.rule_num ==0 || ae_arr_rule == NULL) {
        return;
    }

    /* 取下一个判断对象 */
    p_rule_loop = &ae_arr_rule[num_cnt];
    num_cnt += 1;
    if(num_cnt >= ae_info.rule_num) num_cnt = 0;

    /* 判断是否开启 */
    if(p_rule_loop->para.para_flag & 0x01 == 0) {
        return;
    }

    /* 判断结果真假 */
    judg_res = ae_get_rule_res(p_rule_loop);



}

/****end****/

void main(char argc, char *agrv[])
{
    FILE *fp = NULL;
    char *fbuf = NULL;
    int rw = 0, file_size = 0;
    struct stat stat_temp;

    /* 读取文件 */
    stat(".\\RULE.json", &stat_temp);
    file_size = stat_temp.st_size;

    fbuf = ae_malloc(file_size+1);
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

    /* 将JSON数据解析到ae结构体中 */
    ae_eu_result_t res = ae_parse_config(root);
    cJSON_Delete(root);

    while(1) {
        ae_loop();
    }

    EXIT:
    return;
}

