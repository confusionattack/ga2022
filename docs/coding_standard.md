# Coding Standard

+ Functions and variables should be named using `snake_case`.
  All words in lower case, separated by underscores.

+ Avoid Win32 specific types in headers. Avoid `windows.h` in engine headers.

+ Document all functions and types defined in engine headers.

+ Avoid global and module-level variables.

+ All memory will be allocated using `heap_t`.
