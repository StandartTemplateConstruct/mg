@echo off

set MGHOME=..\..\..\..

rem ---- Where to put class files ----
set JAVACLASSDIR=classes

rem ---- Name and location of java programs ----
set JAVAC="%JAVA_HOME%\bin\javac"
set JAVAH="%JAVA_HOME%\bin\javah"
set JAVA="%JAVA_HOME%\bin\java"
set JAVADOC="%JAVA_HOME%\bin\javadoc"
set JAR="%JAVA_HOME%\bin\jar"

set JAVACOPTIONS= -deprecation -g -O

if "%DEBUG%" == "" (
  set MAKE_VARS=DEBUG=0
) else (
  set MAKE_VARS=DEBUG=%DEBUG%
)


if ""%1"" == """" goto all
if ""%1"" == ""all"" goto all
if ""%1"" == ""install"" goto install
if ""%1"" == ""clean"" goto clean

:unknown
	echo Error: Unrecognized argument %1.
	goto done

:all
	if not exist %JAVACLASSDIR% mkdir %JAVACLASSDIR%
	echo Compiling MG Java classes ...
        %JAVAC% -d %JAVACLASSDIR% %JAVACOPTIONS% *.java
	%JAVAH% -classpath %JAVACLASSDIR% -d %MGHOME%\jni org.greenstone.mg.MGWrapper
	%JAVAH% -classpath %JAVACLASSDIR% -d %MGHOME%\jni org.greenstone.mg.MGPassesWrapper
	%JAR% cf %MGHOME%\mg.jar -C %JAVACLASSDIR% org
	goto done

:install
	goto done

:clean
	echo Cleaning up...
	if exist %JAVACLASSDIR% rmdir /S /Q %JAVACLASSDIR%
        if exist %MGHOME%\mg.jar del %MGHOME%\mg.jar
        if exist %MGHOME%\jni\org_greenstone_mg_MGWrapper.h del %MGHOME%\jni\org_greenstone_mg_MGWrapper.h
        if exist %MGHOME%\jni\org_greenstone_mg_MGPassesWrapper.h del %MGHOME%\jni\org_greenstone_mg_MGPassesWrapper.h
	goto done

:done
