#include "fcgi_stdio.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "make_log.h" //��־ͷ�ļ�
#include "util_cgi.h"
#include "deal_mysql.h"
#include "cfg.h"
#include "cJSON.h"
#include <sys/time.h>

#define REG_LOG_MODULE       "cgi"
#define REG_LOG_PROC         "reg"


//�����û�ע����Ϣ��json��
int get_reg_info(char *reg_buf, char *user, char *nick_name, char *pwd, char *tel, char *email)
{
	int ret = 0;

	/*json��������
	{
		userName:xxxx,
		nickName:xxx,
		firstPwd:xxx,
		phone:xxx,
		email:xxx
	}
	*/

	//����json��
	//����һ��json�ַ���ΪcJSON����
	cJSON * root = cJSON_Parse(reg_buf);
	if (NULL == root)
	{
		LOG(REG_LOG_MODULE, REG_LOG_PROC, "cJSON_Parse err\n");
		ret = -1;
		goto END;
	}

	//����ָ���ַ�����Ӧ��json����
	//�û�
	cJSON *child1 = cJSON_GetObjectItem(root, "userName");
	if (NULL == child1)
	{
		LOG(REG_LOG_MODULE, REG_LOG_PROC, "cJSON_GetObjectItem err\n");
		ret = -1;
		goto END;
	}

	//LOG(REG_LOG_MODULE, REG_LOG_PROC, "child1->valuestring = %s\n", child1->valuestring);
	strcpy(user, child1->valuestring); //��������

	//�ǳ�
	cJSON *child2 = cJSON_GetObjectItem(root, "nickName");
	if (NULL == child2)
	{
		LOG(REG_LOG_MODULE, REG_LOG_PROC, "cJSON_GetObjectItem err\n");
		ret = -1;
		goto END;
	}
	strcpy(nick_name, child2->valuestring); //��������

	//����
	cJSON *child3 = cJSON_GetObjectItem(root, "firstPwd");
	if (NULL == child3)
	{
		LOG(REG_LOG_MODULE, REG_LOG_PROC, "cJSON_GetObjectItem err\n");
		ret = -1;
		goto END;
	}
	strcpy(pwd, child3->valuestring); //��������

	//�绰
	cJSON *child4 = cJSON_GetObjectItem(root, "phone");
	if (NULL == child4)
	{
		LOG(REG_LOG_MODULE, REG_LOG_PROC, "cJSON_GetObjectItem err\n");
		ret = -1;
		goto END;
	}
	strcpy(tel, child4->valuestring); //��������

	//����
	cJSON *child5 = cJSON_GetObjectItem(root, "email");
	if (NULL == child5)
	{
		LOG(REG_LOG_MODULE, REG_LOG_PROC, "cJSON_GetObjectItem err\n");
		ret = -1;
		goto END;
	}
	strcpy(email, child5->valuestring); //��������

END:
	if (root != NULL)
	{
		cJSON_Delete(root);//ɾ��json����
		root = NULL;
	}

	return ret;
}

int user_register(char *reg_buf)
{
	MYSQL* conn = NULL;
	char mysql_user[256] = { 0 };
	char mysql_pwd[256] = { 0 };
	char mysql_db[256] = { 0 };
	int ret = 0;
	ret = get_mysql_info(mysql_user, mysql_pwd, mysql_db);//��ȡ���ݿ����ӵ���Ϣ
	if (ret != 0)
	{
		goto END;
	}

	//��ȡע���û�����Ϣ
	char user[128];
	char nick_name[128];
	char pwd[128];
	char tel[128];
	char email[128];
	ret = get_reg_info(reg_buf, user, nick_name, pwd, tel, email);
	if (ret != 0)
	{
		goto END;
	}

	//�������ݿ�
	conn = msql_conn(mysql_user, mysql_pwd, mysql_db);
	if (conn == NULL)
	{
		LOG(REG_LOG_MODULE, REG_LOG_PROC, "msql_conn err\n");
		ret = -1;
		goto END;
	}

	//�������ݿ���룬��Ҫ�������ı�������
	mysql_query(conn, "set names utf8");

	char sql_cmd[SQL_MAX_LEN] = { 0 };
	sprintf(sql_cmd, "select * from user where name = '%s'", user);

	int ret2 = 0;
	ret2 = process_result_one(conn, sql_cmd, NULL);
	if (ret2 == 2)
	{
		LOG(REG_LOG_MODULE, REG_LOG_PROC, "��%s�����û��Ѵ���\n");
		ret = -2;
		goto END;
	}

	//��ǰʱ���
	struct timeval tv;
	struct tm* ptm;
	char time_str[128];

	//ʹ�ú���gettimeofday()�������õ�ʱ�䡣���ľ��ȿ��Դﵽ΢��
	gettimeofday(&tv, NULL);
	ptm = localtime(&tv.tv_sec);//�Ѵ�1970-1-1�����ֵ���ǰʱ��ϵͳ��ƫ�Ƶ�����ʱ��ת��Ϊ����ʱ��
	//strftime() ���������������ø�ʽ������ʱ��/���ڣ������Ĺ��ܽ�ʱ���ʽ��������˵��ʽ��һ��ʱ���ַ���
	strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", ptm);

	//sql ���, ����ע����Ϣ
	sprintf(sql_cmd, "insert into user (name, nickname, password, phone, createtime, email) values ('%s', '%s', '%s', '%s', '%s', '%s')", user, nick_name, pwd, tel, time_str, email);

	if (mysql_query(conn, sql_cmd) != 0)
	{
		LOG(REG_LOG_MODULE, REG_LOG_PROC, "%s ����ʧ�ܣ�%s\n", sql_cmd, mysql_error(conn));
		ret = -1;
		goto END;
	}

END:

	if (conn != NULL)
	{
		mysql_close(conn); //�Ͽ����ݿ�����
	}

	return ret;
}

int main()
{
	//�����ȴ��û�����
	while (FCGI_Accept() >= 0)
	{
		LOG(REG_LOG_MODULE, REG_LOG_PROC, "FCGI_Accept()\n");
		char *contenLength = getenv("CONTENT_LENGTH");//��ȡ�ͷ��˷��������ݳ���
		int len;
		printf("Content-type: text/html\r\n\r\n");
		
		if (contenLength == NULL)
		{
			len = 0;
		}
		else
		{
			len = atoi(contenLength);
		}
		if (len <= 0)
		{
			printf("No data from standar input.<p>\n");
			LOG(REG_LOG_MODULE, REG_LOG_PROC, "len = 0, No data from standar input<p>\n");
		}
		else//��ȡ�û�����
		{
			char buf[4 * 1024] = { 0 };
			int ret = 0;
			char *out = NULL;

			ret = fread(buf, 1, len, stdin);
			if (ret == 0)
			{
				LOG(REG_LOG_MODULE, REG_LOG_PROC, " fread(buf, 1, len, stdin) err \n");
				continue;
			}

			LOG(REG_LOG_MODULE, REG_LOG_PROC, "buf = %s\n", buf);

			ret = user_register(buf);
			if (ret == 0)//ע��ɹ�
			{
				out = return_status("002");
			}
			else if (ret == -1)//ע��ʧ��
			{
				out = return_status("004");
			}
			else if (ret == -2)//�û��Ѿ�����
			{
				out = return_status("003");
			}
			if (out != NULL)
			{
				printf(out);
				free(out);
			}
		}
	}
}

