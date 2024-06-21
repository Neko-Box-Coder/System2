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
    {
        System2CommandInfo commandInfo;
        memset(&commandInfo, 0, sizeof(System2CommandInfo));
        commandInfo.RedirectInput = true;
        commandInfo.RedirectOutput = true;
        SYSTEM2_RESULT result;

        #if defined(__unix__) || defined(__APPLE__)
            result = System2Run("read testVar && echo testVar is \\\"$testVar\\\"", &commandInfo);
        #endif
        
        #if defined(_WIN32)
            result = System2Run("set /p testVar= && echo testVar is \"!testVar!\"", &commandInfo);
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
        //In which case can use a do while loop to keep getting the output
        result = System2ReadFromOutput(&commandInfo, outputBuffer, 1023, &bytesRead);
        EXIT_IF_FAILED(result);
        outputBuffer[bytesRead] = 0;
        
        int returnCode = -1;
        result = System2GetCommandReturnValueSync(&commandInfo, &returnCode);
        EXIT_IF_FAILED(result);
        
        printf("%s", outputBuffer);
        printf("%s: %d\n", "Command has executed with return value", returnCode);
    }
    
    //Output: Command has executed with return value: : 0
    //Output: testVar is "test content"
    
    printf("\nUsing stdin now, enter the value of testVar: ");
    fflush(stdout);
    
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
        result = System2GetCommandReturnValueSync(&commandInfo, &returnCode);
        EXIT_IF_FAILED(result);
        
        printf("%s: %d\n", "Command has executed with return value", returnCode);
    }
    
    return 0;
}
