#include "stdafx.h"
#include "Config.h"
#include <string>
CConfig::~CConfig()
{

}

BOOL CConfig::LoadConfig(const CString& config_file_path)
{
	CFile			file;
	ULONGLONG		FileSize;
	CArray<CHAR>    buff;
	

	if (!file.Open(config_file_path, CFile::modeRead))
		return FALSE;

	FileSize = file.GetLength();
	buff.SetSize(FileSize);
	file.Read(buff.GetData(), FileSize);
	file.Close();

	if (buff.GetSize() == 0)
	{
		return FALSE;
	}

	if (!Json::Reader().parse(buff.GetData(), m_config))
	{
		return FALSE;
	}

	return TRUE;
}

VOID CConfig::SaveConfig(const CString & config_file_path) const
{
	CFile     file;
	std::string config = Json::FastWriter().write(m_config);
	
	if (!file.Open(config_file_path, CFile::modeWrite|CFile::modeCreate))
		return;

	file.Write(config.c_str(), config.length());
	file.Close();
}

//void CConfig::DefaultConfig()
//{
//	Json::Value server;
//	Json::Value filetrans;
//	Json::Value remote_desktop;
//	Json::Value cam;
//	Json::Value lan;
//	Json::Value groups;
//
//	if (m_config.size())
//	{
//		m_config.clear();
//	}
//
//	////server....
//	server ["listen_port"]     = "10086";
//	server ["max_connections"] = "10000";
//	server ["modules"]         = "modules";
//
//	////File transfer;
//	filetrans["overwrite_file"] = "true";
//
//	////Remote Desktop
//	remote_desktop["record_save_path"] = "data\\screenshot\\records";
//	remote_desktop["screenshot_save_path"] = "data\\remotedesktop\\screenshot";
//	////Camera...
//	cam["record_save_path"] = "data\\cam\\records";
//	cam["screenshot_save_path"] = "data\\cam\\screenshot";
//
//	//LAN IP
//	lan["address"] = "127.0.0.1";
//	lan["broadcast"] = "127.0.0.1";
//
//	//Set Default Configs...
//	m_config["server"] = server;
//	m_config["filetrans"] = filetrans;
//	m_config["remote_desktop"] = remote_desktop;
//	m_config["cam"] = cam;
//	m_config["lan"] = lan;
//}
//
//CString CConfig::GetConfig(const CString& key, const CString&subkey) const 
//{	
//	CStringA k(key);
//	CStringA sk(subkey);
//
//	return CString(m_config[k][sk].asCString());
//}
//
//void CConfig::SetConfig(const CString&key, const CString&subkey, const CString&value)
//{
//	CStringA k(key);
//	CStringA sk(subkey);
//
//	m_config[k][sk] = CStringA(value).GetBuffer();
//}