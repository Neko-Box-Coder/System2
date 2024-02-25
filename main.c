#include "System2.h"
#include <stdio.h>

int main(int argc, char** argv) 
{
    System2CommandInfo commandInfo;
    
    #ifdef __unix__
        System2Run("read testVar && echo testVar is \\\"$testVar\\\"", &commandInfo);
    #endif
    
    SYSTEM2_RESULT result;
    
    #ifdef _WIN32
        result = System2Run("set /p testVar= && cmd /s /v /c \"echo testVar is ^\"!testVar!^\"\"", &commandInfo);
        //result = System2Run("dir", &commandInfo);
    #endif
    
    if(result != SYSTEM2_RESULT_SUCCESS) 
    {
        printf("Error System2Run: %d\n", result);
        return -1;
    }
    
    char input[] = "test content\n";
    result = System2WriteToInput(&commandInfo, input, sizeof(input));
    
    //Sleep(2000);
    
    if(result != SYSTEM2_RESULT_SUCCESS) 
    {
        printf("Error System2WriteToInput: %d\n", result);
        return -1;
    }
    
    char outputBuffer[1024];
    uint32_t bytesRead = 0;
    result = System2ReadFromOutput(&commandInfo, outputBuffer, 1023, &bytesRead);
    outputBuffer[bytesRead] = 0;
    
    if(result != SYSTEM2_RESULT_SUCCESS) 
    {
        printf("Error System2ReadFromOutput: %d\n", result);
        return -1;
    }
    
    int returnCode;
    if(result != System2GetCommandReturnValueSync(&commandInfo, &returnCode))
    {
        printf("Error System2GetCommandReturnValueSync: %d\n", result);
        return -1;
    }
    
    printf("%s", outputBuffer);
    return 0;
}
