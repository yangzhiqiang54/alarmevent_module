/*
    ģ�����ƣ��������¼��߼��жϴ���ģ��
    ���룺�ַ������жϹ�������Դ����������������
    ������жϽ������ֵ����ǰֵ
    ���ߣ�yzq
    
    �汾��¼��
        2022-12-17 ��һ�汾
*/

#include "alaeve.h"
#include "cJSON.h"
#include "stdio.h"

/* �궨�� */

/****end****/

/* ���� */
ae_info_t ae_info; //ģ��ȫ����Ϣ��¼
/****end****/

/* ����ʱ��ģ���ʼ�� */
int ae_init(void) {
    

}

char *cjosn_out = NULL;
void main(char argc, char* agrv[]) {
    cJSON *root = cJSON_CreateObject();

    cJSON_AddItemToObject(root, "abc", cJSON_CreateNumber(123.456789));
    cjosn_out = cJSON_Print(root);

    printf("json:\n%s\n", cjosn_out);
}




