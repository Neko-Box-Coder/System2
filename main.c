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

#if SYSTEM2_MIN_EXAMPLE
//This is the readme example
int main(int argc, char** argv) 
{
    (void)argc;
    (void)argv;
    
    //Initialize command info
    System2CommandInfo commandInfo;
    {
        memset(&commandInfo, 0, sizeof(System2CommandInfo));
        commandInfo.RedirectInput = true;
        commandInfo.RedirectOutput = true;
    }

    //Run the command in shell (subprocess is also available, see main.c)
    {
        #if defined(__unix__) || defined(__APPLE__)
            System2Run("read testVar && echo testVar is \\\"$testVar\\\"", &commandInfo);
        #elif
            System2Run("set /p testVar= && echo testVar is \"!testVar!\"", &commandInfo);
        #endif
    }
    
    //Send input to the command
    {
        char input[] = "test content\n";
        System2WriteToInput(&commandInfo, input, sizeof(input));
    }
    
    //Wait for command to finish and get return code
    int returnCode = -1;
    System2GetCommandReturnValue(&commandInfo, -1, &returnCode);
    
    //Capture output and print it
    {
        char outputBuffer[1024];
        uint32_t bytesRead = 0;
        
        System2ReadFromOutput(&commandInfo, outputBuffer, 1023, &bytesRead);
        outputBuffer[bytesRead] = 0;
        
        printf("%s\n", outputBuffer);
        printf("%s: %d\n", "Command has executed with return value", returnCode);
    }
    
    System2CleanupCommand(&commandInfo);
    return 0;
    
    //Output: testVar is "test content"
    //Output: Command has executed with return value: 0
}

#else //#if SYSTEM2_MIN_EXAMPLE

#if SYSTEM2_TEST_MEMORY
    #include <stdlib.h>
#endif

void RunSubprocessExample(void);
System2CommandInfo RedirectIOExample(void);
void ReadRedirectedIOAsyncExample(System2CommandInfo commandInfo);
void BlockedCommandExample(void);
void StdinStdoutExample(void);
void RunWithEnvExample(void);
void SetEnvVarsExample(void);
void ReadEnvVarsExample(void);
void TermExample(void);
void KillExample(void);
void TimeoutExample(void);

int main(int argc, char** argv) 
{
    (void)argc;
    (void)argv;
    
    #if SYSTEM2_TEST_MEMORY
        void* testMem = malloc(50 * 1024 * 1024); //Test 50 MB
        memset(testMem, 1, 50 * 1024 * 1024);
    #endif
    
    #if !SYSTEM2_POSIX_SPAWN
        RunSubprocessExample();
    #endif
    
    //Execute the first command
    System2CommandInfo commandInfo = RedirectIOExample();

    //We can execute other commands while the previous one is still running
    //Output: Hello
    BlockedCommandExample();
    
    //The first command should be finish by now.
    ReadRedirectedIOAsyncExample(commandInfo);
    memset(&commandInfo, 0, sizeof(commandInfo));
    
    //If you don't do redirect, it will directly use stdin and stdout instead
    StdinStdoutExample();
    
    #if SYSTEM2_TEST_MEMORY
        free(testMem);
    #endif
    
    SetEnvVarsExample();
    ReadEnvVarsExample();
    RunWithEnvExample();
    TermExample();
    KillExample();
    TimeoutExample();
    
    return 0;
}

#define EXIT_IF_FAILED(result) \
    if(result != SYSTEM2_RESULT_SUCCESS) \
    {\
        printf("Error at %d: %d", __LINE__, result);\
        exit(-1);\
    }


#define FUNC_HEADER() printf("\n\n---------------------\n%s\n---------------------\n", __func__)

#if defined(_WIN32)
    #define sleep(x) Sleep(1000 * (x))
#endif

void RunSubprocessExample(void)
{
    FUNC_HEADER();
    
    System2CommandInfo commandInfo;
    
    memset(&commandInfo, 0, sizeof(System2CommandInfo));
    commandInfo.RunDirectory = "..";
    SYSTEM2_RESULT result;
    #if defined(__unix__) || defined(__APPLE__)
        const char* args[] = {"-r", "-i", "--include", "*.c", "main"};
        result = System2RunSubprocess("grep", args, sizeof(args) / sizeof(char*), &commandInfo);
    #endif
    
    #if defined(_WIN32)
        const char* args[] = {"/s", "/i", "main", "*.c"};
        result = System2RunSubprocess("findstr", args, sizeof(args) / sizeof(char*), &commandInfo);
    #endif
    
    EXIT_IF_FAILED(result);
    
    int returnCode = -1;
    result = System2GetCommandReturnValue(&commandInfo, -1, &returnCode);
    EXIT_IF_FAILED(result);
    
    result = System2CleanupCommand(&commandInfo);
    EXIT_IF_FAILED(result);
}

System2CommandInfo RedirectIOExample(void)
{
    FUNC_HEADER();
    
    System2CommandInfo commandInfo;
    memset(&commandInfo, 0, sizeof(System2CommandInfo));
    commandInfo.RedirectInput = true;
    commandInfo.RedirectOutput = true;
    SYSTEM2_RESULT result;

    #if defined(__unix__) || defined(__APPLE__)
        result = System2Run("sleep 3; read testVar && echo testVar is \\\"$testVar\\\"", &commandInfo);
    #endif
    
    #if defined(_WIN32)
        result = System2Run("ping localhost -n 4 > nul & set /p testVar= && "
                            "echo testVar is \"!testVar!\"", &commandInfo);
    #endif
    
    EXIT_IF_FAILED(result);
    
    char input[] = "test content\n";
    result = System2WriteToInput(&commandInfo, input, sizeof(input));
    EXIT_IF_FAILED(result);
    
    return commandInfo;
}

void ReadRedirectedIOAsyncExample(System2CommandInfo commandInfo)
{
    FUNC_HEADER();
    
    //Output: testVar is "test content"
    //Output: 1st command has finished with return value: : 0
    
    int returnCode = -1;
    SYSTEM2_RESULT result = System2GetCommandReturnValue(&commandInfo, 0, &returnCode);
    while(result == SYSTEM2_RESULT_COMMAND_NOT_FINISHED)
    {
        printf("1st command not yet finished\n");
        sleep(3);
        result = System2GetCommandReturnValue(&commandInfo, 0, &returnCode);
    }
    
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
    else
        EXIT_IF_FAILED(result);
    
    result = System2CleanupCommand(&commandInfo);
    EXIT_IF_FAILED(result);
}

void BlockedCommandExample(void)
{
    FUNC_HEADER();
    
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
    result = System2GetCommandReturnValue(&commandInfo, -1, &returnCode);
    EXIT_IF_FAILED(result);
    
    result = System2CleanupCommand(&commandInfo);
    EXIT_IF_FAILED(result);
}

void StdinStdoutExample(void)
{
    FUNC_HEADER();
    
    printf("\nUsing stdin now, enter the value of testVar: ");
    fflush(stdout);
    
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
    result = System2GetCommandReturnValue(&commandInfo, -1, &returnCode);
    EXIT_IF_FAILED(result);
    
    printf("%s: %d\n", "Command has executed with return value", returnCode);
    
    result = System2CleanupCommand(&commandInfo);
    EXIT_IF_FAILED(result);
}

void RunWithEnvExample(void)
{
    FUNC_HEADER();
    
    System2CommandInfo commandInfo;
    memset(&commandInfo, 0, sizeof(System2CommandInfo));
    const char* envName = "NewEnv";
    const char* envValue = "NewEnvValue";
    commandInfo.EnvVarsNames = &envName;
    commandInfo.EnvVarsValues = &envValue;
    commandInfo.EnvVarsCount = 1;
    
    SYSTEM2_RESULT result;
    #if defined(__unix__) || defined(__APPLE__)
        result = System2Run("echo NewEnv is \\\"$NewEnv\\\"", &commandInfo);
    #endif
    
    #if defined(_WIN32)
        result = System2Run("echo NewEnv is \"%NewEnv%\"", &commandInfo);
    #endif
    
    EXIT_IF_FAILED(result);
    
    int returnCode = -1;
    result = System2GetCommandReturnValue(&commandInfo, -1, &returnCode);
    EXIT_IF_FAILED(result);
    
    result = System2CleanupCommand(&commandInfo);
    EXIT_IF_FAILED(result);
}

void SetEnvVarsExample(void)
{
    FUNC_HEADER();
    
    SYSTEM2_RESULT result = System2SetEnvironmentVariable("TestEnv", "TestEnvValue");
    EXIT_IF_FAILED(result);
}

void ReadEnvVarsExample(void)
{
    FUNC_HEADER();
    
    int envVarsCount = 0;
    void* resource;
    SYSTEM2_RESULT result = System2GetEnvironmentVariablesCount(&envVarsCount, &resource);
    EXIT_IF_FAILED(result);
    
    for(int i = 0; i < envVarsCount; ++i)
    {
        const char* envName;
        int envNameLength;
        const char* envValue;
        int envValueLength;
        
        result = System2GetEnvironmentVariable( resource,
                                                &envName, 
                                                &envNameLength, 
                                                &envValue, 
                                                &envValueLength,
                                                i);
        EXIT_IF_FAILED(result);
    
        printf("[%d] %.*s: %.*s\n", i, envNameLength, envName, envValueLength, envValue);
    }
    
    result = System2EnvironmentVariableFree(&resource);
    EXIT_IF_FAILED(result);
}

void TermExample(void)
{
    FUNC_HEADER();
    
    System2CommandInfo commandInfo;
    memset(&commandInfo, 0, sizeof(System2CommandInfo));
    SYSTEM2_RESULT result;

    #if defined(__unix__) || defined(__APPLE__)
        result = System2Run("sleep 2; echo Hello", &commandInfo);
    #endif
    
    #if defined(_WIN32)
        result = System2Run("notepad.exe", &commandInfo);
    #endif
    
    EXIT_IF_FAILED(result);
    
    sleep(1);
    
    result = System2Term(&commandInfo);
    EXIT_IF_FAILED(result);
    
    sleep(1);
    
    int returnCode = -1;
    result = System2GetCommandReturnValue(&commandInfo, 0, &returnCode);
    printf( "GetCommandReturnValue Result after System2Term is %d, returnCode is %d\n",  
            (int)result, 
            returnCode);
    
    result = System2CleanupCommand(&commandInfo);
    EXIT_IF_FAILED(result);
}

void KillExample(void)
{
    FUNC_HEADER();
    
    System2CommandInfo commandInfo;
    memset(&commandInfo, 0, sizeof(System2CommandInfo));
    SYSTEM2_RESULT result;

    #if defined(__unix__) || defined(__APPLE__)
        result = System2Run("sleep 2; echo Hello", &commandInfo);
    #endif
    
    #if defined(_WIN32)
        result = System2Run("ping localhost -n 3 > nul & echo Hello", &commandInfo);
    #endif
    
    sleep(1);
    
    result = System2Kill(&commandInfo);
    EXIT_IF_FAILED(result);
    
    sleep(1);
    
    int returnCode = -1;
    result = System2GetCommandReturnValue(&commandInfo, 0, &returnCode);
    printf( "GetCommandReturnValue Result after System2Kill is %d, returnCode is %d\n",  
            (int)result, 
            returnCode);
    
    result = System2CleanupCommand(&commandInfo);
    EXIT_IF_FAILED(result);
}

void TimeoutExample(void)
{
    FUNC_HEADER();
    
    System2CommandInfo commandInfo;
    memset(&commandInfo, 0, sizeof(System2CommandInfo));
    SYSTEM2_RESULT result;

    #if defined(__unix__) || defined(__APPLE__)
        result = System2Run("sleep 4; echo Hello", &commandInfo);
    #endif
    
    #if defined(_WIN32)
        result = System2Run("ping localhost -n 5 > nul & echo Hello", &commandInfo);
    #endif
    
    EXIT_IF_FAILED(result);
    
    printf("Trying to wait for command to finish with 2 seconds timeout...\n");
    int returnCode = -1;
    result = System2GetCommandReturnValue(&commandInfo, 2, &returnCode);
    printf( "GetCommandReturnValue Result after System2Term is %d, returnCode is %d\n",  
            (int)result, 
            returnCode);
    
    result = System2CleanupCommand(&commandInfo);
    EXIT_IF_FAILED(result);
    
    printf("Hello should be printed now even after we have cleaned up the command info\n");
    
    sleep(3);
}

#endif //#else
