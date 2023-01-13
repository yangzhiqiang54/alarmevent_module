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

#define AE_JUDGE_UNIT_MAXNUM    4
/****end****/


/* 测试数据 */
typedef struct {
    char name[8];
    float val;
}test_data_t;
test_data_t test_devdata[3][10] = {
    {
        {"Ua", 225.11},{"Ub", 221},{"Uc", 221},
        {"Ia", 5},{"Ib", 6},{"Ic", 7},
        {"Pa", 0.1},{"Pb", 0.2},{"Pc", 0.3},{"P", 0.6}
    },
    {
        {"Ua", 235.21},{"Ub", 220},{"Uc", 261},
        {"Ia", 5},{"Ib", 6},{"Ic", 7},
        {"Pa", 0.1},{"Pb", 0.2},{"Pc", 0.3},{"P", 0.6}
    },
    {
        {"Ua", 220.31},{"Ub", 221},{"Uc", 221},
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
    AE_NOLOGIC = 0,
    AE_AND,
    AE_OR,
    AE_DIST,
}ae_eu_logic_t;

typedef enum {
    AE_TYPE_NORMAL = 0,
    AE_TYPE_CHANGE,
    AE_TYPE_RUNSASO,
}ae_eu_type_t;
/****end****/

/* typedef */
typedef ae_eu_result_t(* judge_pfun_t)(void *, void *);
typedef struct ae_judge_t {
    float cdtval; //条件值
    judge_pfun_t judge_pfun; //进行各种逻辑判断的函数指针
    ae_eu_logic_t logic_next; //0-NULL  1-&&  2-||  3-;
    char srcname[20]; //规则中字符串名称
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
    ae_judge_t judge_unit[AE_JUDGE_UNIT_MAXNUM]; //判断字符串解析后的源数据和回调判断函数
    char *rule; //判断规则字符串
    char *out_opt; //输出
    uint16_t obj_num;
}ae_rule_t;

typedef struct ae_dev_list_t {
    ae_rule_t * rule;
    uint8_t * savebuf;
    uint8_t dev_id;
    uint8_t flag;
}ae_dev_list_t;

typedef struct ae_info_t {
    uint16_t rule_num; //报警规则总数
    uint16_t dev_num; //报警设备总数
}ae_info_t;

typedef struct ae_type_table_t {
    ae_eu_type_t sn;
    char *type_val;
}ae_type_table_t;

typedef struct ae_judge_table_t {
    char jname[4];
    judge_pfun_t jfun;
}ae_judge_table_t;
/****end****/

/* 变量 */
ae_info_t ae_info; // 模块全局信息记录
ae_rule_t * ae_arr_rule = NULL; //报警事件结构体数组
ae_dev_list_t * ae_dev_list = NULL; //参与报警判断的设备列表
//uint8_t * ae_dev_list = NULL; //设备列表，用来记录报警规则对应了哪些设备
/* end 变量 */

/* 函数声明 */
ae_eu_result_t ae_judge_large(void *v1, void *v2);
ae_eu_result_t ae_judge_large_equal(void *v1, void *v2);
ae_eu_result_t ae_judge_less(void *v1, void *v2);
ae_eu_result_t ae_judge_less_equal(void *v1, void *v2);
ae_eu_result_t ae_judge_equal(void *v1, void *v2);
ae_eu_result_t ae_judge_noequal(void *v1, void *v2);
/* end 函数声明 */

/* 判断符号列表 */
#define AE_JUDGE_ENUMNUM    6
const ae_judge_table_t ae_judge_char_enum[AE_JUDGE_ENUMNUM] = {
    {">", ae_judge_large}, 
    {">=", ae_judge_large_equal}, 
    {"<", ae_judge_less}, 
    {"<=", ae_judge_less_equal}, 
    {"==", ae_judge_equal}, 
    {"!=", ae_judge_noequal}
};

/* 逻辑符号列表 */
#define AE_LOGIC_ENUMNUM    3
const char ae_logic_char_enum[AE_LOGIC_ENUMNUM][4] = {
    "&&", "||", ";"
};


/* 报警类型与字符串映射表 */
#define AE_TYPE_TABLE_NUM 3
const ae_type_table_t ae_type_table[AE_TYPE_TABLE_NUM] = {
    {AE_TYPE_NORMAL,    "NORMAL"},
    {AE_TYPE_CHANGE,    "CHANGE"},
    {AE_TYPE_RUNSASO,   "RUNSASO"}
};

/* 链表操作 */
/* 新建头节点 */
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

// /* 新建一个next节点 */
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

/* 计算子字符串在主字符串中出现的次数 */
static uint8_t ae_get_strnum(char *buf, char *tarbuf) {
    int tarcnt = 0;
    char *p = strstr(buf, tarbuf);
    while(p != NULL) {
        tarcnt++;
        p += strlen(tarbuf);
        p = strstr(p, tarbuf);
    }
    return tarcnt;
}

/* 字符串拷贝 申请动态内存 */
static char *ae_my_strdup(char *input)
{
    int len = strlen(input);
    char *output = ae_malloc(len + 1); //加入一个结束符的长度
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
    取出规则字符串中的参数名称
    src：源字符串指针
    num：参数个数
    out：返回字符串指针
*/
static void ae_get_vname(char * src, uint8_t * pla, char * out) {
    int i = 0, j = 0, flag = 0;
    char tbuf[20] = {0};
    uint8_t len = strlen(src) - (*pla);

    src += *pla;
    for(i=0; i<len; i++) {
        if(ae_is_vname(src[i]) == AE_TRUE) {
            if(!(src[i]>='0' && src[i]<='9')) {
                flag = 1;
                tbuf[j++] = src[i];
            }
            else if(flag == 1) {
                tbuf[j++] = src[i];
            }
        }
        else {
            if(flag == 1) {
                break;
            }
        }
    }
    strcpy(out, tbuf);
    *pla += i;
}

/* 获取判断字符串 */
static void ae_get_jname(char * srcstr, uint8_t * pla, char * tarstr) {
    uint8_t i = *pla, j = 0;
    while( srcstr[i] == '>' ||
           srcstr[i] == '<' ||
           srcstr[i] == '=' ||
           srcstr[i] == '!' ) {
                tarstr[j++] = srcstr[i++];
                (*pla)++;
           }
}

/* 
    定位到源数据指针地址 
    @此函数需要根据外部实时数据结构修改
    dnum: 设备序号，从1开始
    vstr: 参数名称字符串指针
    return: 源数据指针地址
*/
static float * ae_get_val_p(uint8_t dnum, char * vstr) {
    for(int i=0; i<10; i++) {
        if(strcmp(test_devdata[dnum - 1][i].name, vstr) == 0) {
            return &test_devdata[dnum - 1][i].val;
        }
    }   
}


/* 逻辑判断函数 */
/* > 大于 */
ae_eu_result_t ae_judge_large(void *v1, void *v2) {
    int val1 = (int)((*(float *)v1)*1000);
    int val2 = (int)((*(float *)v2)*1000);
    if(val1 > val2) return AE_TRUE;
    else return AE_FALSE;
}

/* >= 大于等于 */
ae_eu_result_t ae_judge_large_equal(void *v1, void *v2) {
    int val1 = (int)((*(float *)v1)*1000);
    int val2 = (int)((*(float *)v2)*1000);
    if(val1 >= val2) return AE_TRUE;
    else return AE_FALSE;
}

/* < 小于 */
ae_eu_result_t ae_judge_less(void *v1, void *v2) {
    int val1 = (int)((*(float *)v1)*1000);
    int val2 = (int)((*(float *)v2)*1000);
    if(val1 < val2) return AE_TRUE;
    else return AE_FALSE;
}

/* <= 小于等于 */
ae_eu_result_t ae_judge_less_equal(void *v1, void *v2) {
    int val1 = (int)((*(float *)v1)*1000);
    int val2 = (int)((*(float *)v2)*1000);
    if(val1 <= val2) return AE_TRUE;
    else return AE_FALSE;
}
/* == 等于 */
ae_eu_result_t ae_judge_equal(void *v1, void *v2) {
    int val1 = (int)((*(float *)v1)*1000);
    int val2 = (int)((*(float *)v2)*1000);
    if(val1 == val2) return AE_TRUE;
    else return AE_FALSE;
}

/* != 不等于 */
ae_eu_result_t ae_judge_noequal(void *v1, void *v2) {
    int val1 = (int)((*(float *)v1)*1000);
    int val2 = (int)((*(float *)v2)*1000);
    if(val1 != val2) return AE_TRUE;
    else return AE_FALSE;
}
/* end 逻辑判断函数 */

/* 通过判断字符串返回对应的判断函数指针 */
static judge_pfun_t ae_get_judge_pfun(char * name) {
    for(int i=0; i<AE_JUDGE_ENUMNUM; i++) {
        if(strcmp(name, ae_judge_char_enum[i].jname) == 0) {
            return ae_judge_char_enum[i].jfun;
        }
    }
    return NULL;
}

/* 通过判断字符串返回与下一级的逻辑关系 */
static ae_eu_logic_t ae_get_logic_next(char * str, uint8_t * pla) {
    uint8_t i = *pla, j = 0;
    char tbuf[4] = {0};

    while( str[i] != '&' && 
           str[i] != '|' && 
           str[i] != ';' && 
           str[i] != '\0')
        {
            i++;
        }
    while( !(str[i] != '&' && 
             str[i] != '|' && 
             str[i] != ';' && 
             str[i] != '\0')) 
        {
            tbuf[j++] = str[i++];
        }
    (*pla) += j;

    if(tbuf[0] == '\0') return AE_NOLOGIC;
    else if(strcmp(tbuf, "&&") == 0) return AE_AND;
    else if(strcmp(tbuf, "||") == 0) return AE_OR;
    else if(strcmp(tbuf, ";") == 0) return AE_DIST;
    else return AE_NOLOGIC;
}

/* 
    将判断字符串解析成对应的数据源指针和逻辑判断回调函数，放入判断链表中 
    junit: 报警事件结构体指针
    st_judge: 规则字符串
*/
static ae_eu_result_t ae_parse_rule_to_list(ae_rule_t * junit, char * st_judge) {
    ae_eu_result_t res = AE_OK;
    char new_judge[64] = {0};
    int i = 0, j = 0;
    int val_cnt = 0;
    uint8_t vname_flag = 0;

    /* 判断是否有判断规则 */
    if(strlen(st_judge) == 0) {
        res = AE_JUDGE_ERROR;
        goto EXIT;
    }

    /* 去掉字符串中的空格 */
    for(i=0,j=0; st_judge[i] != '\0'; i++) {
        if(st_judge[i] != ' ') {
            new_judge[j++] = st_judge[i];
        }
        if(j >= 64) {
            res = AE_JUDGE_ERROR;
            goto EXIT;
        }
    }
    new_judge[j] = '\0';

    /* 通过逻辑符号计算出此报警规则判断总个数 */
    junit->obj_num = 1; //单个条件时，没有逻辑符号
    for(i=0; i<AE_JUDGE_ENUMNUM; i++) {
        junit->obj_num += ae_get_strnum(new_judge, (char *)ae_logic_char_enum[i]);
    }

    /* 解析出数据源、判断逻辑、阈值 */
    char vname[20] = {0}, jdg[4] = {0};
    uint8_t strpla = 0;
    for(j=0; j<junit->obj_num; j++) {
        //获取参数名称
        ae_get_vname(new_judge, &strpla, vname);
        strcpy(junit->judge_unit[j].srcname, vname);

        //获取对应判断规则的函数指针
        memset(jdg, 0, sizeof(jdg));
        ae_get_jname(new_judge, &strpla, jdg);
        junit->judge_unit[j].judge_pfun = ae_get_judge_pfun(jdg);

        //获取阈值
        junit->judge_unit[j].cdtval = atof(new_judge + strpla);

        //获取下一个判断的关联逻辑
        junit->judge_unit[j].logic_next = ae_get_logic_next(new_judge, &strpla);
    }
   
    EXIT:
    return res;
}

/* 将配置文件内容解析到内存的ae结构体中 */
static ae_eu_result_t ae_parse_config(cJSON * json_root) {
    ae_eu_result_t res = AE_OK;
    cJSON *rule, *rule_out, *rule_para, *rule_dev, *json_temp, *json_temp_para;
    int rule_dev_obj_num = 0; 
    
    rule = cJSON_GetObjectItem(json_root, "rule");
    rule_out = cJSON_GetObjectItem(json_root, "rule_out");
    rule_dev = cJSON_GetObjectItem(json_root, "rule_dev");
    rule_para = cJSON_GetObjectItem(json_root, "rule_para");
    

    /* 判断JSON格式是否解析成功 */
    if(rule==0 || rule_out==0 || rule_para==0) {
        res = AE_JSON_ERROR;
        goto ERR_EXIT;
    }

    if( ! ( cJSON_IsArray(rule) && 
            cJSON_IsArray(rule_out) && 
            cJSON_IsArray(rule_dev)) &&
            cJSON_IsArray(rule_para) ) {
        res = AE_JSON_ERROR;
        goto ERR_EXIT;
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
    if(ae_arr_rule == NULL) {
        res = AE_MALLOC_ERROR;
        goto ERR_EXIT;
    }
    memset(ae_arr_rule, 0, (sizeof(ae_rule_t) * ae_info.rule_num));

    /* 给设备列表申请内存,并初始化设备ID和对应的报警规则 */
    ae_dev_list = ae_malloc(ae_info.dev_num * sizeof(ae_dev_list_t *));
    if(ae_dev_list == NULL) {
        res = AE_MALLOC_ERROR;
        goto ERR_EXIT;
    }
    memset(ae_dev_list, 0, ae_info.dev_num * sizeof(ae_dev_list_t *));
    
    int dev_list_cnt = 0, rule_dev_arr_per = 0;
    cJSON * dev_list_num = NULL;
    for(int i=0; i<rule_dev_obj_num; i++) {
        json_temp = cJSON_GetArrayItem(rule_dev, i);
        rule_dev_arr_per = cJSON_GetArraySize(json_temp);
        for(int j = 0; j<rule_dev_arr_per; j++) {
            dev_list_num = cJSON_GetArrayItem(json_temp, j);
            ae_dev_list[dev_list_cnt].dev_id = cJSON_GetNumberValue(dev_list_num);
            ae_dev_list[dev_list_cnt].rule = &ae_arr_rule[i];
            dev_list_cnt++;
        }
    }

    /* 将JSON格式种的数据解析到结构体中 */
    for(int i=0; i<ae_info.rule_num; i++) {
        //解析判断规则
        json_temp = cJSON_GetArrayItem(rule, i);
        ae_arr_rule[i].rule = ae_my_strdup(cJSON_GetStringValue(json_temp));
        ae_parse_rule_to_list(&ae_arr_rule[i], ae_arr_rule[i].rule);

        //解析输出
        json_temp = cJSON_GetArrayItem(rule_out, i);
        ae_arr_rule[i].out_opt = ae_my_strdup(cJSON_GetStringValue(json_temp));

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

        json_temp_para = cJSON_GetObjectItem(json_temp, "para");
        ae_arr_rule[i].para.para_string = ae_my_strdup(cJSON_GetStringValue(json_temp_para));
    }

    return AE_OK;

    ERR_EXIT:
    ae_info.rule_num = 0;
    return res;
}

/* 常规判断 */
static ae_eu_result_t ae_judge_normal(ae_dev_list_t *popt) {
    int i = 0;
    float * cur_val = NULL;
    ae_eu_result_t cur_res = AE_FALSE;

    /* 连续判断多条规则 */
    for(i=0; i<popt->rule->obj_num; i++) {
        //对源数据和阈值数据进行比较
        cur_val = ae_get_val_p(popt->dev_id, popt->rule->judge_unit[i].srcname);
        if(popt->rule->judge_unit[i].judge_pfun) {
            cur_res = popt->rule->judge_unit[i].judge_pfun(cur_val, &(popt->rule->judge_unit[i].cdtval));
        }
        else {
            return AE_FALSE;
        }

        //连续逻辑判断时，根据逻辑符号进行判断
        if(popt->rule->judge_unit[i].logic_next == AE_AND) {
            if(cur_res == AE_FALSE) return AE_FALSE;
        }
        else if(popt->rule->judge_unit[i].logic_next == AE_OR || 
                popt->rule->judge_unit[i].logic_next == AE_DIST) {
            if(cur_res == AE_TRUE) return AE_TRUE;
        }
        else if(popt->rule->judge_unit[i].logic_next == AE_NOLOGIC) {
            return cur_res;
        }
        else {
            return AE_FALSE;
        }
    }
}

/* 启停时间段判断 */
typedef struct ae_runsaso_t {
    uint32_t start_timestamp;
    float start_EPI;
    float stop_EPI;
    uint8_t flag; //bit0：0-停止 1-启动
}ae_runsaso_t;
static ae_eu_result_t ae_judge_runsaso(ae_dev_list_t *popt) {
    /*
        "rule": "Ua > 200 && P > 1; Ua < 150 || P < 0.5"
        "para": "RUN_START;RUN_STOP"
    */

    int i = 0;
    float * cur_val = NULL;
    ae_eu_result_t cur_res = AE_FALSE, res1, res2;
    ae_runsaso_t * saveinfo = NULL;
    
    /* 初始化savebuf */
    if(popt->savebuf == NULL) {
        popt->savebuf = ae_malloc(sizeof(ae_runsaso_t));
        if(popt->savebuf == NULL) {
            return AE_FALSE;
        }
        memset(popt->savebuf, 0, sizeof(ae_runsaso_t));
    }
    saveinfo = (ae_runsaso_t *)popt->savebuf;

    /* 判断res1 */
    for(i=0; i<popt->rule->obj_num; i++) {
        /* 获取数据后判断 */
        cur_val = ae_get_val_p(popt->dev_id, popt->rule->judge_unit[i].srcname);
        if(popt->rule->judge_unit[i].judge_pfun) {
            cur_res = popt->rule->judge_unit[i].judge_pfun(cur_val, &(popt->rule->judge_unit[i].cdtval));
        }
        else {
            return AE_FALSE;
        }

        /* 判断逻辑关系 */
        if(popt->rule->judge_unit[i].logic_next == AE_AND) {
            if(cur_res == AE_FALSE) {
                res1 = cur_res;
                break;
            }
        }
        else if(popt->rule->judge_unit[i].logic_next == AE_OR) {
            if(cur_res == AE_TRUE) {
                res1 = cur_res;
                break;
            }
        }
        else if(popt->rule->judge_unit[i].logic_next == AE_DIST) {
            res1 = cur_res;
            break;
        }
    }

    /* 判断res2 */
    for(i++; i<popt->rule->obj_num; i++) {
        cur_val = ae_get_val_p(popt->dev_id, popt->rule->judge_unit[i].srcname);
        if(popt->rule->judge_unit[i].judge_pfun) {
            cur_res = popt->rule->judge_unit[i].judge_pfun(cur_val, &(popt->rule->judge_unit[i].cdtval));
        }
        else {
            return AE_FALSE;
        }

        /* 判断逻辑关系 */
        if(popt->rule->judge_unit[i].logic_next == AE_AND) {
            if(cur_res == AE_FALSE) {
                res2 = cur_res;
                break;
            }
        }
        else if(popt->rule->judge_unit[i].logic_next == AE_OR) {
            if(cur_res == AE_TRUE) {
                res2 = cur_res;
                break;
            }
        }
        else if(popt->rule->judge_unit[i].logic_next == AE_NOLOGIC) {
            res2 = cur_res;
            break;
        }
    }

    /* 判断成功，进行输出 */
    if(res1 == AE_TRUE && res2 == AE_FALSE && ((saveinfo->flag & 0x01) == 0)) {
        /* 启动 */
        saveinfo->flag |= 0x01;
        printf(" dev_id[%d], run start\n", popt->dev_id);
        return AE_OK;
    }
    else if(res1 == AE_FALSE && res2 == AE_TRUE && (saveinfo->flag & 0x01)) {
        /* 停止 */
        saveinfo->flag &= ~0x01;
        printf(" dev_id[%d], run stop\n", popt->dev_id);
        return AE_OK;
    }
    else {
        return AE_FALSE;
    }
}

/* 进行判断入口 */
static ae_eu_result_t ae_rule_pro(ae_dev_list_t *popt)
{
    ae_eu_result_t res = AE_FALSE;
    printf(" [DBG s] \n");
    switch(popt->rule->para.type) 
    {
        case AE_TYPE_NORMAL:
            printf(" [DBG 5] \n");
            res = ae_judge_normal(popt);
        break;

        case AE_TYPE_RUNSASO:
            printf(" [DBG 4] \n");
            res = ae_judge_runsaso(popt);
        break;

        default: 
            res = AE_FALSE;
        break;
    }
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

/* 模块循环运行线程 */
void ae_loop(void)
{
    static int dev_cnt = 0;
    ae_eu_result_t judg_res = AE_FALSE;
    ae_dev_list_t * cur_dev_p = NULL;
    ae_rule_t * cur_rule_p = NULL;

    if(ae_info.rule_num ==0 || ae_info.dev_num == 0) {
        return;
    }

    /* 取下一个判断对象 */
    cur_dev_p = &ae_dev_list[dev_cnt];
    cur_rule_p = cur_dev_p->rule;
    dev_cnt++;
    if(dev_cnt >= ae_info.dev_num) dev_cnt = 0;

    /* 判断是否开启 */
    if(cur_rule_p->para.para_flag & 0x01 == 0) {
        return;
    }

    /* 进行规则判断并执行结果输出 */
    judg_res = ae_rule_pro(cur_dev_p);

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

