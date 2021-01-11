/** \file ConfigFile.h
/brief ���� �������� ����� ��� ������ � ��������� ������ ������������
*/
#pragma once

#include <stdio.h>
#include <codecvt>
#include "main.h"

/** \class CConfigFile
\brief ����� �������� �������� � ������ ��� ������ � ��������� ������ ������������
*/
class CConfigFile
{
public:
	/** \fn CConfigFile(void)
	\brief �����������
	*/
	CConfigFile(bool bWrite);
	/** \fn virtual ~CConfigFile(void)
	\brief ����������
	*/
	virtual ~CConfigFile(void);

	/** \fn int GetStringParameter(const wchar_t* szSection, const wchar_t* szParameter, wstring& strResult)
	\brief �������� ������ �� ����� ������������
	\param[in] szSection - ��������� �� ��� ������
	\param[in] szParameter - ��������� �� ��� ���������
	\param[out] strResult - ������ �� ������, ���� ����� ������� ���������
	\return ����� htpekmnbhe.otq ������
	*/
	size_t GetStringParameter(const char* szSection, const char* szParameter, wstring& wstrResult);
	/** \fn int _GetRegistryState(wchar_t* szBuffer, size_t stBufferSize)
	\brief �������� ����������� ��������� ����������� � �����
	\param[out] szBuffer - ��������� �� �����, ���� ����� ������� ���������
	\param[in] stBufferSize - ����� ������ ����������
	\return ����� ������ � ������
	*/
	int GetIntParameter(const char* szSection, const char* szParameter, int iDefault);
	/** \fn int _GetRegistryState(wchar_t* szBuffer, size_t stBufferSize)
	\brief �������� ����������� ��������� ����������� � �����
	\param[out] szBuffer - ��������� �� �����, ���� ����� ������� ���������
	\param[in] stBufferSize - ����� ������ ����������
	\return ����� ������ � ������
	*/
	size_t GetCodedStringParameter(const char* szSection, const char* szParameter, wstring& wstrResult);
	bool _CheckConfig();
private:
	FILE* m_fCfgFile{ nullptr }; /**< ��������� �� ���������� ��������� ����� ������������*/
	bool m_bWriteOnly{ false }; /**< ���� ������ ������ ��� ������*/

};
