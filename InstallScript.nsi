; Installation Script (C) 2007-2014 RenderHeads Ltd.  All Rights Reserved.
; ________________________________________________________________________

!define     PRODUCTNAME             "UVAutoRatio 2.0 Pro"	; This is the parent product name
!define     SHORTPRODUCTNAME        "UVAutoRatio2Pro"		  ; This is the parent product name
!define     VERSIONNAME             "UVAutoRatio 2.6.0 Pro"	; All subversion (2.1 etc) go here
!define     COMPANY                 "RenderHeads"
!define     PRODUCTTYPE             "Maya Plugin"
!define     VERSION                 "2.0 Pro"
!define     SHORTVERSION            "2.6.0"
!define     INSTALLDATA             "InstallData"           ; Folder where data used for installation is kept

SetCompressor /Solid lzma

Name "${VERSIONNAME}"
Caption "${VERSIONNAME} - ${PRODUCTTYPE}"

;OutFile "Releases\${VERSIONNAME} Install.exe"
OutFile "Releases\UVAutoRatioPro-${SHORTVERSION}-Setup.exe"

Icon			"${InstallData}\icon.ico"

XPStyle on
InstallDir "$PROGRAMFILES\${COMPANY}\${PRODUCTNAME}"

; _____________________________
; Install Pages
;

PageEx license
    Caption " - License Agreement"
    LicenseText "License Agreement"
    LicenseData "${InstallData}\LicenseAgreement.rtf"
PageExEnd

PageEx directory
    Caption " - Select Install Path"
    DirVar $INSTDIR
    DirText "Select the path where you'd like to install the plugin" "Install Path" "Browse" "Browse"
    PageCallbacks "" "" CheckMayaRunning
PageExEnd


PageEx instfiles
    Caption " - Installing"
PageExEnd

PageEx license
    Caption " - Read Instructions"
    LicenseText "Launch Instructions"
    LicenseData "${InstallData}\FinalMessage.rtf"
PageExEnd


;Function validateMayaFolder
;    IfFileExists $MAYA_DIR\bin\maya.exe PathGood
;        MessageBox MB_OK "This doesn't appear to be a valid Maya installation folder.  It should look something like this: 'C:\Program Files\Autodesk\Maya2008'."
;	    Abort
;    PathGood:
;    call CheckMayaRunning 
;FunctionEnd


Function CheckMayaRunning
    FindProcDLL::FindProc "maya.exe"
    StrCmp $R0 "1" found not_found
      found:
      MessageBox MB_OK "Please close Maya before running this installation"
      Abort
      not_found:
FunctionEnd

; _____________________________
; Maya Module Operations
;

Section "Maya Module"
    ; Copy the module file into the Maya\modules folder
    ;SetOutPath $MAYA_DIR\modules
    ;SetOutPath $DOCUMENTS\maya\modules
    ;File "module\${SHORTPRODUCTNAME}WindowsModule.txt"

    ; Append the module installation path to the file
    ClearErrors
    ;FileOpen $0 $MAYA_DIR\modules\${SHORTPRODUCTNAME}WindowsModule.txt a
    CreateDirectory $DOCUMENTS\maya\modules
    FileOpen $0 $DOCUMENTS\maya\modules\${SHORTPRODUCTNAME}Module.txt w
        IfErrors done
        FileSeek $0 0 END
        FileWrite $0 "+ ${SHORTPRODUCTNAME} "
        FileWrite $0 "${SHORTVERSION} "
        FileWrite $0 $INSTDIR
    FileClose $0
    done:
SectionEnd

; ____________________________
; Program Files Operations
;

Section "Plugin Files"
  SetOutPath $INSTDIR
  File /r /x *.bundle /x *.so /x *.svn /x thumbs.db /x *.tga /x *.exe /x *.bat module\*.*
SectionEnd

; _____________________________
; Registry Operations
;

Section "Registry"
    WriteRegStr HKLM "Software\${COMPANY}\${PRODUCTNAME}" "InstallPath" $INSTDIR
    ;WriteRegStr HKLM "Software\${COMPANY}\${PRODUCTNAME}" "MayaPath" $MAYA_DIR

	; Write the uninstall keys for Windows
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${COMPANY}\${PRODUCTNAME}" "DisplayName" "${PRODUCTNAME}"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${COMPANY}\${PRODUCTNAME}" "UninstallString" '"$INSTDIR\uninstall.exe"'
    WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${COMPANY}\${PRODUCTNAME}" "NoModify" 1
    WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${COMPANY}\${PRODUCTNAME}" "NoRepair" 1
	WriteUninstaller "uninstall.exe"
	
	CreateDirectory "$APPDATA\RenderHeads\${PRODUCTNAME}"
	CreateDirectory "$SMPROGRAMS\RenderHeads\${PRODUCTNAME}"
	createShortCut "$SMPROGRAMS\RenderHeads\${PRODUCTNAME}\Uninstall.lnk" "$INSTDIR\uninstall.exe"
	createShortCut "$SMPROGRAMS\RenderHeads\${PRODUCTNAME}\Documentation.lnk" "$INSTDIR\docs\manual\index.html"
SectionEnd

; _______________________
; Uninstall
;

UninstPage uninstConfirm
UninstPage instfiles

UninstallIcon	"installdata\icon.ico"

Section "Uninstall"
	; remove maya module file
	ReadRegStr $0 HKLM "Software\${COMPANY}\${PRODUCTNAME}" "MayaPath"
	;Delete "$0\modules\${SHORTPRODUCTNAME}WindowsModule.txt"
	Delete "$DOCUMENTS\maya\modules\${SHORTPRODUCTNAME}Module.txt"

	; remove registry keys
	DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${COMPANY}\${PRODUCTNAME}"
	DeleteRegKey HKLM "Software\${COMPANY}\${PRODUCTNAME}"
	
	; remove files
	RMDir /r "$INSTDIR"
	
    ; remove the links from the start menu
    RMDir /r "$SMPROGRAMS\RenderHeads\${PRODUCTNAME}"
    ; remove the company folder only if empty
    RMDir "$SMPROGRAMS\RenderHeads"
SectionEnd