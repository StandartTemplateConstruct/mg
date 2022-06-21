@echo off

set MAKE=nmake
set MAKE_OPTIONS=/f

if "%DEBUG%" == "" (
  set MAKE_VARS=DEBUG=0
) else (
  set MAKE_VARS=DEBUG=%DEBUG%
)

if ""%1"" == """" goto all
if ""%1"" == ""all"" goto go
if ""%1"" == ""install"" goto go
if ""%1"" == ""clean"" goto go

:unknown
	echo Error: Unrecognized argument %1.
	goto done

:go
	if ""%2"" == ""javaonly"" goto java
	if ""%2"" == ""nojava"" goto nojava

:all
	cd lib
	%MAKE% %MAKE_OPTIONS% win32.mak %1 %MAKE_VARS% 
	cd ..

	cd src\images
	%MAKE% %MAKE_OPTIONS% win32.mak %1 %MAKE_VARS% 
	cd ..\..

	cd src\text
	%MAKE% %MAKE_OPTIONS% win32.mak %1 %MAKE_VARS% 
	cd ..\..

	cd java\org\greenstone\mg
	call winMake.bat %1
	cd ..\..\..\..

	cd jni
	%MAKE% %MAKE_OPTIONS% win32.mak %1 %MAKE_VARS% 
	cd ..
	goto done
:java
	cd java\org\greenstone\mg
	call winMake.bat %1
	cd ..\..\..\..
	goto done

:nojava
	cd lib
	%MAKE% %MAKE_OPTIONS% win32.mak %1 %MAKE_VARS% 
	cd ..

	cd src\images
	%MAKE% %MAKE_OPTIONS% win32.mak %1 %MAKE_VARS% 
	cd ..\..

	cd src\text
	%MAKE% %MAKE_OPTIONS% win32.mak %1 %MAKE_VARS% 
	cd ..\..
	goto done
:done


