#ifndef SYSTEM2_H
#define SYSTEM2_H

//============================================================
//Declaration
//============================================================

/*
If you do not want to use header only due to system header leakage
1.  Define SYSTEM2_DECLARATION_ONLY 1 before you include this header
2.  Add System2.c to your project or
    define SYSTEM2_IMPLEMENTATION_ONLY 1 and include this header in a c file
*/

#if SYSTEM2_DECLARATION_ONLY
    #if defined(__unix__) || defined(__APPLE__)
        typedef int pid_t;
    #endif
    
    #if defined(_WIN32)
        typedef void* HANDLE;
    #endif
#else
    #if defined(__unix__) || defined(__APPLE__)
        #include <unistd.h>
    #endif

    #if defined(_WIN32)
        #include <windows.h>
    #endif
#endif

#if SYSTEM2_DECLARATION_ONLY || SYSTEM2_IMPLEMENTATION_ONLY
    #define SYSTEM2_FUNC_PREFIX
#else
    #define SYSTEM2_FUNC_PREFIX static inline
#endif

#include <stdint.h>
#include <string.h>

typedef struct
{
    #if defined(__unix__) || defined(__APPLE__)
        int ParentToChildPipes[2];
        int ChildToParentPipes[2];
        pid_t ChildProcessID;
    #endif
    
    #if defined(_WIN32)
        HANDLE ParentToChildPipes[2];
        HANDLE ChildToParentPipes[2];
        HANDLE ChildProcessHandle;
    #endif
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
    SYSTEM2_RESULT_CREATE_CHILD_PROCESS_FAILED = -3,
    SYSTEM2_RESULT_READ_FAILED = -4,
    SYSTEM2_RESULT_WRITE_FAILED = -5,
    SYSTEM2_RESULT_COMMAND_WAIT_SYNC_FAILED = -6,
    SYSTEM2_RESULT_COMMAND_WAIT_ASYNC_FAILED = -7,
    SYSTEM2_RESULT_UNSUPPORTED_PLATFORM = -8,
    SYSTEM2_RESULT_COMMAND_CONSTRUCT_FAILED = -9
} SYSTEM2_RESULT;

/*
Runs the command and stores the internal details in outCommandInfo.
This uses 
`execl("/bin/sh", "sh", "-c", command, NULL);` for POSIX and
`cmd /s /v /c "command"` for Windows

Could return the follow result:
- SYSTEM2_RESULT_SUCCESS
- SYSTEM2_RESULT_PIPE_CREATE_FAILED
- SYSTEM2_RESULT_CREATE_CHILD_PROCESS_FAILED
- SYSTEM2_RESULT_PIPE_FD_CLOSE_FAILED
- SYSTEM2_RESULT_COMMAND_CONSTRUCT_FAILED
*/
SYSTEM2_FUNC_PREFIX SYSTEM2_RESULT System2Run(  const char* command, 
                                                System2CommandInfo* outCommandInfo);

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
Gets the return code if the command has finished.
Otherwise, this will return SYSTEM2_RESULT_COMMAND_NOT_FINISHED immediately.

Could return the follow result:
- SYSTEM2_RESULT_SUCCESS
- SYSTEM2_RESULT_COMMAND_NOT_FINISHED
- SYSTEM2_RESULT_COMMAND_TERMINATED
- SYSTEM2_RESULT_PIPE_FD_CLOSE_FAILED
- SYSTEM2_RESULT_COMMAND_WAIT_ASYNC_FAILED
*/
SYSTEM2_FUNC_PREFIX 
SYSTEM2_RESULT System2GetCommandReturnValueAsync(   const System2CommandInfo* info, 
                                                    int* outReturnCode);

/*
Wait for the command to finish and gets the return code

Could return the follow result:
- SYSTEM2_RESULT_SUCCESS
- SYSTEM2_RESULT_COMMAND_TERMINATED
- SYSTEM2_RESULT_PIPE_FD_CLOSE_FAILED
- SYSTEM2_RESULT_COMMAND_WAIT_SYNC_FAILED
*/
SYSTEM2_FUNC_PREFIX SYSTEM2_RESULT System2GetCommandReturnValueSync(const System2CommandInfo* info, 
                                                                    int* outReturnCode);


//============================================================
//Implementation
//============================================================

#if !SYSTEM2_DECLARATION_ONLY

#include <stdlib.h>
#include <stdbool.h>

#if defined(__unix__) || defined(__APPLE__)
    #include <sys/wait.h>

    SYSTEM2_FUNC_PREFIX SYSTEM2_RESULT System2RunPosix( const char* command, 
                                                        System2CommandInfo* outCommandInfo)
    {
        int result = pipe(outCommandInfo->ParentToChildPipes);
        if(result != 0)
            return SYSTEM2_RESULT_PIPE_CREATE_FAILED;
        
        result = pipe(outCommandInfo->ChildToParentPipes);
        if(result != 0)
            return SYSTEM2_RESULT_PIPE_CREATE_FAILED;

        char* commandCopy = NULL;

        //Process the command and make a copy on the parent process 
        //because malloc is not safe in forked child processes
        {
            const int commandLength = strlen(command);
            int quoteCount = 0;
            for(int i = 0; i < commandLength; ++i)
            {
                if(command[i] == '"')
                    quoteCount++;
            }
            
            const int finalCommandLength =  commandLength +     //Content
                                            quoteCount * 2 +    //Quotes to escape
                                            2 +                 //Wrapping in double quotes
                                            1;                  //Null terminator
        
            commandCopy = (char*)malloc(finalCommandLength);
            if(commandCopy == NULL)
                return SYSTEM2_RESULT_COMMAND_CONSTRUCT_FAILED;

            commandCopy[0] = '"';
            int currentIndex = 1;
            for(int i = 0; i < commandLength; ++i)
            {
                if(command[i] == '"')
                {
                    commandCopy[currentIndex] = '\\';
                    commandCopy[currentIndex + 1] = '"';
                    currentIndex += 2;
                }
                else
                {
                    commandCopy[currentIndex] = command[i];
                    currentIndex++;
                }
            }
            commandCopy[currentIndex] = '"';
            commandCopy[currentIndex + 1] = '\0';
        }
        
        pid_t pid = fork();
        
        if(pid < 0)
        {
            free(commandCopy);
            return SYSTEM2_RESULT_CREATE_CHILD_PROCESS_FAILED;
        }
        //Child
        else if(pid == 0)
        {
            if(close(outCommandInfo->ParentToChildPipes[SYSTEM2_FD_WRITE]) != 0)
                _exit(1);
            
            if(close(outCommandInfo->ChildToParentPipes[SYSTEM2_FD_READ]) != 0)
                _exit(1);
            
            
            result = dup2(outCommandInfo->ParentToChildPipes[0], STDIN_FILENO);
            if(result == -1)
                _exit(1);

            result = dup2(outCommandInfo->ChildToParentPipes[1], STDOUT_FILENO);
            if(result == -1)
                _exit(1);
            
            result = dup2(outCommandInfo->ChildToParentPipes[1], STDERR_FILENO);
            if(result == -1)
                _exit(1);
            
            execl("/bin/sh", "sh", "-c", command, NULL);
            
            //Should never be reached
            
            _exit(1);
        }
        //Parent
        else
        {
            free(commandCopy);
            
            if(close(outCommandInfo->ParentToChildPipes[SYSTEM2_FD_READ]) != 0)
                return SYSTEM2_RESULT_PIPE_FD_CLOSE_FAILED;
            
            if(close(outCommandInfo->ChildToParentPipes[SYSTEM2_FD_WRITE]) != 0)
                return SYSTEM2_RESULT_PIPE_FD_CLOSE_FAILED;
            
            outCommandInfo->ChildProcessID = pid;
        }
        
        return SYSTEM2_RESULT_SUCCESS;
    }
    
    SYSTEM2_FUNC_PREFIX SYSTEM2_RESULT System2ReadFromOutputPosix(  const System2CommandInfo* info, 
                                                                    char* outputBuffer, 
                                                                    uint32_t outputBufferSize,
                                                                    uint32_t* outBytesRead)
    {
        uint32_t readResult;
        *outBytesRead = 0;
        
        while (true)
        {
            readResult = read(  info->ChildToParentPipes[SYSTEM2_FD_READ], 
                                outputBuffer, 
                                outputBufferSize - *outBytesRead);
            
            if(readResult == 0)
                break;
            
            if(readResult == -1)
                return SYSTEM2_RESULT_READ_FAILED;

            outputBuffer += readResult;
            *outBytesRead += readResult;
            
            if(outputBufferSize - *outBytesRead == 0)
                return SYSTEM2_RESULT_READ_NOT_FINISHED;
        }
        
        return SYSTEM2_RESULT_SUCCESS;
    }
    
    SYSTEM2_FUNC_PREFIX SYSTEM2_RESULT System2WriteToInputPosix(const System2CommandInfo* info, 
                                                                const char* inputBuffer, 
                                                                const uint32_t inputBufferSize)
    {
        uint32_t currentWriteLengthLeft = inputBufferSize;
        
        while(true)
        {
            uint32_t writeResult = write(   info->ParentToChildPipes[SYSTEM2_FD_WRITE], 
                                            inputBuffer, 
                                            inputBufferSize);

            if(writeResult == -1)
                return SYSTEM2_RESULT_WRITE_FAILED;
            
            inputBuffer += writeResult;
            currentWriteLengthLeft -= writeResult;
            
            if(currentWriteLengthLeft == 0)
                break;
        }
        
        return SYSTEM2_RESULT_SUCCESS;
    }
    
    SYSTEM2_FUNC_PREFIX 
    SYSTEM2_RESULT System2GetCommandReturnValueAsyncPosix(  const System2CommandInfo* info, 
                                                            int* outReturnCode)
    {
        int status;
        pid_t pidResult = waitpid(info->ChildProcessID, &status, WNOHANG);
        
        if(pidResult == 0)
            return SYSTEM2_RESULT_COMMAND_NOT_FINISHED;
        else if(pidResult == -1)
            return SYSTEM2_RESULT_COMMAND_WAIT_ASYNC_FAILED;

        if(close(info->ChildToParentPipes[SYSTEM2_FD_READ]) != 0)
            return SYSTEM2_RESULT_PIPE_FD_CLOSE_FAILED;

        if(close(info->ParentToChildPipes[SYSTEM2_FD_WRITE]) != 0)
            return SYSTEM2_RESULT_PIPE_FD_CLOSE_FAILED;
        
        if(!WIFEXITED(status))
        {
            *outReturnCode = -1;
            return SYSTEM2_RESULT_COMMAND_TERMINATED;
        }

        *outReturnCode = WEXITSTATUS(status);
        return SYSTEM2_RESULT_SUCCESS;
    }
    
    SYSTEM2_FUNC_PREFIX 
    SYSTEM2_RESULT System2GetCommandReturnValueSyncPosix(   const System2CommandInfo* info, 
                                                            int* outReturnCode)
    {
        int status;
        pid_t pidResult = waitpid(info->ChildProcessID, &status, 0);
        
        if(pidResult == -1)
            return SYSTEM2_RESULT_COMMAND_WAIT_SYNC_FAILED;
        
        if(close(info->ChildToParentPipes[SYSTEM2_FD_READ]) != 0)
            return SYSTEM2_RESULT_PIPE_FD_CLOSE_FAILED;

        //if(close(info->ParentToChildPipes[SYSTEM2_FD_WRITE]) != 0)
        //    return SYSTEM2_RESULT_PIPE_FD_CLOSE_FAILED;
        
        if(!WIFEXITED(status))
        {
            *outReturnCode = -1;
            return SYSTEM2_RESULT_COMMAND_TERMINATED;
        }

        *outReturnCode = WEXITSTATUS(status);
        return SYSTEM2_RESULT_SUCCESS;
    }
#endif

#if defined(_WIN32)
    SYSTEM2_FUNC_PREFIX SYSTEM2_RESULT System2RunWindows(   const char* command, 
                                                            System2CommandInfo* outCommandInfo)
    {
        SECURITY_ATTRIBUTES pipeSecurityAttributes; 
        
        pipeSecurityAttributes.nLength = sizeof(SECURITY_ATTRIBUTES); 
        pipeSecurityAttributes.bInheritHandle = TRUE;
        pipeSecurityAttributes.lpSecurityDescriptor = NULL; 

        // Create a pipe for the child process's STDOUT. 
        if(!CreatePipe( &outCommandInfo->ChildToParentPipes[SYSTEM2_FD_READ], 
                        &outCommandInfo->ChildToParentPipes[SYSTEM2_FD_WRITE], 
                        &pipeSecurityAttributes, 
                        0))
        {
            return SYSTEM2_RESULT_PIPE_CREATE_FAILED;
        }

        // Ensure the read handle to the pipe for STDOUT is not inherited.
        if(!SetHandleInformation(   outCommandInfo->ChildToParentPipes[SYSTEM2_FD_READ], 
                                    HANDLE_FLAG_INHERIT, 
                                    0))
        {
            return SYSTEM2_RESULT_PIPE_CREATE_FAILED;
        }

        // Create a pipe for the child process's STDIN. 
        if(!CreatePipe( &outCommandInfo->ParentToChildPipes[SYSTEM2_FD_READ], 
                        &outCommandInfo->ParentToChildPipes[SYSTEM2_FD_WRITE], 
                        &pipeSecurityAttributes, 
                        0))
        {
            return SYSTEM2_RESULT_PIPE_CREATE_FAILED;
        }

        // Ensure the write handle to the pipe for STDIN is not inherited. 
        if(!SetHandleInformation(   outCommandInfo->ParentToChildPipes[SYSTEM2_FD_WRITE], 
                                    HANDLE_FLAG_INHERIT, 
                                    0))
        {
            return SYSTEM2_RESULT_PIPE_CREATE_FAILED;
        }
        
        
        PROCESS_INFORMATION processInfo; 
        STARTUPINFO startupInfo;
        BOOL success = FALSE; 
        
        // Set up members of the PROCESS_INFORMATION structure. 
        ZeroMemory( &processInfo, sizeof(PROCESS_INFORMATION) );
        
        // Set up members of the STARTUPINFO structure. 
        // This structure specifies the STDIN and STDOUT handles for redirection.
        ZeroMemory( &startupInfo, sizeof(STARTUPINFO) );
        startupInfo.cb = sizeof(STARTUPINFO); 
        startupInfo.hStdError = outCommandInfo->ChildToParentPipes[SYSTEM2_FD_WRITE];
        startupInfo.hStdOutput = outCommandInfo->ChildToParentPipes[SYSTEM2_FD_WRITE];
        startupInfo.hStdInput = outCommandInfo->ParentToChildPipes[SYSTEM2_FD_READ];
        startupInfo.dwFlags |= STARTF_USESTDHANDLES;
        
        // Create the child process.
        // cmd /s /v /c "command"
        const char commandPrefix[] = "cmd /s /v /c ";
        const int commandLength = strlen(command);
        const int commandPrefixLength = strlen(commandPrefix);
        const int finalCommandLength = commandLength + commandPrefixLength + 2 + 1;
        
        char* commandCopy = (char*)malloc(finalCommandLength);
        if(commandCopy == NULL)
            return SYSTEM2_RESULT_COMMAND_CONSTRUCT_FAILED;

        errno_t result = strcpy_s(commandCopy, finalCommandLength, commandPrefix);
        if(result != 0)
        {
            free(commandCopy);
            return SYSTEM2_RESULT_COMMAND_CONSTRUCT_FAILED;
        }
        
        commandCopy[commandPrefixLength] = '"';
        result = strcpy_s(  commandCopy + commandPrefixLength + 1, 
                            finalCommandLength - commandPrefixLength - 1, 
                            command);
        
        if(result != 0)
        {
            free(commandCopy);
            return SYSTEM2_RESULT_COMMAND_CONSTRUCT_FAILED;
        }
        
        commandCopy[finalCommandLength - 2] = '"';
        commandCopy[finalCommandLength - 1] = '\0';
        
        success = CreateProcessA(   NULL, 
                                    commandCopy,    // command line 
                                    NULL,           // process security attributes 
                                    NULL,           // primary thread security attributes 
                                    TRUE,           // handles are inherited 
                                    0,              // creation flags 
                                    NULL,           // use parent's environment 
                                    NULL,           // use parent's current directory 
                                    &startupInfo,   // STARTUPINFO pointer 
                                    &processInfo);  // receives PROCESS_INFORMATION 
        
        free(commandCopy);
        
        // If an error occurs, exit the application. 
        if(!success)
            return SYSTEM2_RESULT_CREATE_CHILD_PROCESS_FAILED;
        else 
        {
            // Close handles to the child process and its primary thread.
            // Some applications might keep these handles to monitor the status
            // of the child process, for example. 
            outCommandInfo->ChildProcessHandle = processInfo.hProcess;
            if(!CloseHandle(processInfo.hThread))
                return SYSTEM2_RESULT_CREATE_CHILD_PROCESS_FAILED;

            // Close handles to the stdin and stdout pipes no longer needed by the child process.
            // If they are not explicitly closed, there is no way to recognize that the child process has ended.
            if(!CloseHandle(outCommandInfo->ChildToParentPipes[SYSTEM2_FD_WRITE]))
                return SYSTEM2_RESULT_PIPE_FD_CLOSE_FAILED;
            
            if(!CloseHandle(outCommandInfo->ParentToChildPipes[SYSTEM2_FD_READ]))
                return SYSTEM2_RESULT_PIPE_FD_CLOSE_FAILED;

            return SYSTEM2_RESULT_SUCCESS;
        }
    }
    
    SYSTEM2_FUNC_PREFIX SYSTEM2_RESULT System2ReadFromOutputWindows(const System2CommandInfo* info, 
                                                                    char* outputBuffer, 
                                                                    uint32_t outputBufferSize,
                                                                    uint32_t* outBytesRead)
    {
        DWORD readResult;
        BOOL bSuccess;
        *outBytesRead = 0;
        
        while (true)
        {
            bSuccess = ReadFile(info->ChildToParentPipes[SYSTEM2_FD_READ], 
                                outputBuffer, 
                                outputBufferSize - *outBytesRead, 
                                &readResult, 
                                NULL);

            if (readResult == 0)
                break;
            
            if(!bSuccess)
                return SYSTEM2_RESULT_READ_FAILED;
            
            outputBuffer += readResult;
            *outBytesRead += readResult;
            
            if(outputBufferSize - *outBytesRead == 0)
                return SYSTEM2_RESULT_READ_NOT_FINISHED;
        }

        //if(GetLastError() == ERROR_BROKEN_PIPE)
        //    return SYSTEM2_RESULT_SUCCESS;
        
        //return SYSTEM2_RESULT_READ_FAILED;
        return SYSTEM2_RESULT_SUCCESS;
    }
    
    SYSTEM2_FUNC_PREFIX SYSTEM2_RESULT System2WriteToInputWindows(  const System2CommandInfo* info, 
                                                                    const char* inputBuffer, 
                                                                    const uint32_t inputBufferSize)
    {
        DWORD currentWriteLengthLeft = inputBufferSize;
        BOOL bSuccess;

        while (true)
        {
            DWORD writeResult;

            bSuccess = WriteFile(   info->ParentToChildPipes[SYSTEM2_FD_WRITE], 
                                    inputBuffer, 
                                    currentWriteLengthLeft, 
                                    &writeResult, 
                                    NULL);

            if (!bSuccess)
                return SYSTEM2_RESULT_WRITE_FAILED;

            inputBuffer += writeResult;
            currentWriteLengthLeft -= writeResult;

            if (currentWriteLengthLeft == 0)
                break;
        }

        return SYSTEM2_RESULT_SUCCESS;
    }
    
    SYSTEM2_FUNC_PREFIX 
    SYSTEM2_RESULT System2GetCommandReturnValueAsyncWindows(const System2CommandInfo* info, 
                                                            int* outReturnCode)
    {
        DWORD exitCode;
        if(!GetExitCodeProcess(info->ChildProcessHandle, &exitCode))
            return SYSTEM2_RESULT_COMMAND_WAIT_ASYNC_FAILED;

        if(!CloseHandle(info->ChildToParentPipes[SYSTEM2_FD_READ]))
            return SYSTEM2_RESULT_PIPE_FD_CLOSE_FAILED;
        
        if(!CloseHandle(info->ParentToChildPipes[SYSTEM2_FD_WRITE]))
            return SYSTEM2_RESULT_PIPE_FD_CLOSE_FAILED;

        CloseHandle(info->ChildProcessHandle);
        *outReturnCode = exitCode;
        return SYSTEM2_RESULT_SUCCESS;
    }
    
    SYSTEM2_FUNC_PREFIX 
    SYSTEM2_RESULT System2GetCommandReturnValueSyncWindows( const System2CommandInfo* info, 
                                                            int* outReturnCode)
    {
        if(WaitForSingleObject(info->ChildProcessHandle, INFINITE) != 0)
            return SYSTEM2_RESULT_COMMAND_WAIT_SYNC_FAILED;
        
        if(!CloseHandle(info->ChildToParentPipes[SYSTEM2_FD_READ]))
            return SYSTEM2_RESULT_PIPE_FD_CLOSE_FAILED;
        
        if(!CloseHandle(info->ParentToChildPipes[SYSTEM2_FD_WRITE]))
            return SYSTEM2_RESULT_PIPE_FD_CLOSE_FAILED;
        
        DWORD exitCode;
        if(!GetExitCodeProcess(info->ChildProcessHandle, &exitCode))
        {
            *outReturnCode = -1;
            CloseHandle(info->ChildProcessHandle);
            return SYSTEM2_RESULT_COMMAND_TERMINATED;
        }
        
        CloseHandle(info->ChildProcessHandle);
        *outReturnCode = exitCode;
        return SYSTEM2_RESULT_SUCCESS;
    }
#endif

SYSTEM2_FUNC_PREFIX SYSTEM2_RESULT System2Run(const char* command, System2CommandInfo* outCommandInfo)
{
    #if defined(__unix__) || defined(__APPLE__)
        return System2RunPosix(command, outCommandInfo);
    #elif defined(_WIN32)
        return System2RunWindows(command, outCommandInfo);
    #else
        return SYSTEM2_RESULT_UNSUPPORTED_PLATFORM; 
    #endif
}

SYSTEM2_FUNC_PREFIX SYSTEM2_RESULT System2ReadFromOutput(   const System2CommandInfo* info, 
                                                            char* outputBuffer, 
                                                            uint32_t outputBufferSize,
                                                            uint32_t* outBytesRead)
{
    #if defined(__unix__) || defined(__APPLE__)
        return System2ReadFromOutputPosix(info, outputBuffer, outputBufferSize, outBytesRead);
    #elif defined(_WIN32)
        return System2ReadFromOutputWindows(info, outputBuffer, outputBufferSize, outBytesRead);
    #else
        return SYSTEM2_RESULT_UNSUPPORTED_PLATFORM; 
    #endif
}

SYSTEM2_FUNC_PREFIX SYSTEM2_RESULT System2WriteToInput( const System2CommandInfo* info, 
                                                        const char* inputBuffer, 
                                                        const uint32_t inputBufferSize)
{
    #if defined(__unix__) || defined(__APPLE__)
        return System2WriteToInputPosix(info, inputBuffer, inputBufferSize);
    #elif defined(_WIN32)
        return System2WriteToInputWindows(info, inputBuffer, inputBufferSize);
    #else
        return SYSTEM2_RESULT_UNSUPPORTED_PLATFORM; 
    #endif
}

SYSTEM2_FUNC_PREFIX 
SYSTEM2_RESULT System2GetCommandReturnValueAsync(   const System2CommandInfo* info, 
                                                    int* outReturnCode)
{
    #if defined(__unix__) || defined(__APPLE__)
        return System2GetCommandReturnValueAsyncPosix(info, outReturnCode);
    #elif defined(_WIN32)
        return System2GetCommandReturnValueAsyncWindows(info, outReturnCode);
    #else
        return SYSTEM2_RESULT_UNSUPPORTED_PLATFORM; 
    #endif
}

SYSTEM2_FUNC_PREFIX 
SYSTEM2_RESULT System2GetCommandReturnValueSync(const System2CommandInfo* info, 
                                                int* outReturnCode)
{
    #if defined(__unix__) || defined(__APPLE__)
        return System2GetCommandReturnValueSyncPosix(info, outReturnCode);
    #elif defined(_WIN32)
        return System2GetCommandReturnValueSyncWindows(info, outReturnCode);
    #else
        return SYSTEM2_RESULT_UNSUPPORTED_PLATFORM; 
    #endif
}

//SYSTEM2_DECLARATION_ONLY
#endif

//SYSTEM2_H
#endif 


