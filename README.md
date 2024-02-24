### System2

`System2` is a cross-platform c library that allows you to call system commands, just like `system` but with the ability to
provide input to stdin and capture the output from stdout and stderr.

#### Features

- Written in C99, and is ready to be used in C++ as well
- Cross-platform (Posix and Windows(WIP))
- Command interaction with stdin, stdout, and stderr
- Blocking (sync) and non-blocking (async) version
- No dependencies (only standard C and system libraries).
    No longer need a heavy framework like boost or poco just to capture output from running a command.
- Header only library
- CMake integration


#### Example

```c

#include "System2.h"
#include <stdio.h>

int main(int argc, char** argv) 
{
    System2CommandInfo commandInfo;
    
    System2Run("read testVar && echo testVar is \\\"$testVar\\\"", &commandInfo);
    char input[] = "test content\n";
    System2WriteToInput(&commandInfo, input, sizeof(input));
    
    char outputBuffer[1024];
    uint32_t bytesRead = 0;
    System2ReadFromOutput(&commandInfo, outputBuffer, 1023, &bytesRead);
    outputBuffer[bytesRead] = 0;
    printf("%s", outputBuffer);
    return 0;
}


```