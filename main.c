#if USE_SYSTEM2_SOURCE
    #define SYSTEM2_DECLARATION_ONLY 1
#endif

#include "System2.h"
#include <stdio.h>

#define EXIT_IF_FAILED(result) \
if(result != SYSTEM2_RESULT_SUCCESS) \
{\
    printf("Error at %d: %d", __LINE__, result);\
    return -1;\
}

int main(int argc, char** argv) 
{
    System2CommandInfo commandInfo;
    SYSTEM2_RESULT result;
    
    #if defined(__unix__) || defined(__APPLE__)
        result = System2Run("read testVar && echo testVar is \\\"$testVar\\\"", &commandInfo);
    #endif
    
    #if defined(_WIN32)
        result = System2Run("set /p testVar= && cmd /s /v /c \"echo testVar is ^\"!testVar!^\"\"", 
                            &commandInfo);
    #endif
    
    EXIT_IF_FAILED(result);
    
    char input[] = "test content\n";
    result = System2WriteToInput(&commandInfo, input, sizeof(input));
    EXIT_IF_FAILED(result);
    
    //Waiting here simulates the child process has "finished" and we read the output of it
    //Sleep(2000);
    
    char outputBuffer[1024];
    uint32_t bytesRead = 0;
    
    //System2ReadFromOutput can also return SYSTEM2_RESULT_READ_NOT_FINISHED if we have more to read
    result = System2ReadFromOutput(&commandInfo, outputBuffer, 1023, &bytesRead);
    EXIT_IF_FAILED(result);
    outputBuffer[bytesRead] = 0;
    
    int returnCode;
    result = System2GetCommandReturnValueSync(&commandInfo, &returnCode);
    EXIT_IF_FAILED(result);
    
    printf("%s: %d\n", "Command has executed with return value", returnCode);
    printf("%s\n", outputBuffer);
    return 0;
    
    //Output: Command has executed with return value: : 0
    //Output: testVar is "test content"
}
