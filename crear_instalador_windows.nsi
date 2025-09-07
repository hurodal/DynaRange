; Incluimos la librería para la Interfaz de Usuario Moderna
!include MUI2.nsh

; --- Información Básica del Instalador ---
Name "DynaRange"
OutFile "dynaRangeInstaller.exe"
InstallDir "$PROGRAMFILES64\DynaRange"
RequestExecutionLevel admin

; --- Información de Versión (para Propiedades del Archivo) ---
VIProductVersion "1.1.0.0"
VIAddVersionKey "FileVersion" "1.1.0.0"
VIAddVersionKey "ProductName" "DynaRange"
VIAddVersionKey "FileDescription" "Dynamic Range Analysis Tool"
VIAddVersionKey "LegalCopyright" "Hurodal"

; --- Iconos del Instalador y Desinstalador ---
; (Asegúrate de tener un archivo "favicon_noise.ico" junto a este script)
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
  
  ; Crea el desinstalador
  WriteUninstaller "$INSTDIR\uninstall.exe"
  
  ; [cite_start]Copia todos los archivos de tu carpeta portable [cite: 6, 7]
  File /r "dynaRangePortable\*.*" 
  
  ; [cite_start]Crea accesos directos (ahora con icono) [cite: 8]
  CreateShortCut "$DESKTOP\DynaRange GUI.lnk" "$INSTDIR\dynaRangeGui.exe" "" "favicon_noise.ico"
  CreateShortCut "$SMPROGRAMS\DynaRange\DynaRange GUI.lnk" "$INSTDIR\dynaRangeGui.exe" "" "favicon_noise.ico"

  ; Escribe la información para "Agregar o quitar programas" de Windows
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\DynaRange" "DisplayName" "DynaRange"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\DynaRange" "UninstallString" '"$INSTDIR\uninstall.exe"'
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\DynaRange" "NoModify" 1
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\DynaRange" "NoRepair" 1

SectionEnd

; --- Sección de Desinstalación ---
Section "Uninstall"

  ; Borra los archivos instalados
  Delete "$INSTDIR\uninstall.exe"
  RMDir /r "$INSTDIR" ; Borra la carpeta de instalación y todo su contenido

  ; Borra los accesos directos
  Delete "$DESKTOP\DynaRange GUI.lnk"
  Delete "$SMPROGRAMS\DynaRange\DynaRange GUI.lnk"
  RMDir "$SMPROGRAMS\DynaRange"

  ; Borra la información de "Agregar o quitar programas"
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\DynaRange"

SectionEnd