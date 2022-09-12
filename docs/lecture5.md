# Lecture 5: Debugging

## Agenda

+ Implementing console logging
+ Breakpoints, conditional breakpoints, data breakpoints
+ Data inspection/editing, array inspection
+ Crash dumps
+ Hardware and software exceptions
+ Exception handling, stack unwinding
+ Implementing a crash handler
+ Compilers, linkers, and debug information
+ Homework: Leak detection and reporting

### Bonus Agenda

+ Sanitizers and static checking
+ GDB basics

## Homework 1

Win32 functions you might want:

+ CaptureStackBackTrace
+ SymSetOptions
+ SymInitialize
+ SymGetSymFromAddr64
+ SymGetLineFromAddr64
+ SymCleanup

## Links

+ https://docs.microsoft.com/en-us/cpp/cpp/hardware-exceptions
+ https://docs.microsoft.com/en-us/windows/win32/api/errhandlingapi/
+ https://darkdust.net/files/GDB%20Cheat%20Sheet.pdf
+ https://docs.microsoft.com/en-us/windows/win32/debug/debug-help-library
+ https://docs.microsoft.com/en-us/visualstudio/debugger/debug-interface-access/debug-interface-access-sdk
+ https://dwarfstd.org/
+ https://clang.llvm.org/docs/AddressSanitizer.html
+ https://clang.llvm.org/docs/UndefinedBehaviorSanitizer.html
+ https://docs.microsoft.com/en-us/cpp/build/reference/analyze-code-analysis
+ https://valgrind.org/
