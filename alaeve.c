/*
    模块名称：报警、事件逻辑判断处理模块
    输入：字符串、判断规则、数据源、输出对象、输出内容
    输出：判断结果、阈值、当前值
    作者：yzq
    
    版本记录：
        2022-12-17 第一版本
*/

#include "alaeve.h"
#include "cJSON.h"
#include "stdio.h"

/* 宏定义 */

/****end****/

/* 变量 */
ae_info_t ae_info; //模块全局信息记录
/****end****/

/* 报警时间模块初始化 */
int ae_init(void) {
    

}

char *cjosn_out = NULL;
void main(char argc, char* agrv[]) {
    cJSON *root = cJSON_CreateObject();

    cJSON_AddItemToObject(root, "abc", cJSON_CreateNumber(123.456789));
    cjosn_out = cJSON_Print(root);

    printf("json:\n%s\n", cjosn_out);
}




