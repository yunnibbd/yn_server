#ifndef __CCONFIG_H__
#define __CCONFIG_H__
#include "./../export_api.h"
#include <vector>

/**
 * @brief 每一项配置
 */
struct ConfigItem
{
	char key[50] = { 0 };
	char val[50] = { 0 };
};

/**
 * @brief 单例配置类
 */
class COMMONLIB_EXPORT CConfig
{
public:
	~CConfig();

	static CConfig *Get()
	{
		static CConfig *ins = new CConfig;
		return ins;
	}

	/**
	 * @brief 读取配置文件
	 * @param filename 配置文件名
	 * @return bool 是否加载配置成功
	 */
	bool Load(const char *filename);

	/**
	 * @brief 删除所有配置项
	 * @param
	 * @return
	 */
	void DelAllConfig();

	/**
	 * @brief 获取配置项
	 * @param key 配置名
	 * @return char* 配置内容
	 */
	char *GetString(const char *key);

	/**
	 * @brief 获取配置项，可以传入默认值
	 * @param key 配置名
	 * @param def 默认值
	 * @return int 配置内容或者默认值
	 */
	int GetIntDefault(const char *key, int def);

	/**
	 * @brief 测试打印所有配置项
	 * @param
	 * @return
	 */
	void ShowAllConfig();
private:
	//私有化构造函数
	CConfig(){}
	//存储所有的配置项
	std::vector<ConfigItem *> configs_;
	//是否是第一次Load标志位
	bool is_first_load_ = false;
};

#endif
