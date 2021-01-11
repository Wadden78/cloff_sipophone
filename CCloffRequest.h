/*****************************************************************//**
 * \file   CCloffRequest.h
 * \brief  Реализация запроса к БД CLOFF
 * 
 * \author Wadden
 * \date   November 2020
 *********************************************************************/
#pragma once
#include "Main.h"

class CCloffRequest
{
public:
	CCloffRequest();
	~CCloffRequest();
	int RequestDataFromDataBase(const char* szRequest, string& strAnswer);
private:
	int HTTPParser(const char* pszText, string& strJSON_Result);
};

