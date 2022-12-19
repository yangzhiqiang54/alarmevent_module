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

/* 变量 */
ae_info_t ae_info; // 模块全局信息记录
/****end****/

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

/* 报警时间模块初始化 */
int ae_init(void)
{
    memset(&ae_info, 0, sizeof(ae_info_t));
}

/* 将配置文件内容解析到内存的ae结构体中 */
int ae_pares_config(void) {


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
    cJSON *root = cJSON_Parse(fbuf);
    free(fbuf);

    return;
}

