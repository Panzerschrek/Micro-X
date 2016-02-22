
set model_name=robot
set skip_normals=--skip-normals
call :Convert

set model_name=pyramid_robot
set skip_normals=--skip-normals
call :Convert

set model_name=cube
set skip_normals=--skip-normals
call :Convert

set model_name=icosahedron
set skip_normals=--skip-normals
call :Convert

PAUSE

:Convert
setlocal
	"./obj2mxmd_converter.exe" models/%model_name%.obj %skip_normals%
	"./bin2ccode.exe" models/%model_name%.mxmd -o models/%model_name%.h
endlocal
goto :eof