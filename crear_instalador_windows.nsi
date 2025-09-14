!include MUI2.nsh

; --- Información Básica del Instalador ---
Name "DynaRange"
; MODIFICADO: Nombre del instalador consistente con el CLI
OutFile "dynaRangeInstaller.exe" 
InstallDir "$PROGRAMFILES64\DynaRange"
RequestExecutionLevel admin

; --- Información de Versión (para Propiedades del Archivo) ---
VIProductVersion "1.2.0.0"
VIAddVersionKey "FileVersion" "1.2.0.0"
VIAddVersionKey "ProductName" "DynaRange"
VIAddVersionKey "FileDescription" "Dynamic Range Analysis Tool"
VIAddVersionKey "LegalCopyright" "Hurodal,JmFont,GLuijk"

; --- Iconos del Instalador y Desinstalador ---
!define MUI_ICON "favicon_noise.ico"
!define MUI_UNICON "favicon_noise.ico"

; --- Páginas del Asistente ---
!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

; --- Páginas del Desinstalador ---
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

; --- Idiomas ---
!insertmacro MUI_LANGUAGE "Spanish"


; --- Sección de Instalación ---
Section "Programa Principal"

  SetOutPath $INSTDIR
  
  WriteUninstaller "$INSTDIR\uninstall.exe"
  
  ; Copia todos los archivos de tu carpeta portable
  File /r "dynaRangePortable\*.*" 
  
  ; Crea accesos directos
  ; El icono ahora se extrae del propio .exe (índice 0, el primero)
  CreateShortCut "$DESKTOP\DynaRange GUI.lnk" "$INSTDIR\dynaRangeGui.exe" "" "$INSTDIR\dynaRangeGui.exe" 0
  CreateShortCut "$SMPROGRAMS\DynaRange\DynaRange GUI.lnk" "$INSTDIR\dynaRangeGui.exe" "" "$INSTDIR\dynaRangeGui.exe" 0  
  ; AÑADIDO (Opcional): Acceso directo para el CLI en el menú de inicio
  CreateShortCut "$SMPROGRAMS\DynaRange\rango.lnk" "$INSTDIR\rango.exe"

  ; Escribe la información para "Agregar o quitar programas"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\DynaRange" "DisplayName" "DynaRange"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\DynaRange" "UninstallString" '"$INSTDIR\uninstall.exe"'
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\DynaRange" "NoModify" 1
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\DynaRange" "NoRepair" 1

SectionEnd

; --- Sección de Desinstalación ---
Section "Uninstall"

  Delete "$INSTDIR\uninstall.exe"
  RMDir /r "$INSTDIR"

  ; Borra los accesos directos
  Delete "$DESKTOP\DynaRange GUI.lnk"
  Delete "$SMPROGRAMS\DynaRange\DynaRange GUI.lnk"
  Delete "$SMPROGRAMS\DynaRange\rango.lnk" ; Limpieza del nuevo acceso directo
  RMDir "$SMPROGRAMS\DynaRange"

  ; Borra la información de "Agregar o quitar programas"
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\DynaRange"

SectionEnd