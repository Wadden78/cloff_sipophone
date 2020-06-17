#pragma once
#include <Windows.h>
#include <string>
#include <vector>
#include <commctrl.h>
#include "resource.h"
#include "CSIPProcess.h"

extern std::unique_ptr<CSIPProcess> m_SIPProcess;

extern std::wstring	wstrLogin;
extern std::wstring	wstrPassword;
extern std::wstring	wstrServer;
extern int iTransportType;
extern uint16_t uiRingToneNumber;
extern bool bDTMF_2833;
extern bool bDebugMode;
extern array<char, MAX_COMPUTERNAME_LENGTH + 1> szCompName;

#define WM_USER_REGISTER_OK	WM_USER + 1	//аккаунт зарегистрирован	
#define WM_USER_REGISTER_ER	WM_USER + 2	//ошибка регистрации аккаунта
#define WM_USER_REGISTER_IC	WM_USER + 3	//входящий вызов
#define WM_USER_REGISTER_OC	WM_USER + 4	//исходящий вызов
#define WM_USER_REGISTER_CN	WM_USER + 5	//разговорное состояние

#define AES_KEY_SIZE 16
#define CHUNK_SIZE (AES_KEY_SIZE*3) // an output buffer must be a multiple of the key size

void LoadConfig(HWND hWnd);
void SaveConfig(HWND hWnd);
