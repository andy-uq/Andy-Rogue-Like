@cd build
@cl ../test/test.c ../memory.c ../collection.c ../string.c /I..
@IF ERRORLEVEL 1 GOTO errorHandling
test.exe
:errorHandling
@cd ..
