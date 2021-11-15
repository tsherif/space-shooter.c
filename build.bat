@if "%~1" == "release" (
	set arg1=release
) else (
	set arg1=debug
)
@set shared_args=/std:c11 /W3 /WX /wd5105 /D SOGL_MAJOR_VERSION=3 /D SOGL_MINOR_VERSION=3 /Fespace-shooter
@if %arg1%==release (
	@set build_args=/O2
) else (
	@set build_args=/Zi /D SPACE_SHOOTER_DEBUG
)
@rmdir /s /q build
@mkdir build
@xcopy /S /y assets build\assets\
@pushd build
@REM /wd5105 suppressed because it causes an error in winbase.h
cl %shared_args% %build_args% ..\src\shared\*.c ..\src\game\*.c ..\src\platform\windows\*.c user32.lib gdi32.lib opengl32.lib xinput.lib ole32.lib
@popd
 
