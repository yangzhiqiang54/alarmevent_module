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

/* �������ļ����ݽ������ڴ��ae�ṹ���� */
int ae_pares_config(void) {


}


void main(char argc, char *agrv[])
{
    FILE *fp = NULL;
    char *fbuf = NULL;
    int rw = 0, file_size = 0;
    struct stat stat_temp;

    /* ��ȡ�ļ� */
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
    
    /* ����JSON���� */
    cJSON *root = cJSON_Parse(fbuf);
    free(fbuf);

    return;
}

