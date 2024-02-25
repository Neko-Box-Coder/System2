#ifndef SYSTEM2_H
#define SYSTEM2_H

//============================================================
//Declaration
//============================================================

#include <stdint.h>
#include <string.h>

#ifdef __unix__
    #include <unistd.h>
#endif

#ifdef _WIN32
    #include <windows.h>
#endif


typedef struct
{
    #ifdef __unix__
        int ParentToChildPipes[2];
        int ChildToParentPipes[2];
        pid_t ChildProcessID;
    #endif
    
    #ifdef _WIN32
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
    SYSTEM2_RESULT_UNSUPPORTED_PLATFORM = -8
} SYSTEM2_RESULT;

/*
Runs the command and stores the internal details in outCommandInfo

Could return the follow result:
- SYSTEM2_RESULT_SUCCESS
- SYSTEM2_RESULT_PIPE_CREATE_FAILED
- SYSTEM2_RESULT_FORK_FAILED
- SYSTEM2_RESULT_PIPE_FD_CLOSE_FAILED
*/
SYSTEM2_RESULT System2Run(const char* command, System2CommandInfo* outCommandInfo);

/*
Reads the output (stdout and stderr) from the command. 
Output string is **NOT** null terminated.

If SYSTEM2_RESULT_READ_NOT_FINISHED is returned, 
this function can be called again until SYSTEM2_RESULT_SUCCESS to retrieve the rest of the output.

Could return the follow result:
- SYSTEM2_RESULT_SUCCESS
- SYSTEM2_RESULT_READ_NOT_FINISHED
- SYSTEM2_RESULT_READ_FAILED
*/
SYSTEM2_RESULT System2ReadFromOutput(   System2CommandInfo* info, 
                                        char* outputBuffer, 
                                        uint32_t outputBufferSize,
                                        uint32_t* outBytesRead);

/*
Write the input (stdin) to the command. 

Could return the follow result:
- SYSTEM2_RESULT_SUCCESS
- SYSTEM2_RESULT_WRITE_FAILED
*/
SYSTEM2_RESULT System2WriteToInput( System2CommandInfo* info, 
                                    const char* inputBuffer, 
                                    const uint32_t inputBufferSize);

/*
Gets the command return code without waiting for the command to finish.
Returns SYSTEM2_RESULT_COMMAND_NOT_FINISHED if the command is still running.

Could return the follow result:
- SYSTEM2_RESULT_SUCCESS
- SYSTEM2_RESULT_COMMAND_NOT_FINISHED
- SYSTEM2_RESULT_COMMAND_TERMINATED
- SYSTEM2_RESULT_PIPE_FD_CLOSE_FAILED
- SYSTEM2_RESULT_COMMAND_WAIT_ASYNC_FAILED
*/
SYSTEM2_RESULT System2GetCommandReturnValueAsync(   System2CommandInfo* info, 
                                                    int* outReturnCode);

/*
Wait for the command to finish and gets the return code

Could return the follow result:
- SYSTEM2_RESULT_SUCCESS
- SYSTEM2_RESULT_COMMAND_TERMINATED
- SYSTEM2_RESULT_PIPE_FD_CLOSE_FAILED
- SYSTEM2_RESULT_COMMAND_WAIT_SYNC_FAILED
*/
SYSTEM2_RESULT System2GetCommandReturnValueSync(System2CommandInfo* info, 
                                                int* outReturnCode);


//============================================================
//Implementation
//============================================================

#include <stdlib.h>
#include <stdbool.h>

#ifdef __unix__
    #include <sys/wait.h>

    inline SYSTEM2_RESULT System2RunPosix(const char* command, System2CommandInfo* outCommandInfo)
    {
        int result = pipe(outCommandInfo->ParentToChildPipes);
        if(result != 0)
            return SYSTEM2_RESULT_PIPE_CREATE_FAILED;
        
        result = pipe(outCommandInfo->ChildToParentPipes);
        if(result != 0)
            return SYSTEM2_RESULT_PIPE_CREATE_FAILED;
        
        pid_t pid = fork();
        
        if(pid < 0)
            return SYSTEM2_RESULT_CREATE_CHILD_PROCESS_FAILED;
        //Child
        else if(pid == 0)
        {
            if(close(outCommandInfo->ParentToChildPipes[SYSTEM2_FD_WRITE]) != 0)
                exit(1);
            
            if(close(outCommandInfo->ChildToParentPipes[SYSTEM2_FD_READ]) != 0)
                exit(1);
            
            //close(STDIN_FILENO);
            //close(STDOUT_FILENO);
            //close(STDERR_FILENO);
            
            result = dup2(outCommandInfo->ParentToChildPipes[0], STDIN_FILENO);
            if(result == -1)
                exit(1);

            result = dup2(outCommandInfo->ChildToParentPipes[1], STDOUT_FILENO);
            if(result == -1)
                exit(1);
            
            result = dup2(outCommandInfo->ChildToParentPipes[1], STDERR_FILENO);
            if(result == -1)
                exit(1);
            
            execlp("sh", "sh", "-c", command, NULL);
            
            //Should never be reached
            
            exit(1);
        }
        //Parent
        else
        {
            if(close(outCommandInfo->ParentToChildPipes[SYSTEM2_FD_READ]) != 0)
                return SYSTEM2_RESULT_PIPE_FD_CLOSE_FAILED;
            
            if(close(outCommandInfo->ChildToParentPipes[SYSTEM2_FD_WRITE]) != 0)
                return SYSTEM2_RESULT_PIPE_FD_CLOSE_FAILED;
            
            outCommandInfo->ChildProcessID = pid;
        }
        
        return SYSTEM2_RESULT_SUCCESS;
    }
    
    inline SYSTEM2_RESULT System2ReadFromOutputPosix(   System2CommandInfo* info, 
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
    
    inline SYSTEM2_RESULT System2WriteToInputPosix( System2CommandInfo* info, 
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
    
    inline SYSTEM2_RESULT System2GetCommandReturnValueAsyncPosix(   System2CommandInfo* info, 
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
    
    inline SYSTEM2_RESULT System2GetCommandReturnValueSyncPosix(System2CommandInfo* info, 
                                                                int* outReturnCode)
    {
        int status;
        pid_t pidResult = waitpid(info->ChildProcessID, &status, 0);
        
        if(pidResult == -1)
            return SYSTEM2_RESULT_COMMAND_WAIT_SYNC_FAILED;
        
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
#endif

#ifdef _WIN32

    #include <stdio.h>
    
    inline SYSTEM2_RESULT System2RunWindows(const char* command, System2CommandInfo* outCommandInfo)
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
            return SYSTEM2_RESULT_CREATE_CHILD_PROCESS_FAILED;

        errno_t result = strcpy_s(commandCopy, finalCommandLength, commandPrefix);
        if(result != 0)
        {
            printf("Error copying command string %i\n", result);
            free(commandCopy);
            return SYSTEM2_RESULT_CREATE_CHILD_PROCESS_FAILED;
        }
        
        commandCopy[commandPrefixLength] = '"';
        result = strcpy_s(commandCopy + commandPrefixLength + 1, finalCommandLength - commandPrefixLength - 1, command);
        if(result != 0)
        {
            printf("Error copying command string %i\n", result);
            free(commandCopy);
            return SYSTEM2_RESULT_CREATE_CHILD_PROCESS_FAILED;
        }
        
        commandCopy[finalCommandLength - 2] = '"';
        commandCopy[finalCommandLength - 1] = '\0';
        //printf("Command: %s\n", commandCopy);
        
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
        {
            printf("Error CreateProcessA\n");
            DWORD dw = GetLastError();
            
            char buf[256];
            FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
               NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), 
               buf, (sizeof(buf) / sizeof(char)), NULL);
            
            printf("Error: %s\n", buf);
            
            return SYSTEM2_RESULT_CREATE_CHILD_PROCESS_FAILED;
        } 
        else 
        {
            // Close handles to the child process and its primary thread.
            // Some applications might keep these handles to monitor the status
            // of the child process, for example. 
            outCommandInfo->ChildProcessHandle = processInfo.hProcess;
            if(!CloseHandle(processInfo.hThread))
            {
                printf("Error CloseHandle\n");
                return SYSTEM2_RESULT_CREATE_CHILD_PROCESS_FAILED;
            }
            
            // Close handles to the stdin and stdout pipes no longer needed by the child process.
            // If they are not explicitly closed, there is no way to recognize that the child process has ended.
            if(!CloseHandle(outCommandInfo->ChildToParentPipes[SYSTEM2_FD_WRITE]))
                return SYSTEM2_RESULT_PIPE_FD_CLOSE_FAILED;
            
            if(!CloseHandle(outCommandInfo->ParentToChildPipes[SYSTEM2_FD_READ]))
                return SYSTEM2_RESULT_PIPE_FD_CLOSE_FAILED;

            return SYSTEM2_RESULT_SUCCESS;
        }
    }
    
    inline SYSTEM2_RESULT System2ReadFromOutputWindows( System2CommandInfo* info, 
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
    
    inline SYSTEM2_RESULT System2WriteToInputWindows(   System2CommandInfo* info, 
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
    
    inline SYSTEM2_RESULT System2GetCommandReturnValueAsyncWindows(   System2CommandInfo* info, 
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
    
    inline SYSTEM2_RESULT System2GetCommandReturnValueSyncWindows(  System2CommandInfo* info, 
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

inline SYSTEM2_RESULT System2Run(const char* command, System2CommandInfo* outCommandInfo)
{
    #ifdef __unix__
        return System2RunPosix(command, outCommandInfo);
    #elif defined(_WIN32)
        return System2RunWindows(command, outCommandInfo);
    #else
        return SYSTEM2_RESULT_UNSUPPORTED_PLATFORM; 
    #endif
}

#ifdef __unix__

#endif

#ifdef _WIN32
    
#endif


inline SYSTEM2_RESULT System2ReadFromOutput(System2CommandInfo* info, 
                                            char* outputBuffer, 
                                            uint32_t outputBufferSize,
                                            uint32_t* outBytesRead)
{
    #ifdef __unix__
        return System2ReadFromOutputPosix(info, outputBuffer, outputBufferSize, outBytesRead);
    #elif defined(_WIN32)
        return System2ReadFromOutputWindows(info, outputBuffer, outputBufferSize, outBytesRead);
    #else
        return SYSTEM2_RESULT_UNSUPPORTED_PLATFORM; 
    #endif
}

inline SYSTEM2_RESULT System2WriteToInput(  System2CommandInfo* info, 
                                            const char* inputBuffer, 
                                            const uint32_t inputBufferSize)
{
    #ifdef __unix__
        return System2WriteToInputPosix(info, inputBuffer, inputBufferSize);
    #elif defined(_WIN32)
        return System2WriteToInputWindows(info, inputBuffer, inputBufferSize);
    #else
        return SYSTEM2_RESULT_UNSUPPORTED_PLATFORM; 
    #endif
}

inline SYSTEM2_RESULT System2GetCommandReturnValueAsync(System2CommandInfo* info, 
                                                        int* outReturnCode)
{
    #ifdef __unix__
        return System2GetCommandReturnValueAsyncPosix(info, outReturnCode);
    #elif defined(_WIN32)
        return System2GetCommandReturnValueAsyncWindows(info, outReturnCode);
    #else
        return SYSTEM2_RESULT_UNSUPPORTED_PLATFORM; 
    #endif
}

inline SYSTEM2_RESULT System2GetCommandReturnValueSync( System2CommandInfo* info, 
                                                        int* outReturnCode)
{
    #ifdef __unix__
        return System2GetCommandReturnValueSyncPosix(info, outReturnCode);
    #elif defined(_WIN32)
        return System2GetCommandReturnValueSyncWindows(info, outReturnCode);
    #else
        return SYSTEM2_RESULT_UNSUPPORTED_PLATFORM; 
    #endif
}


#if 0
void tempLinux()
{
    int       parentToChild[2];
    int       childToParent[2];
    pid_t     pid;
    string    dataReadFromChild;
    char      buffer[BUFFER_SIZE + 1];
    ssize_t   readResult;
    int       status;
    
    #define ASSERT_IS(shouldBe, value) \
        if (value != shouldBe) { printf("Value: %d\n", value); exit(-1); }

    #define ASSERT_NOT(notToBe, value) \
        if (value == notToBe) { printf("Value: %d\n", value); exit(-1); }

    ASSERT_IS(0, pipe(parentToChild));
    ASSERT_IS(0, pipe(childToParent));

    switch (pid = fork())
    {
        case -1:
            printf("Fork failed\n");
            exit(-1);

        case 0: /* Child */
            ASSERT_NOT(-1, dup2(parentToChild[READ_FD], STDIN_FILENO));
            ASSERT_NOT(-1, dup2(childToParent[WRITE_FD], STDOUT_FILENO));
            ASSERT_NOT(-1, dup2(childToParent[WRITE_FD], STDERR_FILENO));
            
            ASSERT_IS(0, close(parentToChild [WRITE_FD]));
            ASSERT_IS(0, close(childToParent [READ_FD]));

            /*     file, arg0, arg1,  arg2 */
            execlp("ls", "ls", "-al", "--color", (char *) 0);

            printf("This line should never be reached!!!\n");
            exit(-1);

        default: /* Parent */
            printf("Child %d process running...\n",  pid);

            ASSERT_IS(0, close(parentToChild [READ_FD]));
            ASSERT_IS(0, close(childToParent [WRITE_FD]));

            while (true)
            {
                switch (readResult = read(  childToParent[READ_FD],
                                            buffer, BUFFER_SIZE))
                {
                case 0: /* End-of-File, or non-blocking read. */
                    cout << "End of file reached..."         << endl
                        << "Data received was ("
                        << dataReadFromChild.size() << "): " << endl
                        << dataReadFromChild                << endl;

                    ASSERT_IS(pid, waitpid(pid, & status, 0));

                    cout << endl
                        << "Child exit staus is:  " << WEXITSTATUS(status) << endl
                        << endl;

                    exit(0);


                case -1:
                    if ((errno == EINTR) || (errno == EAGAIN))
                    {
                    errno = 0;
                    break;
                    }
                    else
                    {
                    FAIL("read() failed");
                    exit(-1);
                    }

                default:
                    dataReadFromChild . append(buffer, readResult);
                    break;
                }
            } /* while (true) */
    } /* switch (pid = fork())*/

}
#endif

#if 0
//https://learn.microsoft.com/en-us/windows/win32/procthread/creating-a-child-process-with-redirected-input-and-output

#include <windows.h> 
#include <tchar.h>
#include <stdio.h> 
#include <strsafe.h>

#define BUFSIZE 4096 
 
HANDLE g_hChildStd_IN_Rd = NULL;
HANDLE g_hChildStd_IN_Wr = NULL;
HANDLE g_hChildStd_OUT_Rd = NULL;
HANDLE g_hChildStd_OUT_Wr = NULL;

HANDLE g_hInputFile = NULL;
 
void CreateChildProcess(void); 
void WriteToPipe(void); 
void ReadFromPipe(void); 
void ErrorExit(PTSTR); 
 
int _tmain(int argc, TCHAR *argv[]) 
{ 
   SECURITY_ATTRIBUTES saAttr; 
 
   printf("\n->Start of parent execution.\n");

// Set the bInheritHandle flag so pipe handles are inherited. 
 
   saAttr.nLength = sizeof(SECURITY_ATTRIBUTES); 
   saAttr.bInheritHandle = TRUE; 
   saAttr.lpSecurityDescriptor = NULL; 

// Create a pipe for the child process's STDOUT. 
 
   if ( ! CreatePipe(&g_hChildStd_OUT_Rd, &g_hChildStd_OUT_Wr, &saAttr, 0) ) 
      ErrorExit(TEXT("StdoutRd CreatePipe")); 

// Ensure the read handle to the pipe for STDOUT is not inherited.

   if ( ! SetHandleInformation(g_hChildStd_OUT_Rd, HANDLE_FLAG_INHERIT, 0) )
      ErrorExit(TEXT("Stdout SetHandleInformation")); 

// Create a pipe for the child process's STDIN. 
 
   if (! CreatePipe(&g_hChildStd_IN_Rd, &g_hChildStd_IN_Wr, &saAttr, 0)) 
      ErrorExit(TEXT("Stdin CreatePipe")); 

// Ensure the write handle to the pipe for STDIN is not inherited. 
 
   if ( ! SetHandleInformation(g_hChildStd_IN_Wr, HANDLE_FLAG_INHERIT, 0) )
      ErrorExit(TEXT("Stdin SetHandleInformation")); 
 
// Create the child process. 
   
   CreateChildProcess();

// Get a handle to an input file for the parent. 
// This example assumes a plain text file and uses string output to verify data flow. 
 
   if (argc == 1) 
      ErrorExit(TEXT("Please specify an input file.\n")); 

   g_hInputFile = CreateFile(
       argv[1], 
       GENERIC_READ, 
       0, 
       NULL, 
       OPEN_EXISTING, 
       FILE_ATTRIBUTE_READONLY, 
       NULL); 

   if ( g_hInputFile == INVALID_HANDLE_VALUE ) 
      ErrorExit(TEXT("CreateFile")); 
 
// Write to the pipe that is the standard input for a child process. 
// Data is written to the pipe's buffers, so it is not necessary to wait
// until the child process is running before writing data.
 
   WriteToPipe(); 
   printf( "\n->Contents of %S written to child STDIN pipe.\n", argv[1]);
 
// Read from pipe that is the standard output for child process. 
 
   printf( "\n->Contents of child process STDOUT:\n\n");
   ReadFromPipe(); 

   printf("\n->End of parent execution.\n");

// The remaining open handles are cleaned up when this process terminates. 
// To avoid resource leaks in a larger application, close handles explicitly. 

   return 0; 
} 
 
void CreateChildProcess()
// Create a child process that uses the previously created pipes for STDIN and STDOUT.
{ 
   TCHAR szCmdline[]=TEXT("child");
   PROCESS_INFORMATION piProcInfo; 
   STARTUPINFO siStartInfo;
   BOOL bSuccess = FALSE; 
 
// Set up members of the PROCESS_INFORMATION structure. 
 
   ZeroMemory( &piProcInfo, sizeof(PROCESS_INFORMATION) );
 
// Set up members of the STARTUPINFO structure. 
// This structure specifies the STDIN and STDOUT handles for redirection.
 
   ZeroMemory( &siStartInfo, sizeof(STARTUPINFO) );
   siStartInfo.cb = sizeof(STARTUPINFO); 
   siStartInfo.hStdError = g_hChildStd_OUT_Wr;
   siStartInfo.hStdOutput = g_hChildStd_OUT_Wr;
   siStartInfo.hStdInput = g_hChildStd_IN_Rd;
   siStartInfo.dwFlags |= STARTF_USESTDHANDLES;
 
// Create the child process. 
    
   bSuccess = CreateProcess(NULL, 
      szCmdline,     // command line 
      NULL,          // process security attributes 
      NULL,          // primary thread security attributes 
      TRUE,          // handles are inherited 
      0,             // creation flags 
      NULL,          // use parent's environment 
      NULL,          // use parent's current directory 
      &siStartInfo,  // STARTUPINFO pointer 
      &piProcInfo);  // receives PROCESS_INFORMATION 
   
   // If an error occurs, exit the application. 
   if ( ! bSuccess ) 
      ErrorExit(TEXT("CreateProcess"));
   else 
   {
      // Close handles to the child process and its primary thread.
      // Some applications might keep these handles to monitor the status
      // of the child process, for example. 

      CloseHandle(piProcInfo.hProcess);
      CloseHandle(piProcInfo.hThread);
      
      // Close handles to the stdin and stdout pipes no longer needed by the child process.
      // If they are not explicitly closed, there is no way to recognize that the child process has ended.
      
      CloseHandle(g_hChildStd_OUT_Wr);
      CloseHandle(g_hChildStd_IN_Rd);
   }
}
 
void WriteToPipe(void) 

// Read from a file and write its contents to the pipe for the child's STDIN.
// Stop when there is no more data. 
{ 
   DWORD dwRead, dwWritten; 
   CHAR chBuf[BUFSIZE];
   BOOL bSuccess = FALSE;
 
   for (;;) 
   { 
      bSuccess = ReadFile(g_hInputFile, chBuf, BUFSIZE, &dwRead, NULL);
      if ( ! bSuccess || dwRead == 0 ) break; 
      
      bSuccess = WriteFile(g_hChildStd_IN_Wr, chBuf, dwRead, &dwWritten, NULL);
      if ( ! bSuccess ) break; 
   } 
 
// Close the pipe handle so the child process stops reading. 
 
   if ( ! CloseHandle(g_hChildStd_IN_Wr) ) 
      ErrorExit(TEXT("StdInWr CloseHandle")); 
} 
 
void ReadFromPipe(void) 

// Read output from the child process's pipe for STDOUT
// and write to the parent process's pipe for STDOUT. 
// Stop when there is no more data. 
{ 
   DWORD dwRead, dwWritten; 
   CHAR chBuf[BUFSIZE]; 
   BOOL bSuccess = FALSE;
   HANDLE hParentStdOut = GetStdHandle(STD_OUTPUT_HANDLE);

   for (;;) 
   { 
      bSuccess = ReadFile( g_hChildStd_OUT_Rd, chBuf, BUFSIZE, &dwRead, NULL);
      if( ! bSuccess || dwRead == 0 ) break; 

      bSuccess = WriteFile(hParentStdOut, chBuf, 
                           dwRead, &dwWritten, NULL);
      if (! bSuccess ) break; 
   } 
} 
 
void ErrorExit(PTSTR lpszFunction) 

// Format a readable error message, display a message box, 
// and exit from the application.
{ 
    LPVOID lpMsgBuf;
    LPVOID lpDisplayBuf;
    DWORD dw = GetLastError(); 

    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | 
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        dw,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR) &lpMsgBuf,
        0, NULL );

    lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT, 
        (lstrlen((LPCTSTR)lpMsgBuf)+lstrlen((LPCTSTR)lpszFunction)+40)*sizeof(TCHAR)); 
    StringCchPrintf((LPTSTR)lpDisplayBuf, 
        LocalSize(lpDisplayBuf) / sizeof(TCHAR),
        TEXT("%s failed with error %d: %s"), 
        lpszFunction, dw, lpMsgBuf); 
    MessageBox(NULL, (LPCTSTR)lpDisplayBuf, TEXT("Error"), MB_OK); 

    LocalFree(lpMsgBuf);
    LocalFree(lpDisplayBuf);
    ExitProcess(1);
}

#elif 0

void tempWindows()
{
    wchar_t command[] = L"nslookup myip.opendns.com. resolver1.opendns.com";

    wchar_t cmd[MAX_PATH] ;
    wchar_t cmdline[ MAX_PATH + 50 ];
    swprintf_s( cmdline, L"%s /c %s", cmd, command );

    STARTUPINFOW startInf;
    memset( &startInf, 0, sizeof startInf );
    startInf.cb = sizeof(startInf);

    PROCESS_INFORMATION procInf;
    memset( &procInf, 0, sizeof procInf );

    BOOL b = CreateProcessW( NULL, cmdline, NULL, NULL, FALSE,
        NORMAL_PRIORITY_CLASS | CREATE_NO_WINDOW, NULL, NULL, &startInf, &procInf );

    DWORD dwErr = 0;
    if( b ) 
    {
        // Wait till process completes
        WaitForSingleObject( procInf.hProcess, INFINITE );
        // Check process’s exit code
        GetExitCodeProcess( procInf.hProcess, &dwErr );
        // Avoid memory leak by closing process handle
        CloseHandle( procInf.hProcess );
    } 
    else 
    {
        dwErr = GetLastError();
    }
    if( dwErr ) 
    {
        wprintf(_T("Command failed. Error %d\n"),dwErr);
    }

}
#endif


#endif