### System2

`System2` is a cross-platform c library that allows you to call shell commands, just like `system` but with the ability to
provide input to stdin and capture the output from stdout and stderr.

```text
  _______________________________
/_______________________________/|
|o   ____  _  _  ____  ____   o|$|
|   / ___)( \/ )/ ___)(___ \   |$| ______
|   \___ \ )  / \___ \ / __/   |$| $$$$$/
|   (____/(__/  (____/(____)   |$| $$$$/
|o____________________________o|/  &//
```
> From: https://patorjk.com with Graceful font

#### Features

- Written in C99, and is ready to be used in C++ as well
- Cross-platform (Posix and Windows)
- Command interaction with stdin, stdout, and stderr
- Blocking (sync) and non-blocking (async) version
- No dependencies (only standard C and system libraries).
    No longer need a heavy framework like boost or poco just to capture output from running a command.
- Header only library (source version available as well)
- CMake integration

#### API Documentation
Just read the header file `System2.h`. Everything is documented there.

#### Minimum running example

```c
#include "System2.h"
#include <stdio.h>

int main(int argc, char** argv) 
{
    System2CommandInfo commandInfo;

    #ifdef __unix__
        System2Run("read testVar && echo testVar is \\\"$testVar\\\"", &commandInfo);
    #endif
    
    #ifdef _WIN32
        System2Run( "set /p testVar= && cmd /s /v /c \"echo testVar is ^\"!testVar!^\"\"", 
                    &commandInfo);
    #endif
    
    char input[] = "test content\n";
    System2WriteToInput(&commandInfo, input, sizeof(input));
    
    //Waiting here simulates the child process has "finished" and we read the output of it
    //Sleep(2000);
    
    char outputBuffer[1024];
    uint32_t bytesRead = 0;
    
    //System2ReadFromOutput can also return SYSTEM2_RESULT_READ_NOT_FINISHED if we have more to read
    System2ReadFromOutput(&commandInfo, outputBuffer, 1023, &bytesRead);
    outputBuffer[bytesRead] = 0;
    
    int returnCode;
    System2GetCommandReturnValueSync(&commandInfo, &returnCode);
    
    printf("%s: %d\n", "Command has executed with return value", returnCode);
    printf("%s\n", outputBuffer);
    return 0;
    
    //Output: Command has executed with return value: 0
    //Output: testVar is "test content"
}
```

#### Using System2 in your project

This library has header only version, just include "System2.h" and you are good to go.

However, this will leak system library headers to your codebase as it uses it.

In that case, you can use the source version of the library.

1. Define `SYSTEM2_DECLARATION_ONLY 1` before including `System2.h`

2. Then, either:
    - Add `System2.c` to you codebase
    - Or include `System2.h` in a single c file and define `SYSTEM2_IMPLEMENTATION_ONLY 1` before it
    - Or link your project with `System2` target in CMake`
