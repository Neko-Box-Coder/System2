

#include "System2.h"
#include <string.h>
#include <stdio.h>

int main(int argc, char** argv)
{
    if(argc == 1)
    {
        printf("Calling with args...\n");
        System2CommandInfo commandInfo;
        memset(&commandInfo, 0, sizeof(System2CommandInfo));
        
        #if defined(_WIN32)
            SYSTEM2_RESULT result = System2Run(".\\VerifyArgs.exe arg1 arg2 arg3", &commandInfo);
        #else
            SYSTEM2_RESULT result = System2Run("./VerifyArgs arg1 arg2 arg3", &commandInfo);
        #endif
        
        if(result != SYSTEM2_RESULT_SUCCESS)
        {
            printf("Line %d failed\n", __LINE__);
            return 1;
        }
        
        int returnCode;
        result = System2GetCommandReturnValue(&commandInfo, -1, &returnCode);
        if(result != SYSTEM2_RESULT_SUCCESS)
        {
            printf("Line %d failed\n", __LINE__);
            return 1;
        }
        
        if(returnCode != 0)
        {
            printf("Line %d failed, return code %i\n", __LINE__, returnCode);
            return returnCode;
        }
        
        const char* args[] = {"arg1", "arg2", "arg3"};
        #if defined(_WIN32)
            result = System2RunSubprocess(".\\VerifyArgs.exe", args, 3, &commandInfo);
        #else
            result = System2RunSubprocess("./VerifyArgs", args, 3, &commandInfo);
        #endif
        if(result != SYSTEM2_RESULT_SUCCESS)
        {
            printf("Line %d failed\n", __LINE__);
            return 1;
        }
        
        result = System2GetCommandReturnValue(&commandInfo, -1, &returnCode);
        if(result != SYSTEM2_RESULT_SUCCESS)
        {
            printf("Line %d failed\n", __LINE__);
            return 1;
        }
        
        if(returnCode != 0)
        {
            printf("Line %d failed, return code %i\n", __LINE__, returnCode);
            return returnCode;
        }
        
        return 0;
    }
    else
    {
        printf("Got args...\n");
        return (argc == 4 && strcmp(argv[1], "arg1") == 0 && strcmp(argv[2], "arg2") == 0 && strcmp(argv[3], "arg3") == 0) ? 0 : 1;
    }
}
