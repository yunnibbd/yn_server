#include "cconfig.h"
#include "clog.h"
#include <string.h>
#include <iostream>
#include <fstream>
using namespace std;

/**
 * @brief 切除左边的空格
 * @param str 源字符串
 * @return
 */
static void LTrim(char *str)
{
	if (!str) return;
	char *pstr = str;
	while (*pstr == ' ')
		++pstr;
	while (*pstr)
	{
		*str = *pstr;
		++str;
		++pstr;
	}
	*str = 0;
}

/**
 * @brief 切除右边的空格
 * @param str 源字符串
 * @return
 */
static void RTrim(char *str)
{
	if (!str) return;
	int len = strlen(str);
	int index = --len;
	while (index > 0 && str[index] == ' ')
		str[index--] = 0;
}

CConfig::~CConfig()
{
	DelAllConfig();
}

/**
 * @brief 读取配置文件
 * @param filename 配置文件名
 * @return bool 是否加载配置成功
 */
bool CConfig::Load(const char *filename)
{
	if (!filename) return false;
	if (!is_first_load_)
		//如果是第一次载入配置文件就表示为true
		is_first_load_ = true;
	else
		//如果不是第一次载入配置文件就释放以前的配置内存
		DelAllConfig();
	ifstream ifs(filename);
	if (!ifs.is_open())
	{
		LOG_WARNING("CConfig::Load file is not open\n");
		return false;
	}

	//每一行的配置文件读取出来都在这里
	char linebuf[512] = { 0 };
	while (!ifs.eof())
	{
		ifs.getline(linebuf, sizeof(linebuf) - 1);
		int nLen = strlen(linebuf);
		if (nLen == 0)
			continue;
		if (linebuf[0] == 0)
			continue;
		if (*linebuf == ' ' ||
			*linebuf == ';' ||
			*linebuf == '\t' ||  
			*linebuf == '#' || 
			*linebuf == '\n' || 
			*linebuf == '[')
			continue;
		//切除后面的换行 回车 或者 空格 等
		while (linebuf[nLen - 1] == 10 || 
			linebuf[nLen - 1] == 13 ||
			linebuf[nLen - 1] == 32)
			linebuf[nLen - 1] = 0;
		if (linebuf[0] == 0)
			//经过处理后变成了空行
			continue;
		if (linebuf[0] == '[')
			//配置组信息也不处理
			continue;

		//找出这一行中的等于号
		//name = lin
		char *ret = strchr(linebuf, '=');
		if (!ret) continue;
		ConfigItem *item = new ConfigItem();
		strncpy(item->key, linebuf, ret - linebuf);
		strcpy(item->val, ret + 1);
		LTrim(item->key);
		RTrim(item->key);
		LTrim(item->val);
		RTrim(item->val);
		configs_.push_back(item);
	}

	ifs.close();
	return true;
}

/**
 * @brief 删除所有配置项
 * @param
 * @return
 */
void CConfig::DelAllConfig()
{
	for (auto &config : configs_)
		delete config;
	configs_.clear();
}

/**
 * @brief 获取配置项
 * @param key 配置名
 * @return char* 配置内容
 */
char *CConfig::GetString(const char *key)
{
	if (!key) return nullptr;
	int len = strlen(key);
	for (auto &config : configs_)
	{
		if (strncmp(key, config->key, len) == 0)
			return config->val;
	}
	return nullptr;
}

/**
 * @brief 获取配置项，可以传入默认值
 * @param key 配置名
 * @param def 默认值
 * @return int 配置内容或者默认值
 */
int CConfig::GetIntDefault(const char *key, int def)
{
	if (!key) return 0;
	int len = strlen(key);
	for (auto &config : configs_)
	{
		if (strncmp(key, config->key, len) == 0)
			return atoi(config->val);
	}
	return def;
}

/**
 * @brief 测试打印所有配置项
 * @param
 * @return
 */
void CConfig::ShowAllConfig()
{
	for (auto &config : configs_)
		cout << "#" <<config->key << "#" << ":" 
		<< "#" << config->val << "#" << endl;
}
