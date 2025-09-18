!include MUI2.nsh
!include LogicLib.nsh

; --- Información Básica del Instalador ---
Name "DynaRange"
OutFile "dynaRangeInstaller.exe" 
InstallDir "$PROGRAMFILES\DynaRange"  ; ✅ ¡CAMBIO CLAVE! Usa $PROGRAMFILES, no $PROGRAMFILES64
RequestExecutionLevel admin

; --- Información de Versión ---
VIProductVersion "1.2.0.0"
VIAddVersionKey "FileVersion" "1.2.0.0"
VIAddVersionKey "ProductName" "DynaRange"
VIAddVersionKey "FileDescription" "Dynamic Range Analysis Tool"
VIAddVersionKey "LegalCopyright" "Hurodal,JmFont,GLuijk"

; --- Iconos ---
!define MUI_ICON "favicon_noise.ico"
!define MUI_UNICON "favicon_noise.ico"

; --- Páginas ---
!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_LANGUAGE "Spanish"


; --- Función para añadir $INSTDIR al PATH del sistema ---
Function AddToPath
  Exch $0
  Push $1
  Push $2
  Push $3
  Push $4
  Push $5

  ReadRegStr $1 HKLM "SYSTEM\CurrentControlSet\Control\Session Manager\Environment" "PATH"
  StrCmp $1 "" +2
    StrCpy $2 $1
    Goto +2
  StrCpy $2 ""

  StrLen $4 $2
  IntOp $4 $4 - 1
  StrCpy $3 $2 1 $4
  StrCmp $3 ";" +2
    StrCpy $2 "$2;"

  StrCpy $4 $0
  StrLen $4 $4
  StrCpy $5 ""
  StrCpy $3 0

LoopCheck:
  StrCpy $5 $2 $4 $3
  StrCmp $5 $0 FoundDuplicate
  IntOp $3 $3 + 1
  StrLen $4 $2
  IntCmp $3 $4 +2 LoopCheck
  Goto NotDuplicate

FoundDuplicate:
  Pop $5
  Pop $4
  Pop $3
  Pop $2
  Pop $1
  Pop $0
  Return

NotDuplicate:
  StrCpy $2 "$2$0"
  WriteRegStr HKLM "SYSTEM\CurrentControlSet\Control\Session Manager\Environment" "PATH" "$2"
  SendMessage ${HWND_BROADCAST} ${WM_SETTINGCHANGE} 0 "STR:Environment" /TIMEOUT=5000

  Pop $5
  Pop $4
  Pop $3
  Pop $2
  Pop $1
  Pop $0
FunctionEnd


; --- Función para eliminar $INSTDIR del PATH ---
Function un.RemoveFromPath
  Exch $0
  Push $1
  Push $2
  Push $3
  Push $4
  Push $5
  Push $6
  Push $7

  ReadRegStr $1 HKLM "SYSTEM\CurrentControlSet\Control\Session Manager\Environment" "PATH"
  StrCmp $1 "" Done

  StrCpy $2 ""
  StrCpy $6 0
  StrLen $4 $0

LoopSplit:
  StrCpy $3 $1 1 $6
  StrCmp $3 "" EndLoop

  StrCmp $3 ";" CheckSegment
  StrCpy $2 "$2$3"
  IntOp $6 $6 + 1
  Goto LoopSplit

CheckSegment:
  StrCpy $5 $2
  StrCmp $5 $0 SkipAdd
  StrCpy $2 "$2;"
  IntOp $6 $6 + 1
  Goto LoopSplit

SkipAdd:
  IntOp $6 $6 + 1
  Goto LoopSplit

EndLoop:
  StrLen $4 $2
  IntOp $4 $4 - 1
  StrCpy $3 $2 1 $4
  StrCmp $3 ";" +2
    StrCpy $2 "$2;"
  StrCmp $2 "" Done
  WriteRegStr HKLM "SYSTEM\CurrentControlSet\Control\Session Manager\Environment" "PATH" "$2"
  SendMessage ${HWND_BROADCAST} ${WM_SETTINGCHANGE} 0 "STR:Environment" /TIMEOUT=5000

Done:
  Pop $7
  Pop $6
  Pop $5
  Pop $4
  Pop $3
  Pop $2
  Pop $1
  Pop $0
FunctionEnd


; --- Sección de Instalación ---
Section "Programa Principal"
  SetOutPath $INSTDIR
  WriteUninstaller "$INSTDIR\uninstall.exe"
  File /r "dynaRangePortable\*.*"

  CreateShortCut "$DESKTOP\DynaRange GUI.lnk" "$INSTDIR\dynaRangeGui.exe" "" "$INSTDIR\dynaRangeGui.exe" 0
  CreateShortCut "$SMPROGRAMS\DynaRange\DynaRange GUI.lnk" "$INSTDIR\dynaRangeGui.exe" "" "$INSTDIR\dynaRangeGui.exe" 0
  CreateShortCut "$SMPROGRAMS\DynaRange\rango.lnk" "$INSTDIR\rango.exe"

  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\DynaRange" "DisplayName" "DynaRange"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\DynaRange" "UninstallString" '"$INSTDIR\uninstall.exe"'
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\DynaRange" "NoModify" 1
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\DynaRange" "NoRepair" 1

  ; ✅ AÑADIR AL PATH (ahora $INSTDIR es C:\Program Files\DynaRange, no una cadena literal)
  Push "$INSTDIR"
  Call AddToPath

  MessageBox MB_OK "Instalación completada.\n\n'rango' ahora está disponible desde cualquier terminal.\nCierra y abre una nueva consola para probarlo."
SectionEnd


; --- Sección de Desinstalación ---
Section "Uninstall"
  Delete "$INSTDIR\uninstall.exe"
  RMDir /r "$INSTDIR"

  Delete "$DESKTOP\DynaRange GUI.lnk"
  Delete "$SMPROGRAMS\DynaRange\DynaRange GUI.lnk"
  Delete "$SMPROGRAMS\DynaRange\rango.lnk"
  RMDir "$SMPROGRAMS\DynaRange"

  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\DynaRange"

  Push "$INSTDIR"
  Call un.RemoveFromPath
SectionEnd