!include "MUI2.nsh"

!define APP_NAME "PatchformStandalone"
!define APP_VERSION "1.0"
!define INSTALL_DIR "$PROGRAMFILES64\${APP_NAME}"
!define MUI_ICON patchformicon.ico
!define MUI_UNICON patchformicon.ico
!define MUI_WELCOMEFINISHPAGE_BITMAP "patchformside.bmp"

OutFile "PatchformStandaloneInstaller.exe"
InstallDir "${INSTALL_DIR}"

Name "Patchform"

; Set the icon for the installer
Icon patchformicon.ico

; Variables for user choices
Var SHORTCUT_STARTMENU
Var SHORTCUT_DESKTOP

; Define welcome image page
!insertmacro MUI_PAGE_WELCOME
; Define license page
!insertmacro MUI_PAGE_LICENSE "LICENSE.txt"
; Define installer pages
!insertmacro MUI_PAGE_DIRECTORY
Page custom CustomOptions CustomOptionsLeave
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_UNPAGE_INSTFILES

; Installer settings
!insertmacro MUI_LANGUAGE "English"

!define PATCH_DIR "${OUTPUT_DIR}\Patches"
!define ASSET_DIR "${OUTPUT_DIR}\Assets"

Section "Install"
    SetOutPath "$INSTDIR"
    File "${OUTPUT_DIR}\PatchformStandalone.exe"
    File "patchformicon.ico"
    File "patchformside.bmp"

    SetOutPath "$INSTDIR\Patches"
    File "${PATCH_DIR}\count.json5"
    File "${PATCH_DIR}\graph.json"
    File "${PATCH_DIR}\graph1.json"
    File "${PATCH_DIR}\graph2.json"
    File "${PATCH_DIR}\graph3.json"
    File "${PATCH_DIR}\graph4.json5"
    File "${PATCH_DIR}\WavetableMovement.json"

    SetOutPath "$INSTDIR\Assets\Fonts"
    File "${ASSET_DIR}\Fonts\Inter_18pt-Regular.ttf"
    File "${ASSET_DIR}\Fonts\Inter_18pt-SemiBold.ttf"

    SetOutPath "$INSTDIR\Assets\Icons"
    File "${ASSET_DIR}\Icons\IconFontPlugPatch.ttf"
    File "${ASSET_DIR}\Icons\ObjectIconFont.ttf"

    SetOutPath "$INSTDIR"

    ; Write the uninstaller
    WriteUninstaller "$INSTDIR\uninstall.exe"

    ; Create shortcuts based on user selections
    StrCmp $SHORTCUT_STARTMENU "1" 0 +2
        CreateShortcut "$SMPROGRAMS\${APP_NAME}\${APP_NAME}.lnk" "$INSTDIR\PatchformStandalone.exe" "" "$INSTDIR\patchformicon.ico" 0

    StrCmp $SHORTCUT_DESKTOP "1" 0 +2
        CreateShortcut "$DESKTOP\${APP_NAME}.lnk" "$INSTDIR\PatchformStandalone.exe" "" "$INSTDIR\patchformicon.ico" 0

    ; Create Start Menu uninstaller shortcut
    CreateShortcut "$SMPROGRAMS\${APP_NAME}\Uninstall ${APP_NAME}.lnk" "$INSTDIR\uninstall.exe" "" "$INSTDIR\patchformicon.ico" 0
SectionEnd

Section "Uninstall"
    Delete "$INSTDIR\PatchformStandalone.exe"
    Delete "$INSTDIR\patchformicon.ico"
    Delete "$INSTDIR\uninstall.exe"

    ; Delete patch files
    Delete "$INSTDIR\Patches\*.json"
    Delete "$INSTDIR\Patches\*.json5"
    RMDir "$INSTDIR\Patches"

    ; Delete fonts
    Delete "$INSTDIR\Assets\Fonts\*.ttf"
    RMDir "$INSTDIR\Assets\Fonts"

    ; Delete icons
    Delete "$INSTDIR\Assets\Icons\*.ttf"
    RMDir "$INSTDIR\Assets\Icons"

    ; Delete assets folder
    RMDir "$INSTDIR\Assets"

    ; Delete shortcuts
    Delete "$SMPROGRAMS\${APP_NAME}\${APP_NAME}.lnk"
    Delete "$SMPROGRAMS\${APP_NAME}\Uninstall ${APP_NAME}.lnk"
    Delete "$DESKTOP\${APP_NAME}.lnk"

    ; Clean up folders
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
