!include "MUI2.nsh"

!define APP_NAME "PlugPatch"
!define APP_VERSION "1.0"
!define INSTALL_DIR "$PROGRAMFILES64\${APP_NAME}"
!define MUI_ICON plugpatchicon.ico
!define MUI_UNICON plugpatchicon.ico

OutFile "PlugPatchInstaller.exe"
InstallDir "${INSTALL_DIR}"

Name "PlugPatch"

; Set the icon for the installer
Icon plugpatchicon.ico

; Variables for user choices
Var SHORTCUT_STARTMENU
Var SHORTCUT_DESKTOP

; Define installer pages
!insertmacro MUI_PAGE_DIRECTORY
Page custom CustomOptions CustomOptionsLeave
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_UNPAGE_INSTFILES

; Installer settings
!insertmacro MUI_LANGUAGE "English"

Section "Install"
    SetOutPath "$INSTDIR"
    File "${OUTPUT_DIR}\PlugPatch.exe"
    File "${OUTPUT_DIR}\count.json5"
    File "${OUTPUT_DIR}\graph.json"
    File "${OUTPUT_DIR}\graph1.json"
    File "${OUTPUT_DIR}\graph2.json"
    File "${OUTPUT_DIR}\graph3.json"
    File "${OUTPUT_DIR}\graph4.json5"
    File "plugpatchicon.ico"

    ; Write the uninstaller
    WriteUninstaller "$INSTDIR\uninstall.exe"

    ; Create shortcuts based on user selections
    StrCmp $SHORTCUT_STARTMENU "1" 0 +2
        CreateShortcut "$SMPROGRAMS\${APP_NAME}\${APP_NAME}.lnk" "$INSTDIR\PlugPatch.exe" "" "$INSTDIR\plugpatchicon.ico" 0

    StrCmp $SHORTCUT_DESKTOP "1" 0 +2
        CreateShortcut "$DESKTOP\${APP_NAME}.lnk" "$INSTDIR\PlugPatch.exe" "" "$INSTDIR\plugpatchicon.ico" 0

    ; Create Start Menu uninstaller shortcut
    CreateShortcut "$SMPROGRAMS\${APP_NAME}\Uninstall ${APP_NAME}.lnk" "$INSTDIR\uninstall.exe" "" "$INSTDIR\plugpatchicon.ico" 0
SectionEnd

Section "Uninstall"
    Delete "$INSTDIR\PlugPatch.exe"
    Delete "$INSTDIR\graph.json"
    Delete "$INSTDIR\graph1.json"
    Delete "$INSTDIR\graph2.json"
    Delete "$INSTDIR\graph3.json"
    Delete "$INSTDIR\graph4.json5"
    Delete "$INSTDIR\count.json5"
    Delete "$INSTDIR\plugpatchicon.ico"
    Delete "$SMPROGRAMS\${APP_NAME}\${APP_NAME}.lnk"
    Delete "$SMPROGRAMS\${APP_NAME}\Uninstall ${APP_NAME}.lnk"
    Delete "$DESKTOP\${APP_NAME}.lnk"
    Delete "$INSTDIR\uninstall.exe"
    RMDir "$SMPROGRAMS\${APP_NAME}"
    RMDir "$INSTDIR"
SectionEnd

; Custom options page for shortcuts
Function CustomOptions
    nsDialogs::Create /NOUNLOAD 1018
    Pop $R0
    ${If} $R0 == error
        Abort
    ${EndIf}

    ; Checkbox for Start Menu shortcut
    ${NSD_CreateCheckbox} 10u 30u 100% 12u "Create Start Menu Shortcut"
    Pop $SHORTCUT_STARTMENU
    ${NSD_Check} $SHORTCUT_STARTMENU
    ${NSD_SetState} $SHORTCUT_STARTMENU ${BST_CHECKED}

    ; Checkbox for Desktop shortcut
    ${NSD_CreateCheckbox} 10u 50u 100% 12u "Create Desktop Shortcut"
    Pop $SHORTCUT_DESKTOP
    ${NSD_Check} $SHORTCUT_DESKTOP
    ${NSD_SetState} $SHORTCUT_DESKTOP ${BST_CHECKED}

    nsDialogs::Show
FunctionEnd

Function CustomOptionsLeave
    ; Read the state of the checkboxes
    ${NSD_GetState} $SHORTCUT_STARTMENU $SHORTCUT_STARTMENU
    ${NSD_GetState} $SHORTCUT_DESKTOP $SHORTCUT_DESKTOP
FunctionEnd
