@if "%~1" == "release" (
	set arg1=release
) else (
	set arg1=debug
)
@REM Warnings suppressed 5105/4100 because they're triggered by windows code, 
@REM 4201 because it doesn't like anonymous structs for some reason.
@set shared_args=/std:c11 /W4 /WX /wd5105 /wd4201 /wd4100 /D SOGL_MAJOR_VERSION=3 /D SOGL_MINOR_VERSION=3 /Fespace-shooter
@if %arg1%==release (
	@set build_args=/O2
) else (
	@set build_args=/Zi /RTCcsu /D SPACE_SHOOTER_DEBUG
)
@rmdir /s /q build
@mkdir build
@xcopy /S /y assets build\assets\
@pushd build
cl %shared_args% %build_args% ..\src\shared\*.c ..\src\game\*.c ..\src\platform\windows\*.c user32.lib gdi32.lib opengl32.lib xinput.lib ole32.lib winmm.lib
@popd
 
