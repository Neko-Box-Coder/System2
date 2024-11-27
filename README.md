### System2

`System2` is a cross-platform c library that allows you to call shell commands and other executables (subprocess), just like `system` but with the ability to
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

> [!NOTE]
> 
> For a C++ wrapper, check out [System2.cpp](https://github.com/Neko-Box-Coder/System2.cpp)

#### Features

- Written in C99, and is ready to be used in C++ as well
- Cross-platform (POSIX and Windows)
- Command interaction with stdin, stdout, and stderr
- Invoking shell commands and launching executables
- Blocking (sync) and non-blocking (async) version
- No dependencies (only standard C and system libraries).
    No longer need a heavy framework like boost or poco just to capture output from running a command.
- Header only library (source version available as well)
- UTF-8 support\*
- CMake integration

\* See Remarks for UTF-8 support

#### Quick Start With Minimum running example (Without checks)
Check [main.c](./main.c) for more examples.

```c
//This bypasses inheriting memory from parent process on linux (glibc 2.24) but removes the ability to use RunDirectory.
//See https://github.com/Neko-Box-Coder/System2/issues/3
//#define SYSTEM2_POSIX_SPAWN 1

//#define SYSTEM2_DECLARATION_ONLY 1

//#define SYSTEM2_IMPLEMENTATION_ONLY 1


#include "System2.h"
#include <stdio.h>

int main(int argc, char** argv) 
{
    System2CommandInfo commandInfo;
    memset(&commandInfo, 0, sizeof(System2CommandInfo));
    commandInfo.RedirectInput = true;
    commandInfo.RedirectOutput = true;

    #if defined(__unix__) || defined(__APPLE__)
        System2Run("read testVar && echo testVar is \\\"$testVar\\\"", &commandInfo);
    #endif
    
    #if defined(_WIN32)
        System2Run("set /p testVar= && echo testVar is \"!testVar!\"", &commandInfo);
    #endif
    
    char input[] = "test content\n";
    System2WriteToInput(&commandInfo, input, sizeof(input));
    
    //Waiting here simulates the child process has "finished" and we read the output of it
    //Sleep(2000);
    
    char outputBuffer[1024];
    uint32_t bytesRead = 0;
    
    //System2ReadFromOutput can also return SYSTEM2_RESULT_READ_NOT_FINISHED if we have more to read
    //In which case can use a do while loop to keep getting the output
    System2ReadFromOutput(&commandInfo, outputBuffer, 1023, &bytesRead);
    outputBuffer[bytesRead] = 0;
    
    int returnCode = -1;
    System2GetCommandReturnValueSync(&commandInfo, &returnCode, false);
    
    printf("%s\n", outputBuffer);
    printf("%s: %d\n", "Command has executed with return value", returnCode);
    
    return 0;
    
    //Output: Command has executed with return value: 0
    //Output: testVar is "test content"
}
```

#### API Documentation
```cpp
typedef struct
{
    bool RedirectInput;         //Redirect input with pipe?
    bool RedirectOutput;        //Redirect output with pipe?
    const char* RunDirectory;   //The directory to run the command in?
    
    #if defined(_WIN32)
        bool DisableEscapes;    //Disable automatic escaping?
    #endif
    
    //Internal fields...
} System2CommandInfo;

/*
Runs the command in system shell just like the `system()` funcion with the given settings 
    passed with `inOutCommandInfo`.

This uses 
`sh -c command` for POSIX and
`cmd /s /v /c command` for Windows

Could return the follow result:
- SYSTEM2_RESULT_SUCCESS
- SYSTEM2_RESULT_PIPE_CREATE_FAILED
- SYSTEM2_RESULT_CREATE_CHILD_PROCESS_FAILED
- SYSTEM2_RESULT_PIPE_FD_CLOSE_FAILED
- SYSTEM2_RESULT_COMMAND_CONSTRUCT_FAILED
- SYSTEM2_RESULT_POSIX_SPAWN_FILE_ACTION_DESTROY_FAILED
- SYSTEM2_RESULT_POSIX_SPAWN_FILE_ACTION_DUP2_FAILED
- SYSTEM2_RESULT_POSIX_SPAWN_RUN_DIRECTORY_NOT_SUPPORTED
*/
SYSTEM2_FUNC_PREFIX SYSTEM2_RESULT System2Run(  const char* command, 
                                                System2CommandInfo* inOutCommandInfo);

/*
Runs the executable (which can search in PATH env variable) with the given arguments and settings
    passed with inOutCommandInfo.

On Windows, automatic escaping can be removed by setting the `DisableEscape` in `inOutCommandInfo`

Could return the follow result:
- SYSTEM2_RESULT_SUCCESS
- SYSTEM2_RESULT_PIPE_CREATE_FAILED
- SYSTEM2_RESULT_CREATE_CHILD_PROCESS_FAILED
- SYSTEM2_RESULT_PIPE_FD_CLOSE_FAILED
- SYSTEM2_RESULT_COMMAND_CONSTRUCT_FAILED
- SYSTEM2_RESULT_POSIX_SPAWN_FILE_ACTION_DESTROY_FAILED
- SYSTEM2_RESULT_POSIX_SPAWN_FILE_ACTION_DUP2_FAILED
- SYSTEM2_RESULT_POSIX_SPAWN_RUN_DIRECTORY_NOT_SUPPORTED
*/
SYSTEM2_FUNC_PREFIX SYSTEM2_RESULT System2RunSubprocess(const char* executable,
                                                        const char* const* args,
                                                        int argsCount,
                                                        System2CommandInfo* inOutCommandInfo);


/*
Reads the output (stdout and stderr) from the command. 
Output string is **NOT** null terminated.

If SYSTEM2_RESULT_READ_NOT_FINISHED is returned, 
this function can be called again until SYSTEM2_RESULT_SUCCESS to retrieve the rest of the output.

outBytesRead determines how many bytes have been read for **this** function call

Could return the follow result:
- SYSTEM2_RESULT_SUCCESS
- SYSTEM2_RESULT_READ_NOT_FINISHED
- SYSTEM2_RESULT_READ_FAILED
*/
SYSTEM2_FUNC_PREFIX SYSTEM2_RESULT System2ReadFromOutput(   const System2CommandInfo* info, 
                                                            char* outputBuffer, 
                                                            uint32_t outputBufferSize,
                                                            uint32_t* outBytesRead);

/*
Write the input (stdin) to the command. 

Could return the follow result:
- SYSTEM2_RESULT_SUCCESS
- SYSTEM2_RESULT_WRITE_FAILED
*/
SYSTEM2_FUNC_PREFIX SYSTEM2_RESULT System2WriteToInput( const System2CommandInfo* info, 
                                                        const char* inputBuffer, 
                                                        const uint32_t inputBufferSize);


//TODO: Might want to add this to have this ability to close input pipe manually
//SYSTEM2_FUNC_PREFIX SYSTEM2_RESULT System2CloseInput(System2CommandInfo* info);

/*
Cleanup any open handles associated with the command.

Could return the follow result:
- SYSTEM2_RESULT_SUCCESS
- SYSTEM2_RESULT_PIPE_FD_CLOSE_FAILED
*/
SYSTEM2_FUNC_PREFIX SYSTEM2_RESULT System2CleanupCommand(const System2CommandInfo* info);

/*
Gets the return code if the command has finished.
Otherwise, this will return SYSTEM2_RESULT_COMMAND_NOT_FINISHED immediately.

If `manualCleanup` is false, 
`System2CleanupCommand()` is automatically called when the command has exited.

Otherwise, `System2CleanupCommand()` should be called when the command has exited.

Could return the follow result:
- SYSTEM2_RESULT_SUCCESS
- SYSTEM2_RESULT_COMMAND_NOT_FINISHED
- SYSTEM2_RESULT_COMMAND_TERMINATED
- SYSTEM2_RESULT_PIPE_FD_CLOSE_FAILED
- SYSTEM2_RESULT_COMMAND_WAIT_ASYNC_FAILED
*/
SYSTEM2_FUNC_PREFIX 
SYSTEM2_RESULT System2GetCommandReturnValueAsync(   const System2CommandInfo* info, 
                                                    int* outReturnCode,
                                                    bool manualCleanup);

/*
Wait for the command to finish and gets the return code

If `manualCleanup` is false, 
`System2CleanupCommand()` is automatically called when the command has exited.

Otherwise, `System2CleanupCommand()` should be called when the command has exited.

Could return the follow result:
- SYSTEM2_RESULT_SUCCESS
- SYSTEM2_RESULT_COMMAND_TERMINATED
- SYSTEM2_RESULT_PIPE_FD_CLOSE_FAILED
- SYSTEM2_RESULT_COMMAND_WAIT_SYNC_FAILED
*/
SYSTEM2_FUNC_PREFIX SYSTEM2_RESULT System2GetCommandReturnValueSync(const System2CommandInfo* info, 
                                                                    int* outReturnCode,
                                                                    bool manualCleanup);
```


#### Using System2 in your project

This library has header only version, just include "System2.h" and you are good to go.

However, this will leak system library headers to your codebase.

In that case, you can use the source version of the library.

1. Define `SYSTEM2_DECLARATION_ONLY 1` before including `System2.h`

2. Then, either:
    - Add `System2.c` to you codebase
    - Or include `System2.h` in a single c file and define `SYSTEM2_IMPLEMENTATION_ONLY 1` before it
    - Or link your project with `System2` target in CMake`

---
#### Remarks
- For Linux or MacOS, `System2Run()` and `System2RunSubprocess()` will inherit the parent process memory (due to how `fork()` works).
    Meaning it is possible to over commit memory and therefore causes out of memory error.
    - A temporary fix is there by using `posix_spawn()` instead of `fork()` by `#define SYSTEM2_POSIX_SPAWN 1` before `#include "System2.h"`
    - See [Issue](https://github.com/Neko-Box-Coder/System2/issues/3)
- For POSIX, UTF-8 support should work if it is available on the system. This is however **not tested**.
- For Windows, UTF-8 support works for the **command** input (in theory XP and above but tested on Windows 10). 
    However, the output part is **NOT** in UTF-8. The closest thing you can get for the output is UTF-16 as far as I know.
    Here's what needed to get output in UTF-16:
    1. Instead of `System2Run("<your command>", &commandInfo)`, do `System2Run("cmd /u /s /v /c \"<your command>\"", &commandInfo)`
        This will output a UTF-16 string from cmd stdout/stderr
    2. Read output as usual from `System2ReadFromOutput` but interpret the output as wchar_t string instead.
        - You can then use `WideCharToMultiByte` to convert the output to UTF-8 if needed
    3. If you want to output the UTF-16 output to console, you need to use `_setmode` before calling `wprintf`/`printf`
        - See [this](https://learn.microsoft.com/en-us/cpp/c-runtime-library/reference/setmode?view=msvc-170) for `_setmode` example
