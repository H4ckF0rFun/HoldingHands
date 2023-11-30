#pragma once
#include "json\json.h"
/*
config format....
#
key = value

.....这东西直接拿json就可以了.....
*/

class CConfig
{
private:
	Json::Value m_config;
	

public:
	void DefaultConfig();
	BOOL LoadConfig(const CString & config_file_path);
	VOID SaveConfig(const CString & config_file_path) const;
	
	CString     	GetConfig(const CString& key, const CString& subkey) const ;
	void		    SetConfig(const CString& key, const CString& subkey, const CString& value);

	CConfig()
	{
		DefaultConfig();
	}

	CConfig(const CString & config_file_path)
	{
		if (!LoadConfig(config_file_path))
		{
			DefaultConfig();
		}
	}
	~CConfig();
};

