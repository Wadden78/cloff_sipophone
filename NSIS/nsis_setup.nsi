/*
CLOFF SIP Phone installer
*/
!include LogicLib.nsh
!include MUI2.nsh

Name "CLOFF SIP Phone"
!define MY_APP_NAME "CLOFF SIP Phone"
!define VERSION "1.0.4"
InstallDir $PROGRAMFILES\CLOFF
OutFile "cloff_sip_phone_installer.exe"
# For removing Start Menu shortcut in Windows 7
;RequestExecutionLevel user
ShowInstDetails show
Var InterfaceType
Var SIPLogin
Var SIPPassword

!define MUI_ICON "Cloff_Phone.ico"

Page custom fnCloseApplication

!define MUI_WELCOMEPAGE_TITLE "Здравствуйте."
!define MUI_WELCOMEPAGE_TEXT "Вас приветствует установщик SIP телефона компании Омикрон."
!define MUI_WELCOMEFINISHPAGE_BITMAP "welcome.bmp"
!define MUI_WELCOMEFINISHPAGE_BITMAP_STRETCH NoStretchNoCropNoAlign
!insertmacro MUI_PAGE_WELCOME

!insertmacro MUI_PAGE_LICENSE "License.txt"
!insertmacro MUI_PAGE_DIRECTORY

Page custom fnc_interface_Show fnInterfaceLeave
Page custom fnc_account_Show fnAccountLeave
Page custom fnc_install_confirm_Show

!insertmacro MUI_PAGE_INSTFILES

# These indented statements modify settings for MUI_PAGE_FINISH
!define MUI_FINISHPAGE_NOAUTOCLOSE
!define MUI_FINISHPAGE_RUN
!define MUI_FINISHPAGE_RUN_CHECKED
!define MUI_FINISHPAGE_RUN_TEXT "Запустить SIP Phone"
!define MUI_FINISHPAGE_RUN_FUNCTION "LaunchLink"
!insertmacro MUI_PAGE_FINISH

UninstPage custom un.fnCloseApplication

!define MUI_UNWELCOMEPAGE_TITLE "До свидания."
!define MUI_UNWELCOMEPAGE_TEXT "Сейчас Вы удалите программу SIP телефона. ."
!define MUI_UNWELCOMEFINISHPAGE_BITMAP "welcome.bmp"
!define MUI_UNWELCOMEFINISHPAGE_BITMAP_STRETCH NoStretchNoCropNoAlign
!insertmacro MUI_UNPAGE_WELCOME

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_DIRECTORY
!insertmacro MUI_UNPAGE_INSTFILES
UninstPage custom un.fnc_confirm_cfg_delete_Show un.fnDeleteConfirm
!insertmacro MUI_UNPAGE_FINISH

;Languages
!insertmacro MUI_LANGUAGE "Russian"
 
Var MyAppName
Section "Install"
	SetOutPath $INSTDIR	
	StrCpy $MyAppName "CLOFF SIP Phone"

	# specify file to go in output path
	File cloff_sipophone.exe
	File Cloff_Phone.ico

	# define uninstaller name
	WriteUninstaller $INSTDIR\uninstaller.exe

	${If} $InterfaceType == "2"
	    CreateShortcut "$DESKTOP\$MyAppName.lnk" "$INSTDIR\cloff_sipophone.exe" " -autostart -minimize"
		CreateShortcut "$SMPROGRAMS\$MyAppName.lnk" "$INSTDIR\cloff_sipophone.exe" " -autostart -minimize"
		WriteRegStr HKEY_CURRENT_USER "SOFTWARE\Microsoft\Windows\CurrentVersion\Run" $MyAppName "$INSTDIR\cloff_sipophone.exe -minimize"
	${Else}
	    CreateShortcut "$DESKTOP\$MyAppName.lnk" "$INSTDIR\cloff_sipophone.exe"
		CreateShortcut "$SMPROGRAMS\$MyAppName.lnk" "$INSTDIR\cloff_sipophone.exe"
	${EndIf}

	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\$MyAppName" "DisplayName" $MyAppName
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\$MyAppName" "DisplayIcon" "$INSTDIR\Cloff_Phone.ico"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\$MyAppName" "UninstallString" "$\"$INSTDIR\uninstaller.exe$\""
SectionEnd

/* Uninstaller section */
;!define MUI_UNICON "Cloff_Phone.ico"

Section "un.Uninstall"
	StrCpy $MyAppName "CLOFF SIP Phone"

	Call un.fnCloseApplication

	# Always delete uninstaller first
	Delete "$INSTDIR\uninstaller.exe"
 
    # second, remove the link from the start menu
    Delete "$SMPROGRAMS\$MyAppName.lnk"
    # second, remove the link from the start menu
    Delete "$DESKTOP\$MyAppName.lnk"

	# now delete installed file
	Delete "$INSTDIR\*.*"
 
	# Delete the directory
	RMDir "$INSTDIR"

	DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\$MyAppName"
	DeleteRegValue HKEY_CURRENT_USER "SOFTWARE\Microsoft\Windows\CurrentVersion\Run" $MyAppName
SectionEnd

/**********************/
Var LoginLen
Var PasswordLen
Function LaunchLink
	${If} $InterfaceType == "2"
		Exec '"$INSTDIR\cloff_sipophone.exe" -autostart -minimize'
	${Else}
		;Call fnc_account_Show
		StrLen $LoginLen $SIPLogin
		StrLen $PasswordLen $SIPPassword
		${If} $LoginLen > 0 
		${AndIf} $PasswordLen > 0 
			Exec '"$INSTDIR\cloff_sipophone.exe" -login:$SIPLogin -password:$SIPPassword'
		${Else}
			Exec '"$INSTDIR\cloff_sipophone.exe"'
		${EndIf}
	${EndIf}
FunctionEnd

; handle variables
Var hCtl_interface
Var hCtl_interface_Bitmap2
Var hCtl_interface_Bitmap2_hImage
Var hCtl_interface_Bitmap1
Var hCtl_interface_Bitmap1_hImage
Var hCtl_interface_RadioButton_WEB
Var hCtl_interface_RadioButton_Phone


; dialog create function
Function fnc_interface_Create

  ; === interface (type: Dialog) ===
  nsDialogs::Create 1018
  Pop $hCtl_interface
  ${If} $hCtl_interface == error
    Abort
  ${EndIf}
  !insertmacro MUI_HEADER_TEXT "Выбор интерфейса" "Пожалуйста выберите интерфейс для работы:"
  
  ; === Bitmap2 (type: Bitmap) ===
  ${NSD_CreateBitmap} 159u 6u 115u 111u ""
  Pop $hCtl_interface_Bitmap2
  File "/oname=$PLUGINSDIR\webphone_scr.bmp" "C:\Users\Wadden\Documents\Work\cloff_sipophone\NSIS\webphone_scr.bmp"
  ${NSD_SetImage} $hCtl_interface_Bitmap2 "$PLUGINSDIR\webphone_scr.bmp" $hCtl_interface_Bitmap2_hImage
  
  ; === Bitmap1 (type: Bitmap) ===
  ${NSD_CreateBitmap} 25u 6u 88u 111u ""
  Pop $hCtl_interface_Bitmap1
  File "/oname=$PLUGINSDIR\phone_scr.bmp" "C:\Users\Wadden\Documents\Work\cloff_sipophone\NSIS\phone_scr.bmp"
  ${NSD_SetImage} $hCtl_interface_Bitmap1 "$PLUGINSDIR\phone_scr.bmp" $hCtl_interface_Bitmap1_hImage
  
  ; === RadioButton_WEB (type: RadioButton) ===
  ${NSD_CreateRadioButton} 159u 118u 146u 15u "Интерфейс WEB кабинета CLOFF"
  Pop $hCtl_interface_RadioButton_WEB
  ${NSD_AddStyle} $hCtl_interface_RadioButton_WEB ${WS_GROUP}
  
  ; === RadioButton_Phone (type: RadioButton) ===
  ${NSD_CreateRadioButton} 25u 118u 105u 15u "Интерфейс SIP телефона"
  Pop $hCtl_interface_RadioButton_Phone
  ${NSD_Check} $hCtl_interface_RadioButton_Phone
  
FunctionEnd

; dialog show function
Function fnc_interface_Show
	Call fnc_interface_Create
	nsDialogs::Show
FunctionEnd

Var InterfaceCheck
Function fnInterfaceLeave
	${NSD_GetState} $hCtl_interface_RadioButton_WEB $InterfaceCheck
	${If} $InterfaceCheck ==  ${BST_CHECKED}
		StrCpy $InterfaceType "2" 
	${Else}
		StrCpy $InterfaceType "1" 
	${EndIf}
FunctionEnd


; handle variables
Var hCtl_account
Var hCtl_account_Label4
Var hCtl_account_Label3
Var hCtl_account_PasswordTextBox
Var hCtl_account_LoginTextBox
Var hCtl_account_Label2
Var hCtl_account_Label1
Var hCtl_account_Font1
Var hCtl_account_Font2
Var hCtl_account_Font3


; dialog create function
Function fnc_account_Create
  
    ; custom font definitions
  CreateFont $hCtl_account_Font1 "Microsoft Sans Serif" "60" "400"
  CreateFont $hCtl_account_Font2 "Microsoft Sans Serif" "16" "400"
  CreateFont $hCtl_account_Font3 "Microsoft Sans Serif" "14" "400"
  
  ; === account (type: Dialog) ===
  nsDialogs::Create 1018
  Pop $hCtl_account
  ${If} $hCtl_account == error
    Abort
  ${EndIf}
  !insertmacro MUI_HEADER_TEXT "Установки аккаунта" "Вы можете задать параметры SIP аккаунта или оставить их пустыми."
  
  ; === Label4 (type: Label) ===
  ${NSD_CreateLabel} 14u 4u 32u 50u "!"
  Pop $hCtl_account_Label4
  SendMessage $hCtl_account_Label4 ${WM_SETFONT} $hCtl_account_Font1 0
  SetCtlColors $hCtl_account_Label4 0xDC143C 0xF0F0F0
  
  ; === Label3 (type: Label) ===
  ${NSD_CreateLabel} 57u 15u 213u 34u "Внимание! Не вводите сюда параметры для входа в кабинет CLOFF! Логин и пароль SIP аккаунта необходимо получить у своего администратора или внутри кабинета CLOFF!"
  Pop $hCtl_account_Label3
  
  ; === PasswordTextBox (type: Password) ===
  ${NSD_CreatePassword} 86u 92u 183u 20u ""
  Pop $hCtl_account_PasswordTextBox
  SendMessage $hCtl_account_PasswordTextBox ${WM_SETFONT} $hCtl_account_Font2 0
  
  ; === LoginTextBox (type: Text) ===
  ${NSD_CreateText} 86u 64u 183u 20u "5111"
  Pop $hCtl_account_LoginTextBox
  SendMessage $hCtl_account_LoginTextBox ${WM_SETFONT} $hCtl_account_Font2 0
  
  ; === Label2 (type: Label) ===
  ${NSD_CreateLabel} 8u 92u 74u 17u "Password:"
  Pop $hCtl_account_Label2
  SendMessage $hCtl_account_Label2 ${WM_SETFONT} $hCtl_account_Font2 0
  
  ; === Label1 (type: Label) ===
  ${NSD_CreateLabel} 14u 64u 68u 17u "SIP Login:"
  Pop $hCtl_account_Label1
  SendMessage $hCtl_account_Label1 ${WM_SETFONT} $hCtl_account_Font3 0
  
FunctionEnd

; dialog show function
Function fnc_account_Show
	${If} $InterfaceType != "2"
	  Call fnc_account_Create
	  nsDialogs::Show
	${EndIf}
FunctionEnd

Function fnAccountLeave
	${NSD_GetText} $hCtl_account_LoginTextBox $SIPLogin
	${NSD_GetText} $hCtl_account_PasswordTextBox $SIPPassword
FunctionEnd

; handle variables
Var hCtl_install_confirm
Var hCtl_install_confirm_InterfaceLabel
Var hCtl_install_confirm_Bitmap1
Var hCtl_install_confirm_Bitmap1_hImage


; dialog create function
Function fnc_install_confirm_Create
  
  ; === install_confirm (type: Dialog) ===
  nsDialogs::Create 1018
  Pop $hCtl_install_confirm
  ${If} $hCtl_install_confirm == error
    Abort
  ${EndIf}
  !insertmacro MUI_HEADER_TEXT "Подтверждение установки" "Всё готово для установки, подтвердите своё действие."
  
  ; === InterfaceLabel (type: Label) ===
  ${NSD_CreateLabel} 68u 124u 219u 12u "Выбран интерфейс SIP телефона"
  Pop $hCtl_install_confirm_InterfaceLabel
	${NSD_AddStyle} $hCtl_install_confirm_InterfaceLabel ${SS_CENTER}
	${If} $InterfaceType == "2"
		${NSD_SetText} $hCtl_install_confirm_InterfaceLabel "Выбран интерфейс WEB кабинета CLOFF"
	${Else}
		StrLen $LoginLen $SIPLogin
		${If} $LoginLen > 0
			${NSD_SetText} $hCtl_install_confirm_InterfaceLabel "Выбран интерфейс SIP телефона. sip:$SIPLogin@cloff.ru"
		${Else}
			${NSD_SetText} $hCtl_install_confirm_InterfaceLabel "Выбран интерфейс SIP телефона"
		${EndIf}
	${EndIf}
  
  ; === Bitmap1 (type: Bitmap) ===
  ${NSD_CreateBitmap} 8u 7u 280u 112u ""
  Pop $hCtl_install_confirm_Bitmap1
  File "/oname=$PLUGINSDIR\app_promo_top.png-revHEAD.bmp" "C:\Users\Wadden\Documents\Work\cloff_sipophone\NSIS\app_promo_top.png-revHEAD.bmp"
  ${NSD_SetStretchedImage} $hCtl_install_confirm_Bitmap1 "$PLUGINSDIR\app_promo_top.png-revHEAD.bmp" $hCtl_install_confirm_Bitmap1_hImage
  
FunctionEnd

; dialog show function
Function fnc_install_confirm_Show
	Call fnc_install_confirm_Create
	nsDialogs::Show
FunctionEnd

; =========================================================
; This file was generated by NSISDialogDesigner 1.5.0.0
; https://coolsoft.altervista.org/nsisdialogdesigner
;
; Do not edit it manually, use NSISDialogDesigner instead!
; =========================================================

; handle variables
Var hCtl_confirm_cfg_delete
Var hCtl_confirm_cfg_delete_CheckBox_log
Var hCtl_confirm_cfg_delete_CheckBox_cfg
Var hCtl_confirm_cfg_delete_Font1


; dialog create function
Function un.fnc_confirm_cfg_delete_Create
  
  ; custom font definitions
  CreateFont $hCtl_confirm_cfg_delete_Font1 "Microsoft Sans Serif" "12" "400"
  
  ; === confirm_cfg_delete (type: Dialog) ===
  nsDialogs::Create 1018
  Pop $hCtl_confirm_cfg_delete
  ${If} $hCtl_confirm_cfg_delete == error
    Abort
  ${EndIf}
  !insertmacro MUI_HEADER_TEXT "Dialog title..." "Dialog subtitle..."
  
  ; === CheckBox_log (type: Checkbox) ===
  ${NSD_CreateCheckbox} 32u 78u 176u 23u "Удалить файлы логов?"
  Pop $hCtl_confirm_cfg_delete_CheckBox_log
  SendMessage $hCtl_confirm_cfg_delete_CheckBox_log ${WM_SETFONT} $hCtl_confirm_cfg_delete_Font1 0
  
  ; === CheckBox_cfg (type: Checkbox) ===
  ${NSD_CreateCheckbox} 32u 42u 176u 18u "Удалить файл настроек?"
  Pop $hCtl_confirm_cfg_delete_CheckBox_cfg
  SendMessage $hCtl_confirm_cfg_delete_CheckBox_cfg ${WM_SETFONT} $hCtl_confirm_cfg_delete_Font1 0
  
FunctionEnd

; dialog show function
Function un.fnc_confirm_cfg_delete_Show
  Call un.fnc_confirm_cfg_delete_Create
  nsDialogs::Show
FunctionEnd

Var DelCfg
Var DelLog
Function un.fnDeleteConfirm
	${NSD_GetState} $hCtl_confirm_cfg_delete_CheckBox_cfg $DelCfg
	${NSD_GetState} $hCtl_confirm_cfg_delete_CheckBox_log $DelLog

	SetShellVarContext all

	${If} $DelCfg == ${BST_CHECKED}
		Delete "$AppData\$MyAppName\*.cfg"
	${EndIf}
	${If} $DelLog == ${BST_CHECKED}
		Delete "$AppData\$MyAppName\*.log"
	${EndIf}

	${If} $DelLog == ${BST_CHECKED} 
	${AndIf} $DelCfg == ${BST_CHECKED}
		RMDir "$AppData\$MyAppName"
	${EndIf}
FunctionEnd

Function fnCloseApplication
	FindWindow $0 "CLOFF SIP phone"
	StrCmp $0 0 notRunning
	SendMessage $0 ${WM_CLOSE} 0 0
	notRunning:
FunctionEnd

Function un.fnCloseApplication
	FindWindow $0 "CLOFF SIP phone"
	StrCmp $0 0 notRunning
	SendMessage $0 ${WM_CLOSE} 0 0
	notRunning:
FunctionEnd
