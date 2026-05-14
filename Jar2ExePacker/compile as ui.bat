set JAVA_HOME=C:\Program Files\Java\jdk1.8.0_162

gcc -mwindows -o MyAppE.exe output.c -I"%JAVA_HOME%\include" -I"%JAVA_HOME%\include\win32" -L"%JAVA_HOME%\jre\bin\server" -ljvm

pause