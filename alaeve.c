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

/* �궨�� */

/****end****/

/* ���� */
ae_info_t ae_info; // ģ��ȫ����Ϣ��¼
/****end****/

/* �ַ������� ���붯̬�ڴ� */
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

/* ����ʱ��ģ���ʼ�� */
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

