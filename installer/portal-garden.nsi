; Original HALVETH installer code: MIT. Payload licenses are kept beside the game.
; Compile only through scripts/package_installer.ps1, which generates an exact file list.
Unicode true
RequestExecutionLevel user
ManifestDPIAware true
CRCCheck on
SetCompressor /SOLID lzma
SetCompressorDictSize 32
SetDatablockOptimize on

!include "MUI2.nsh"
!include "LogicLib.nsh"

!ifndef PRODUCT_VERSION
  !define PRODUCT_VERSION "0.1.0"
!endif
!ifndef PROJECT_ROOT
  !error "PROJECT_ROOT is required; use scripts/package_installer.ps1."
!endif
!ifndef PACKAGE_ROOT
  !error "PACKAGE_ROOT is required."
!endif
!ifndef GENERATED_DIR
  !error "GENERATED_DIR is required."
!endif
!ifndef OUTPUT_EXE
  !error "OUTPUT_EXE is required."
!endif

!define PRODUCT_NAME "HALVETH Portal Garden"
!define UNINSTALL_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\HALVETH.PortalGarden.${PRODUCT_VERSION}"
Name "${PRODUCT_NAME} ${PRODUCT_VERSION}"
OutFile "${OUTPUT_EXE}"
InstallDir "$LOCALAPPDATA\HALVETH\PortalGarden\${PRODUCT_VERSION}"
BrandingText "HALVETH - Portal Garden"
Icon "${PROJECT_ROOT}\Build\Windows\Application.ico"
UninstallIcon "${PROJECT_ROOT}\Build\Windows\Application.ico"
ShowInstDetails show
ShowUninstDetails show
VIProductVersion "${PRODUCT_VERSION}.0"
VIAddVersionKey /LANG=1033 "ProductName" "${PRODUCT_NAME}"
VIAddVersionKey /LANG=1033 "FileDescription" "HALVETH Portal Garden per-user installer"
VIAddVersionKey /LANG=1033 "FileVersion" "${PRODUCT_VERSION}"
VIAddVersionKey /LANG=1033 "ProductVersion" "${PRODUCT_VERSION}"
VIAddVersionKey /LANG=1033 "LegalCopyright" "2026 HALVETH contributors"

!define MUI_ABORTWARNING
!define MUI_ICON "${PROJECT_ROOT}\Build\Windows\Application.ico"
!define MUI_UNICON "${PROJECT_ROOT}\Build\Windows\Application.ico"
!define MUI_WELCOMEPAGE_TITLE "Welcome to HALVETH Portal Garden"
!define MUI_WELCOMEPAGE_TEXT "Explore four connected worlds, practise magic and meet the garden's guides.$\r$\n$\r$\nThis prototype installs for your Windows user. Your saved settings and game data are retained when you uninstall."
!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "${PACKAGE_ROOT}\THIRD-PARTY-NOTICES.md"
!insertmacro MUI_PAGE_INSTFILES
!define MUI_FINISHPAGE_RUN
!define MUI_FINISHPAGE_RUN_FUNCTION LaunchGarden
!define MUI_FINISHPAGE_RUN_TEXT "Launch HALVETH Portal Garden"
!define MUI_FINISHPAGE_RUN_NOTCHECKED
!define MUI_FINISHPAGE_TEXT "Portal Garden is installed. Use the desktop or Start menu shortcut to play.$\r$\n$\r$\nSave folders and user-created files are preserved by the uninstaller."
!insertmacro MUI_PAGE_FINISH
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_UNPAGE_FINISH
!insertmacro MUI_LANGUAGE "English"

; Known installation directories may not redirect operations through a junction.
!macro AssertPlainDirectory DIRECTORY
  System::Call 'kernel32::GetFileAttributesW(w "${DIRECTORY}") i .r0'
  ${If} $0 != -1
    IntOp $1 $0 & 0x400
    ${If} $1 != 0
      MessageBox MB_ICONSTOP "An installation entry is a filesystem link. No files were changed: ${DIRECTORY}"
      Abort
    ${EndIf}
  ${EndIf}
!macroend

!include "${GENERATED_DIR}\payload.nsh"

Function .onInit
  SetShellVarContext current
  SetRegView 64
  ; No directory page: this package has one per-user, versioned destination.
  StrCpy $INSTDIR "$LOCALAPPDATA\HALVETH\PortalGarden\${PRODUCT_VERSION}"
FunctionEnd

Function LaunchGarden
  SetOutPath "$INSTDIR"
  Exec '"$INSTDIR\HALVETHRealms.exe"'
FunctionEnd

Section "Portal Garden" SEC_MAIN
  SetShellVarContext current
  SetRegView 64
  StrCpy $INSTDIR "$LOCALAPPDATA\HALVETH\PortalGarden\${PRODUCT_VERSION}"
  !insertmacro AssertPlainDirectory "$LOCALAPPDATA\HALVETH"
  !insertmacro AssertPlainDirectory "$LOCALAPPDATA\HALVETH\PortalGarden"
  !insertmacro AssertPlainDirectory "$INSTDIR"
  !insertmacro AssertPayloadPaths
  !insertmacro AssertPlainDirectory "$INSTDIR\Application.ico"
  !insertmacro AssertPlainDirectory "$INSTDIR\install-manifest.json"
  !insertmacro AssertPlainDirectory "$INSTDIR\Uninstall.exe"
  SetOverwrite on
  !insertmacro InstallPayload
  SetOutPath "$INSTDIR"
  File /oname=Application.ico "${PROJECT_ROOT}\Build\Windows\Application.ico"
  File /oname=install-manifest.json "${GENERATED_DIR}\install-manifest.json"
  WriteUninstaller "$INSTDIR\Uninstall.exe"
  CreateDirectory "$SMPROGRAMS\HALVETH Portal Garden"
  CreateShortcut "$DESKTOP\HALVETH Portal Garden.lnk" "$INSTDIR\HALVETHRealms.exe" "" "$INSTDIR\Application.ico"
  CreateShortcut "$SMPROGRAMS\HALVETH Portal Garden\Play.lnk" "$INSTDIR\HALVETHRealms.exe" "" "$INSTDIR\Application.ico"
  CreateShortcut "$SMPROGRAMS\HALVETH Portal Garden\Uninstall.lnk" "$INSTDIR\Uninstall.exe"
  WriteRegStr HKCU "${UNINSTALL_KEY}" "DisplayName" "${PRODUCT_NAME} ${PRODUCT_VERSION}"
  WriteRegStr HKCU "${UNINSTALL_KEY}" "DisplayVersion" "${PRODUCT_VERSION}"
  WriteRegStr HKCU "${UNINSTALL_KEY}" "Publisher" "HALVETH contributors"
  WriteRegStr HKCU "${UNINSTALL_KEY}" "InstallLocation" "$INSTDIR"
  WriteRegStr HKCU "${UNINSTALL_KEY}" "DisplayIcon" "$INSTDIR\Application.ico"
  WriteRegStr HKCU "${UNINSTALL_KEY}" "UninstallString" '$\"$INSTDIR\Uninstall.exe$\"'
  WriteRegStr HKCU "${UNINSTALL_KEY}" "QuietUninstallString" '$\"$INSTDIR\Uninstall.exe$\" /S'
  WriteRegDWORD HKCU "${UNINSTALL_KEY}" "NoModify" 1
  WriteRegDWORD HKCU "${UNINSTALL_KEY}" "NoRepair" 1
  WriteRegDWORD HKCU "${UNINSTALL_KEY}" "EstimatedSize" ${PAYLOAD_KIB}
SectionEnd

Function un.onInit
  SetShellVarContext current
  SetRegView 64
  GetFullPathName $0 "$LOCALAPPDATA\HALVETH\PortalGarden\${PRODUCT_VERSION}"
  GetFullPathName $1 "$INSTDIR"
  ${If} $0 != $1
    MessageBox MB_ICONSTOP "The uninstaller is outside its original versioned installation. No files were removed."
    Abort
  ${EndIf}
FunctionEnd

Section "Uninstall"
  SetShellVarContext current
  SetRegView 64
  !insertmacro AssertPlainDirectory "$LOCALAPPDATA\HALVETH"
  !insertmacro AssertPlainDirectory "$LOCALAPPDATA\HALVETH\PortalGarden"
  !insertmacro AssertPlainDirectory "$INSTDIR"
  !insertmacro AssertPayloadPaths
  !insertmacro AssertPlainDirectory "$INSTDIR\Application.ico"
  !insertmacro AssertPlainDirectory "$INSTDIR\install-manifest.json"
  !insertmacro AssertPlainDirectory "$INSTDIR\Uninstall.exe"
  ; This macro contains exact compiled file names, not a mutable runtime wildcard.
  ; RMDir has no /r flag. Saved folders and unknown files keep their directories.
  !insertmacro RemovePayload
  Delete "$INSTDIR\Application.ico"
  Delete "$INSTDIR\install-manifest.json"
  Delete "$INSTDIR\Uninstall.exe"
  Delete "$DESKTOP\HALVETH Portal Garden.lnk"
  Delete "$SMPROGRAMS\HALVETH Portal Garden\Play.lnk"
  Delete "$SMPROGRAMS\HALVETH Portal Garden\Uninstall.lnk"
  RMDir "$SMPROGRAMS\HALVETH Portal Garden"
  DeleteRegKey HKCU "${UNINSTALL_KEY}"
  RMDir "$INSTDIR"
  ; Per-user Unreal Saved folders outside the install root are never addressed.
SectionEnd
