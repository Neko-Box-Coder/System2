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
