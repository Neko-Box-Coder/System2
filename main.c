#if SYSTEM2_USE_SOURCE
    #define SYSTEM2_DECLARATION_ONLY 1
#endif

//This bypasses inheriting memory from parent process on linux (glibc 2.24) but removes the ability to use RunDirectory.
//See https://github.com/Neko-Box-Coder/System2/issues/3
//#define SYSTEM2_POSIX_SPAWN 1

//#define SYSTEM2_DECLARATION_ONLY 1

//#define SYSTEM2_IMPLEMENTATION_ONLY 1

#include "System2.h"
#include <stdio.h>

#if SYSTEM2_TEST_MEMORY
    #include <stdlib.h>
#endif

#define EXIT_IF_FAILED(result) \
if(result != SYSTEM2_RESULT_SUCCESS) \
{\
    printf("Error at %d: %d", __LINE__, result);\
    exit(-1);\
}


System2CommandInfo RedirectIOExample(void)
{
    System2CommandInfo commandInfo;
    memset(&commandInfo, 0, sizeof(System2CommandInfo));
    commandInfo.RedirectInput = true;
    commandInfo.RedirectOutput = true;
    SYSTEM2_RESULT result;

    #if defined(__unix__) || defined(__APPLE__)
        result = System2Run("sleep 1; read testVar && echo testVar is \\\"$testVar\\\"", &commandInfo);
    #endif
    
    #if defined(_WIN32)
        result = System2Run("ping localhost -n 2 > nul & set /p testVar= && "
                            "echo testVar is \"!testVar!\"", &commandInfo);
    #endif
    
    EXIT_IF_FAILED(result);
    
    char input[] = "test content\n";
    result = System2WriteToInput(&commandInfo, input, sizeof(input));
    EXIT_IF_FAILED(result);
    
    return commandInfo;
}

void BlockedCommandExample(void)
{
    System2CommandInfo commandInfo;
    
    memset(&commandInfo, 0, sizeof(System2CommandInfo));
    SYSTEM2_RESULT result;
    #if defined(__unix__) || defined(__APPLE__)
        result = System2Run("sleep 2; echo Hello", &commandInfo);
    #endif
    
    #if defined(_WIN32)
        result = System2Run("ping localhost -n 3 > nul & echo Hello", &commandInfo);
    #endif
    
    EXIT_IF_FAILED(result);
    
    int returnCode = -1;
    result = System2GetCommandReturnValueSync(&commandInfo, &returnCode, false);
    EXIT_IF_FAILED(result);
}

void StdinStdoutExample(void)
{
    System2CommandInfo commandInfo;
    memset(&commandInfo, 0, sizeof(System2CommandInfo));
    SYSTEM2_RESULT result;

    #if defined(__unix__) || defined(__APPLE__)
        result = System2Run("read testVar && echo testVar is \\\"$testVar\\\"", &commandInfo);
    #endif
    
    #if defined(_WIN32)
        result = System2Run("set /p testVar= && echo testVar is \"!testVar!\"", &commandInfo);
    #endif
    
    EXIT_IF_FAILED(result);
    
    int returnCode = -1;
    result = System2GetCommandReturnValueSync(&commandInfo, &returnCode, false);
    EXIT_IF_FAILED(result);
    
    printf("%s: %d\n", "Command has executed with return value", returnCode);
}

int main(int argc, char** argv) 
{
    (void)argc;
    (void)argv;
    
    #if SYSTEM2_TEST_MEMORY
        void* testMem = malloc(50 * 1024 * 1024); //Test 50 MB
        memset(testMem, 1, 50 * 1024 * 1024);
    #endif
    
    //Execute the first command
    System2CommandInfo commandInfo = RedirectIOExample();

    //We can execute other commands while the previous one is still running
    //Output: Hello
    BlockedCommandExample();
    
    //The first command should be finish by now.
    //Output: testVar is "test content"
    //Output: 1st command has finished with return value: : 0
    {
        int returnCode = -1;
        //True to perform manual cleanup
        SYSTEM2_RESULT result = System2GetCommandReturnValueAsync(&commandInfo, &returnCode, true);
        
        if(result == SYSTEM2_RESULT_SUCCESS)
        {
            char outputBuffer[1024];
            
            do
            {
                uint32_t bytesRead = 0;
                result = System2ReadFromOutput(&commandInfo, outputBuffer, 1023, &bytesRead);
                outputBuffer[bytesRead] = 0;
                printf("%s", outputBuffer);
            }
            while(result == SYSTEM2_RESULT_READ_NOT_FINISHED);
            
            printf("%s: %d\n", "1st command has finished with return value", returnCode);
        }
        else if(result == SYSTEM2_RESULT_COMMAND_NOT_FINISHED)
            printf("1st command not yet finished");
        else
            EXIT_IF_FAILED(result);
        
        result = System2CleanupCommand(&commandInfo);
        EXIT_IF_FAILED(result);
    }
    
    printf("\nUsing stdin now, enter the value of testVar: ");
    fflush(stdout);
    
    //If you don't do redirect, it will directly use stdin and stdout instead
    StdinStdoutExample();
    
    #if SYSTEM2_TEST_MEMORY
        free(testMem);
    #endif
    
    return 0;
}
