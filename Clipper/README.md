To compile the c++ file: (Linux)

```less
x86_64-w64-mingw32-g++ -static -O2 -o update.exe update.cpp -lwininet -lcrypt32 -lshell32 -ladvapi32 -luser32 -mwindows
```
