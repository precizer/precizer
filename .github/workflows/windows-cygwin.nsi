!include FileFunc.nsh
!insertmacro GetParameters

!ifndef OUTPUT_EXE
!error "OUTPUT_EXE is required"
!endif

!ifndef STAGING_DIR
!error "STAGING_DIR is required"
!endif

Unicode true
Name "Precizer"
OutFile "${OUTPUT_EXE}"
RequestExecutionLevel user
SilentInstall silent
AutoCloseWindow true
ShowInstDetails nevershow

Section
	InitPluginsDir
	SetOutPath "$PLUGINSDIR\precizer"
	File /r "${STAGING_DIR}\*.*"

	${GetParameters} $R0
	ClearErrors
	ExecWait '"$PLUGINSDIR\precizer\precizer.exe" $R0' $0
	IfErrors exec_failed

	SetErrorLevel $0
	Quit

exec_failed:
	SetErrorLevel 1
	Quit
SectionEnd
