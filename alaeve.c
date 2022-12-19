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

char *cjosn_out = NULL;
void main(char argc, char *agrv[])
{
    FILE *fp = NULL;
    int rw = 0;

    fp = fopen("./RULE.json", "r");

    cJSON *root = cJSON_CreateObject();

    cJSON_AddItemToObject(root, "abc", cJSON_CreateString("hello"));
    cjosn_out = cJSON_Print(root);

    

    printf("json:\n%s\n", cjosn_out);
}

