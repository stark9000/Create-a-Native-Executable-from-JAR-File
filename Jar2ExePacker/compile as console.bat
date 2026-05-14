set JAVA_HOME=C:\Program Files\Java\jdk1.8.0_162

gcc -I"%JAVA_HOME%\include" -I"%JAVA_HOME%\include\win32" -o MyAppE.exe output.c -L"%JAVA_HOME%\lib" -ljvm

pause