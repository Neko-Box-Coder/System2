#ifndef SYSTEM2_H
#define SYSTEM2_H

#ifdef __cplusplus
    extern "C" {
#endif

#include <stdint.h>
#include <string.h>

#ifdef __unix__
    #include <unistd.h>
#endif

#ifdef __unix__
    typedef struct
    {
        int ParentToChildPipes[2];
        int ChildToParentPipes[2];
        pid_t ChildProcessID;
    } System2CommandInfo;
    
    typedef enum
    {
        SYSTEM2_FD_READ = 0,
        SYSTEM2_FD_WRITE = 1
    } SYSTEM2_PIPE_FILE_DESCRIPTORS;
    
    typedef enum
    {
        SYSTEM2_RESULT_COMMAND_TERMINATED = 3,
        SYSTEM2_RESULT_COMMAND_NOT_FINISHED = 2,
        SYSTEM2_RESULT_READ_NOT_FINISHED = 1,
        SYSTEM2_RESULT_SUCCESS = 0,
        SYSTEM2_RESULT_PIPE_CREATE_FAILED = -1,
        SYSTEM2_RESULT_PIPE_FD_CLOSE_FAILED = -2,
        SYSTEM2_RESULT_FORK_FAILED = -3,
        SYSTEM2_RESULT_READ_FAILED = -4,
        SYSTEM2_RESULT_WRITE_FAILED = -5,
        SYSTEM2_RESULT_COMMAND_WAIT_FAILED = -6,
    } SYSTEM2_RESULT;
#endif

SYSTEM2_RESULT System2Run(const char* command, System2CommandInfo* outCommandInfo);
SYSTEM2_RESULT System2ReadFromOutput(   System2CommandInfo* info, 
                                        char* outputBuffer, 
                                        uint32_t outputBufferSize,
                                        uint32_t* outBytesRead);

SYSTEM2_RESULT System2WriteToInput( System2CommandInfo* info, 
                                    const char* inputBuffer, 
                                    const uint32_t inputBufferSize);

SYSTEM2_RESULT System2GetCommandReturnValueAsync(   System2CommandInfo* info, 
                                                    int* outReturnCode);

SYSTEM2_RESULT System2GetCommandReturnValueSync(System2CommandInfo* info, 
                                                int* outReturnCode);

#ifdef __cplusplus
    }
#endif

#endif