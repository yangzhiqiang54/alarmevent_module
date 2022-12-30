/*
    ģ�����ƣ��������¼��߼��жϴ���ģ��
    ���룺�ַ������жϹ�������Դ����������������
    ������жϽ������ֵ����ǰֵ
    ���ߣ�yzq

    Ҫ��
        �����ִ��������еĿո�


    �汾��¼��
        2022-12-17 ��һ�汾
*/

#include "alaeve.h"
#include "cJSON.h"
#include "stdio.h"
#include "string.h"
#include "stdlib.h"
#include "sys/stat.h"

/* �궨�� */
#define ae_malloc   malloc
#define ae_free     free
#define ae_remalloc realloc
/****end****/

/* ö�� */
typedef enum {
    AE_FALSE = 2,
    AE_TRUE = 1,
    AE_OK = 0,
    AE_JSON_ERROR = -1,
}ae_eu_result_t;

typedef enum {
    AE_TYPE_NORMAL = 0,
    AE_TYPE_PAIR = 1,
}ae_eu_type_t;
/****end****/

/* typedef */
typedef struct ae_judge_list_t {
    ae_judge_list_t * next;
    void * val;
    int (* logic_judg_tfun)(void *);
    uint8_t logic_next; //0-NULL; 1-&&; 2-||
}ae_judge_list_t;

typedef struct ae_para_t {
    uint32_t period; //�жϼ�� ��
    ae_eu_type_t type; //���� ö�� �����й̶��ַ���
    char *para_string; //�����ַ���ָ��
    uint8_t para_flag; //�������� bit0:����
    uint8_t trans_id; //ת��ID
}ae_para_t;

typedef struct ae_rule_t {
    ae_para_t para; //����
    // char *src_data; //����Դ
    char *judg; //�ж��ַ���
    ae_judge_list_t *jnode; //���� �ж��ַ���������Ļص�����
    char *out_opt; //���
}ae_rule_t;

typedef struct ae_info_t {
    uint16_t num; //�����ж�����
}ae_info_t;

typedef struct ae_type_table_t {
    ae_eu_type_t sn;
    char *type_val;
}ae_type_table_t;
/****end****/

/* ���� */
ae_info_t ae_info; // ģ��ȫ����Ϣ��¼
ae_rule_t * ae_arr_rule = NULL; //�����¼��ṹ������
/****end****/

/* �����������ַ���ӳ��� */
#define AE_TYPE_TABLE_NUM 2
ae_type_table_t ae_type_table[AE_TYPE_TABLE_NUM] = {
    {AE_TYPE_NORMAL,    "normal"},
    {AE_TYPE_PAIR,      "pair"}
};

/* ������� */
/* ������ͷ�ڵ� */
ae_judge_list_t * ae_list_new_head(void) {
    ae_judge_list_t * plist = ae_malloc(sizeof(ae_judge_list_t));
    if(plist) memset(plist, 0, sizeof(ae_judge_list_t));
    return plist;
}

/* �����µĺ�һ���ڵ� */
ae_judge_list_t * ae_list_add_tail(ae_judge_list_t * cur_list) {
    ae_judge_list_t * plist = ae_malloc(sizeof(ae_judge_list_t));
    if(plist) memset(plist, 0, sizeof(ae_judge_list_t));
    else return NULL;
    
    cur_list->next = plist;
    return plist;
}
/* end ������� */

/* �ַ������� ���붯̬�ڴ� */
static char *ae_my_strdup(char *input)
{
    
    int len = strlen(input);
    char *output = (char *)ae_malloc(len + 1); //����һ���������ĳ���
    if (output == NULL)
        return NULL;
    memset(output, 0, len + 1);
    memcpy(output, input, len);
    return output;
}

/* �����������ַ��������ɶ�Ӧ������ö��ֵ */
static ae_eu_type_t ae_type_parse(char * str) {
    for(int i=0; i<AE_TYPE_TABLE_NUM; i++) {
        if(strcmp(str, ae_type_table[i].type_val) == 0) {
            return ae_type_table[i].sn;
        }
    }
    return AE_TYPE_NORMAL; //�Ҳ�����Ӧ�����ͣ��򷵻���ͨ����
}

/* ���ж��ַ��������ɶ�Ӧ������Դָ����߼��жϻص����� */
void ae_parse_rule_to_list(ae_rule_t * opt, char * st_judge) {
    
}

/* �������ļ����ݽ������ڴ��ae�ṹ���� */
static ae_eu_result_t ae_parse_config(cJSON * json_root) {
    ae_eu_result_t res = AE_OK;
    cJSON *rule, *rule_out, *rule_para, *json_temp, *json_temp_para;
    char tempbuf[64] = {0};
    
    rule = cJSON_GetObjectItem(json_root, "rule");
    rule_out = cJSON_GetObjectItem(json_root, "rule_out");
    rule_para = cJSON_GetObjectItem(json_root, "rule_para");

    /* �ж�JSON��ʽ�Ƿ�����ɹ� */
    if(rule==0 || rule_out==0 || rule_para==0) {
        return AE_JSON_ERROR;
    }

    if( !(cJSON_IsArray(rule) && cJSON_IsArray(rule_out) && cJSON_IsArray(rule_para)) ) {
        return AE_JSON_ERROR;
    }

    /* ��ȡ�����¼����������� */
    ae_info.num = cJSON_GetArraySize(rule);
    if(ae_info.num !=  cJSON_GetArraySize(rule_out)) {
        res = AE_JSON_ERROR;
        goto ERR_EXIT;
    }
    if(ae_info.num !=  cJSON_GetArraySize(rule_para)) {
        res = AE_JSON_ERROR;
        goto ERR_EXIT;
    }
    
    /* �������¼��ṹ�����붯̬�ڴ� */
    ae_arr_rule = ae_malloc(sizeof(ae_rule_t) * ae_info.num);
    memset(ae_arr_rule, 0, (sizeof(ae_rule_t) * ae_info.num));
    if(ae_arr_rule == NULL) {
        res = AE_JSON_ERROR;
        goto ERR_EXIT;
    }

    /* ��JSON��ʽ�ֵ����ݽ������ṹ���� */
    for(int i=0; i<ae_info.num; i++) {
        json_temp = cJSON_GetArrayItem(rule, i);
        ae_parse_rule_to_list(&ae_arr_rule[i], cJSON_GetStringValue(json_temp));

        json_temp = cJSON_GetArrayItem(rule_out, i);
        ae_arr_rule[i].out_opt = ae_my_strdup(cJSON_GetStringValue(json_temp));

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

        /* ���֧��p1 p2 p3 */ 
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
    ae_info.num = 0;
    return res;
}



/* �����ж� */
static ae_eu_result_t ae_rule_judge(ae_rule_t *prule)
{
    



    return AE_FALSE;
}

/* �������� */
typedef struct {
    char name[8];
    float val;
}test_data_t;
test_data_t test_data[] = {
    {"Ua", 220},{"Ub", 221},{"Uc", 221},
    {"Ia", 5},{"Ib", 6},{"Ic", 7},
    {"Pa", 0.1},{"Pb", 0.2},{"Pc", 0.3},{"P", 0.6}
};
/****end****/


/*******/
/* API */
/*******/

/* ����ʱ��ģ���ʼ�� */
int ae_init(void)
{
    memset(&ae_info, 0, sizeof(ae_info_t));
}

/* ģ��ѭ�������߳� */
void ae_loop(void)
{
    static int num_cnt = 0;
    ae_rule_t * p_rule_loop = NULL;
    ae_eu_result_t judg_res = AE_FALSE;

    if(ae_info.num ==0 || ae_arr_rule == NULL) {
        return;
    }

    /* ȡ��һ���ж϶��� */
    p_rule_loop = &ae_arr_rule[num_cnt];
    num_cnt += 1;
    if(num_cnt >= ae_info.num) num_cnt = 0;

    /* �ж��Ƿ��� */
    if(p_rule_loop->para.para_flag & 0x01 == 0) {
        return;
    }

    /* �жϽ����� */
    judg_res = ae_rule_judge(p_rule_loop);



}

/****end****/

void main(char argc, char *agrv[])
{
    FILE *fp = NULL;
    char *fbuf = NULL;
    int rw = 0, file_size = 0;
    struct stat stat_temp;

    /* ��ȡ�ļ� */
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
    
    /* ����JSON���� */
    cJSON * root = cJSON_Parse(fbuf);
    free(fbuf);
    if(root == NULL) {
        goto EXIT;
    }

    /* ��JSON���ݽ�����ae�ṹ���� */
    ae_eu_result_t res = ae_parse_config(root);
    cJSON_Delete(root);

    /* �������� */
    ae_parse_rule();

    while(1) {
        ae_loop();
    }

    EXIT:
    return;
}

