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
    //We need system types defined if we don't want to include system headers
    #if defined(__unix__) || defined(__APPLE__)
        typedef int pid_t;
    #endif
    
    #if defined(_WIN32)
        typedef void* HANDLE;
    #endif
#else
    //Includes all the required system headers
    #if defined(__unix__) || defined(__APPLE__)
        #include <unistd.h>
        #include <stdio.h>
    #endif

    #if defined(_WIN32)
        #ifndef _CRT_SECURE_NO_WARNINGS
            #define _CRT_SECURE_NO_WARNINGS
            #define INTERNAL_SYSTEM2_APPLY_NO_WARNINGS 1
        #endif
        
        #if !defined(NOMINMAX)
            #define NOMINMAX 1
        #endif
        
        #include <windows.h>
    #endif
#endif

//TODO: MSVC dllimport dllexport
#if SYSTEM2_DECLARATION_ONLY || SYSTEM2_IMPLEMENTATION_ONLY
    //We don't need any inline prefix if we are having declaration and implementation separated
    #define SYSTEM2_FUNC_PREFIX
#else
    #define SYSTEM2_FUNC_PREFIX static inline
#endif

#include <stdint.h>
#include <string.h>
#include <stdbool.h>

typedef struct
{
    bool RedirectInput;         //Redirect input with pipe?
    bool RedirectOutput;        //Redirect output with pipe?
    const char* RunDirectory;   //The directory to run the command in?
    
    #if defined(__unix__) || defined(__APPLE__)
        int ParentToChildPipes[2];
        int ChildToParentPipes[2];
        pid_t ChildProcessID;
    #endif
    
    #if defined(_WIN32)
        bool DisableEscapes;    //Disable automatic escaping?
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
    SYSTEM2_RESULT_COMMAND_CONSTRUCT_FAILED = -9,
    SYSTEM2_RESULT_POSIX_SPAWN_FILE_ACTION_DESTROY_FAILED = -10,
    SYSTEM2_RESULT_POSIX_SPAWN_FILE_ACTION_DUP2_FAILED = -11,
    SYSTEM2_RESULT_POSIX_SPAWN_RUN_DIRECTORY_NOT_SUPPORTED = -12,
    SYSTEM2_RESULT_INVALID_ARGUMENT = -13,
    SYSTEM2_RESULT_MALLOC_FAILED = -14,
    SYSTEM2_RESULT_WINDOWS_UNICODE_FAILED = -15,
    SYSTEM2_RESULT_WINDOWS_SET_ENV_FAILED = -16,
} SYSTEM2_RESULT;

/*
Runs the command in system shell just like the `system()` funcion with the given settings 
    passed with `inOutCommandInfo`.

This uses 
`sh -c command` for POSIX and
`cmd /s /v /c command` for Windows

Could return the following result:
- SYSTEM2_RESULT_SUCCESS
- SYSTEM2_RESULT_PIPE_CREATE_FAILED
- SYSTEM2_RESULT_CREATE_CHILD_PROCESS_FAILED
- SYSTEM2_RESULT_PIPE_FD_CLOSE_FAILED
- SYSTEM2_RESULT_COMMAND_CONSTRUCT_FAILED
- SYSTEM2_RESULT_POSIX_SPAWN_FILE_ACTION_DESTROY_FAILED
- SYSTEM2_RESULT_POSIX_SPAWN_FILE_ACTION_DUP2_FAILED
- SYSTEM2_RESULT_POSIX_SPAWN_RUN_DIRECTORY_NOT_SUPPORTED
- SYSTEM2_RESULT_INVALID_ARGUMENT
*/
SYSTEM2_FUNC_PREFIX SYSTEM2_RESULT System2Run(  const char* command, 
                                                System2CommandInfo* inOutCommandInfo);

/*
Runs the executable (which can search in PATH env variable) with the given arguments and settings
    passed with inOutCommandInfo.

On Windows, automatic escaping can be removed by setting the `DisableEscape` in `inOutCommandInfo`

Could return the following result:
- SYSTEM2_RESULT_SUCCESS
- SYSTEM2_RESULT_PIPE_CREATE_FAILED
- SYSTEM2_RESULT_CREATE_CHILD_PROCESS_FAILED
- SYSTEM2_RESULT_PIPE_FD_CLOSE_FAILED
- SYSTEM2_RESULT_COMMAND_CONSTRUCT_FAILED
- SYSTEM2_RESULT_POSIX_SPAWN_FILE_ACTION_DESTROY_FAILED
- SYSTEM2_RESULT_POSIX_SPAWN_FILE_ACTION_DUP2_FAILED
- SYSTEM2_RESULT_POSIX_SPAWN_RUN_DIRECTORY_NOT_SUPPORTED
- SYSTEM2_RESULT_INVALID_ARGUMENT
- SYSTEM2_RESULT_WINDOWS_UNICODE_FAILED
- SYSTEM2_RESULT_MALLOC_FAILED
*/
SYSTEM2_FUNC_PREFIX SYSTEM2_RESULT System2RunSubprocess(const char* executable,
                                                        const char* const* args,
                                                        int argsCount,
                                                        System2CommandInfo* inOutCommandInfo);


/*
Reads the output (stdout and stderr) from the command. 
Output string is **NOT** null terminated.

If SYSTEM2_RESULT_READ_NOT_FINISHED is returned, 
this function can be called again until SYSTEM2_RESULT_SUCCESS to retrieve the rest of the output.

outBytesRead determines how many bytes have been read for **this** function call

Could return the following result:
- SYSTEM2_RESULT_SUCCESS
- SYSTEM2_RESULT_READ_NOT_FINISHED
- SYSTEM2_RESULT_READ_FAILED
- SYSTEM2_RESULT_INVALID_ARGUMENT
*/
SYSTEM2_FUNC_PREFIX SYSTEM2_RESULT System2ReadFromOutput(   const System2CommandInfo* info, 
                                                            char* outputBuffer, 
                                                            uint32_t outputBufferSize,
                                                            uint32_t* outBytesRead);

/*
Write the input (stdin) to the command. 

Could return the following result:
- SYSTEM2_RESULT_SUCCESS
- SYSTEM2_RESULT_WRITE_FAILED
- SYSTEM2_RESULT_INVALID_ARGUMENT
*/
SYSTEM2_FUNC_PREFIX SYSTEM2_RESULT System2WriteToInput( const System2CommandInfo* info, 
                                                        const char* inputBuffer, 
                                                        const uint32_t inputBufferSize);


//TODO: Might want to add this to have this ability to close input pipe manually
//SYSTEM2_FUNC_PREFIX SYSTEM2_RESULT System2CloseInput(System2CommandInfo* info);

/*
Cleanup any open handles associated with the command.

Could return the following result:
- SYSTEM2_RESULT_SUCCESS
- SYSTEM2_RESULT_PIPE_FD_CLOSE_FAILED
- SYSTEM2_RESULT_INVALID_ARGUMENT
*/
SYSTEM2_FUNC_PREFIX SYSTEM2_RESULT System2CleanupCommand(const System2CommandInfo* info);

/*
Gets the return code if the command has finished.
Otherwise, this will return SYSTEM2_RESULT_COMMAND_NOT_FINISHED immediately.

If `manualCleanup` is false, 
`System2CleanupCommand()` is automatically called when the command has exited.

Otherwise, `System2CleanupCommand()` should be called when the command has exited.

Could return the following result:
- SYSTEM2_RESULT_SUCCESS
- SYSTEM2_RESULT_COMMAND_NOT_FINISHED
- SYSTEM2_RESULT_COMMAND_TERMINATED
- SYSTEM2_RESULT_PIPE_FD_CLOSE_FAILED
- SYSTEM2_RESULT_COMMAND_WAIT_ASYNC_FAILED
- SYSTEM2_RESULT_INVALID_ARGUMENT
*/
SYSTEM2_FUNC_PREFIX 
SYSTEM2_RESULT System2GetCommandReturnValueAsync(   const System2CommandInfo* info, 
                                                    int* outReturnCode,
                                                    bool manualCleanup);

/*
Wait for the command to finish and gets the return code

If `manualCleanup` is false, 
`System2CleanupCommand()` is automatically called when the command has exited.

Otherwise, `System2CleanupCommand()` should be called when the command has exited.

Could return the following result:
- SYSTEM2_RESULT_SUCCESS
- SYSTEM2_RESULT_COMMAND_TERMINATED
- SYSTEM2_RESULT_PIPE_FD_CLOSE_FAILED
- SYSTEM2_RESULT_COMMAND_WAIT_SYNC_FAILED
- SYSTEM2_RESULT_INVALID_ARGUMENT
*/
SYSTEM2_FUNC_PREFIX SYSTEM2_RESULT System2GetCommandReturnValueSync(const System2CommandInfo* info, 
                                                                    int* outReturnCode,
                                                                    bool manualCleanup);

/*
Returns the count of environment variables, along with a resource handle which can be used to 
access the environment variable values with `System2GetEnvironmentVariables()`.

The resource handle should be freed with `System2EnvironmentVariableFree()` when done.

NOTE: If you need to get a particular environment variable without iteration, use `getenv()` from the
      standard library.

Could return the following result:
- SYSTEM2_RESULT_SUCCESS
- SYSTEM2_RESULT_INVALID_ARGUMENT
- SYSTEM2_RESULT_MALLOC_FAILED
*/
SYSTEM2_FUNC_PREFIX 
SYSTEM2_RESULT System2GetEnvironmentVariablesCount(int* outCount, void** outResource);

/*
Returns the environment variable name and value for a given index. The behavior is undefined if 
trying to index an environment variable outside of bound.

The content of the returned environment name and value should be copied to a local buffer 
immediately as changes to the environment variable might invalidate them.

NOTE: If you need to get a particular environment variable without iteration, use `getenv()` from the
      standard library.

Could return the following result:
- SYSTEM2_RESULT_SUCCESS
- SYSTEM2_RESULT_INVALID_ARGUMENT
*/
SYSTEM2_FUNC_PREFIX SYSTEM2_RESULT System2GetEnvironmentVariable(   const void* resource,
                                                                    const char** outName,
                                                                    int* outNameLength,
                                                                    const char** outValue,
                                                                    int* outValueLength,
                                                                    int index);

/*
Free the resource handle created by `System2GetEnvironmentVariablesCount()` and set it to NULL.

Could return the following result:
- SYSTEM2_RESULT_SUCCESS
- SYSTEM2_RESULT_INVALID_ARGUMENT
*/
SYSTEM2_FUNC_PREFIX SYSTEM2_RESULT System2EnvironmentVariableFree(void** resource);

/*
Sets/unsets an environment variable where it is unset if `envValue` is `NULL`.
`envName` must be valid for the platform otherwise this function will fail.

If the environment variable with `envName` already exists when trying to set or not exists when 
trying to unset, this function MIGHT fail depending on the platform.

To make sure the environement variable is correctly set, you should get the environment variable.

Could return the following result:
- SYSTEM2_RESULT_SUCCESS
- SYSTEM2_RESULT_INVALID_ARGUMENT
- SYSTEM2_RESULT_WINDOWS_UNICODE_FAILED
- SYSTEM2_RESULT_MALLOC_FAILED
- SYSTEM2_RESULT_WINDOWS_SET_ENV_FAILED
*/
SYSTEM2_FUNC_PREFIX 
SYSTEM2_RESULT System2SetEnvironmentVariable(const char* envName, const char* envValue);


//============================================================
//Implementation
//============================================================

#if !SYSTEM2_DECLARATION_ONLY

#include <stdlib.h>
#include <stdbool.h>

#if defined(__unix__) || defined(__APPLE__)
    #include <sys/wait.h>
    extern char** environ;
    
    //This bypasses inheriting memory from parent process (glibc 2.24) but removes the rundir feature
    //#define SYSTEM2_POSIX_SPAWN 1
    #if defined(SYSTEM2_POSIX_SPAWN) && SYSTEM2_POSIX_SPAWN != 0
        #include <spawn.h>
    #endif

    SYSTEM2_FUNC_PREFIX 
    SYSTEM2_RESULT System2RunSubprocessPosix(   const char* executable,
                                                const char* const* args,
                                                int argsCount,
                                                System2CommandInfo* inOutCommandInfo)
    {
        if(!executable || !args || !inOutCommandInfo)
            return SYSTEM2_RESULT_INVALID_ARGUMENT;
        
        int result = pipe(inOutCommandInfo->ParentToChildPipes);
        if(result != 0)
            return SYSTEM2_RESULT_PIPE_CREATE_FAILED;
        
        result = pipe(inOutCommandInfo->ChildToParentPipes);
        if(result != 0)
            return SYSTEM2_RESULT_PIPE_CREATE_FAILED;

        const char** nullTerminatedArgs = calloc(argsCount + 1, sizeof(char**));
        if(nullTerminatedArgs == NULL)
            return SYSTEM2_RESULT_COMMAND_CONSTRUCT_FAILED;
        
        for(int i = 0; i < argsCount; ++i)
            nullTerminatedArgs[i] = args[i];
        
        nullTerminatedArgs[argsCount] = NULL;
        
        #if !defined(SYSTEM2_POSIX_SPAWN) || SYSTEM2_POSIX_SPAWN == 0
            pid_t pid = fork();
        
            if(pid < 0)
            {
                free(nullTerminatedArgs);
                return SYSTEM2_RESULT_CREATE_CHILD_PROCESS_FAILED;
            }
            //Child
            else if(pid == 0)
            {
                if(close(inOutCommandInfo->ParentToChildPipes[SYSTEM2_FD_WRITE]) != 0)
                    _exit(2);
                
                if(close(inOutCommandInfo->ChildToParentPipes[SYSTEM2_FD_READ]) != 0)
                    _exit(3);
                
                if(inOutCommandInfo->RunDirectory != NULL)
                {
                    if(chdir(inOutCommandInfo->RunDirectory) != 0)
                        _exit(4);
                }
                
                if(inOutCommandInfo->RedirectInput)
                {
                    result = dup2(  inOutCommandInfo->ParentToChildPipes[SYSTEM2_FD_READ], 
                                    STDIN_FILENO);
                    if(result == -1)
                        _exit(5);
                }

                if(inOutCommandInfo->RedirectOutput)
                {
                    result = dup2(  inOutCommandInfo->ChildToParentPipes[SYSTEM2_FD_WRITE], 
                                    STDOUT_FILENO);
                    if(result == -1)
                        _exit(6);
                    
                    result = dup2(  inOutCommandInfo->ChildToParentPipes[SYSTEM2_FD_WRITE], 
                                    STDERR_FILENO);
                    if(result == -1)
                        _exit(7);
                }
                
                //TODO: Send the errno back to the host and display the error
                if(execvp(executable, (char**)nullTerminatedArgs) == -1)
                    _exit(52);
                
                //Should never be reached
                
                _exit(8);
            }
        #else //SYSTEM2_POSIX_SPAWN
            posix_spawn_file_actions_t file_actions;
            posix_spawn_file_actions_init(&file_actions);

            //Close unused pipe ends in the child process
            int* parentToChildPipes = inOutCommandInfo->ParentToChildPipes;
            if(posix_spawn_file_actions_addclose(   &file_actions, 
                                                    parentToChildPipes[SYSTEM2_FD_WRITE]) != 0) 
            {
                posix_spawn_file_actions_destroy(&file_actions);
                free(nullTerminatedArgs);
                return SYSTEM2_RESULT_POSIX_SPAWN_FILE_ACTION_DESTROY_FAILED;
            }

            int* childToParentPipes = inOutCommandInfo->ChildToParentPipes;
            if(posix_spawn_file_actions_addclose(   &file_actions, 
                                                    childToParentPipes[SYSTEM2_FD_READ]) != 0) 
            {
                posix_spawn_file_actions_destroy(&file_actions);
                free(nullTerminatedArgs);
                return SYSTEM2_RESULT_POSIX_SPAWN_FILE_ACTION_DESTROY_FAILED;
            }

            //Redirect input
            if(inOutCommandInfo->RedirectInput)
            {
                if(posix_spawn_file_actions_adddup2(&file_actions,
                                                    parentToChildPipes[SYSTEM2_FD_READ],
                                                    STDIN_FILENO) != 0) 
                {
                    posix_spawn_file_actions_destroy(&file_actions);
                    free(nullTerminatedArgs);
                    return SYSTEM2_RESULT_POSIX_SPAWN_FILE_ACTION_DUP2_FAILED;
                }
            }

            //Redirect output
            if(inOutCommandInfo->RedirectOutput)
            { 
                if(posix_spawn_file_actions_adddup2(&file_actions,
                                                    childToParentPipes[SYSTEM2_FD_WRITE],
                                                    STDOUT_FILENO) != 0)
                {
                    posix_spawn_file_actions_destroy(&file_actions);
                    free(nullTerminatedArgs);
                    return SYSTEM2_RESULT_POSIX_SPAWN_FILE_ACTION_DUP2_FAILED;
                }

                if(posix_spawn_file_actions_adddup2(&file_actions,
                                                    childToParentPipes[SYSTEM2_FD_WRITE],
                                                    STDERR_FILENO) != 0)
                {
                    posix_spawn_file_actions_destroy(&file_actions);
                    free(nullTerminatedArgs);
                    return SYSTEM2_RESULT_POSIX_SPAWN_FILE_ACTION_DUP2_FAILED;
                }
            }

            //Close the duplicated file descriptors
            posix_spawn_file_actions_addclose(&file_actions, parentToChildPipes[SYSTEM2_FD_READ]);
            posix_spawn_file_actions_addclose(&file_actions, childToParentPipes[SYSTEM2_FD_WRITE]);

            //Handle changing the directory
            if(inOutCommandInfo->RunDirectory)
            {
                free(nullTerminatedArgs);
                return SYSTEM2_RESULT_POSIX_SPAWN_RUN_DIRECTORY_NOT_SUPPORTED;
            }

            pid_t pid;
            int spawn_status = posix_spawnp(&pid, 
                                            executable, 
                                            &file_actions, 
                                            NULL, 
                                            (char **)nullTerminatedArgs, 
                                            environ);
            
            posix_spawn_file_actions_destroy(&file_actions);

            if(spawn_status != 0)
            {
                fprintf(stderr, "posix_spawn failed: %s\n", strerror(spawn_status));
                free(nullTerminatedArgs);
                return SYSTEM2_RESULT_CREATE_CHILD_PROCESS_FAILED;
            }
        #endif //SYSTEM2_POSIX_SPAWN
        
        //Parent code
        {
            free(nullTerminatedArgs);
            
            if(close(inOutCommandInfo->ParentToChildPipes[SYSTEM2_FD_READ]) != 0)
                return SYSTEM2_RESULT_PIPE_FD_CLOSE_FAILED;
            
            if(close(inOutCommandInfo->ChildToParentPipes[SYSTEM2_FD_WRITE]) != 0)
                return SYSTEM2_RESULT_PIPE_FD_CLOSE_FAILED;
            
            inOutCommandInfo->ChildProcessID = pid;
        }
        return SYSTEM2_RESULT_SUCCESS;
    }

    SYSTEM2_FUNC_PREFIX SYSTEM2_RESULT System2RunPosix( const char* command, 
                                                        System2CommandInfo* inOutCommandInfo)
    {
        const char* args[] = { "/bin/sh", "-c", command };
        
        return System2RunSubprocessPosix("/bin/sh", args, 3, inOutCommandInfo);
    }
    
    SYSTEM2_FUNC_PREFIX SYSTEM2_RESULT System2ReadFromOutputPosix(  const System2CommandInfo* info, 
                                                                    char* outputBuffer, 
                                                                    uint32_t outputBufferSize,
                                                                    uint32_t* outBytesRead)
    {
        if(!info || !outputBuffer || !outBytesRead)
            return SYSTEM2_RESULT_INVALID_ARGUMENT;
        
        int32_t readResult;
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
        if(!info || !inputBuffer)
            return SYSTEM2_RESULT_INVALID_ARGUMENT;
        
        uint32_t currentWriteLengthLeft = inputBufferSize;
        
        while(true)
        {
            int32_t writeResult = write(info->ParentToChildPipes[SYSTEM2_FD_WRITE], 
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
    
    SYSTEM2_FUNC_PREFIX SYSTEM2_RESULT System2CleanupCommandPosix(const System2CommandInfo* info)
    {
        if(!info)
            return SYSTEM2_RESULT_INVALID_ARGUMENT;

        if(close(info->ChildToParentPipes[SYSTEM2_FD_READ]) != 0)
            return SYSTEM2_RESULT_PIPE_FD_CLOSE_FAILED;

        if(close(info->ParentToChildPipes[SYSTEM2_FD_WRITE]) != 0)
            return SYSTEM2_RESULT_PIPE_FD_CLOSE_FAILED;
        
        return SYSTEM2_RESULT_SUCCESS;
    }
    
    SYSTEM2_FUNC_PREFIX 
    SYSTEM2_RESULT System2GetCommandReturnValueAsyncPosix(  const System2CommandInfo* info, 
                                                            int* outReturnCode,
                                                            bool manualCleanup)
    {
        if(!info || !outReturnCode)
            return SYSTEM2_RESULT_INVALID_ARGUMENT;
        
        int status;
        pid_t pidResult = waitpid(info->ChildProcessID, &status, WNOHANG);
        
        if(pidResult == 0)
            return SYSTEM2_RESULT_COMMAND_NOT_FINISHED;
        else if(pidResult == -1)
            return SYSTEM2_RESULT_COMMAND_WAIT_ASYNC_FAILED;

        if(!manualCleanup)
            System2CleanupCommandPosix(info);
        
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
                                                            int* outReturnCode,
                                                            bool manualCleanup)
    {
        if(!info || !outReturnCode)
            return SYSTEM2_RESULT_INVALID_ARGUMENT;
        
        int status;
        pid_t pidResult = waitpid(info->ChildProcessID, &status, 0);
        
        if(pidResult == -1)
            return SYSTEM2_RESULT_COMMAND_WAIT_SYNC_FAILED;
        
        if(!manualCleanup)
            System2CleanupCommandPosix(info);
        
        if(!WIFEXITED(status))
        {
            *outReturnCode = -1;
            return SYSTEM2_RESULT_COMMAND_TERMINATED;
        }

        *outReturnCode = WEXITSTATUS(status);
        return SYSTEM2_RESULT_SUCCESS;
    }
    
    typedef struct
    {
        char** Envs;
        int EnvsLength;
    } System2EnvVarInfoPosix;
    
    SYSTEM2_FUNC_PREFIX 
    SYSTEM2_RESULT System2GetEnvironmentVariablesCountPosix(int* outCount, void** outResource)
    {
        if(!outCount || !outResource)
            return SYSTEM2_RESULT_INVALID_ARGUMENT;
        
        *outResource = NULL;
        *outCount = 0;
        while(environ[*outCount])
            ++(*outCount);
        
        System2EnvVarInfoPosix* info = calloc(1, sizeof(System2EnvVarInfoPosix));
        if(!info)
            return SYSTEM2_RESULT_MALLOC_FAILED;
        
        info->Envs = calloc(*outCount, sizeof(char*));
        if(!info->Envs)
        {
            free(info);
            return SYSTEM2_RESULT_MALLOC_FAILED;
        }
        
        info->EnvsLength = *outCount;
        int i;
        for(i = 0; i < *outCount; ++i)
        {
            int memNeeded = strlen(environ[i]) + 1;
            char* envVar = (char*)malloc(memNeeded);
            if(!envVar)
                break;
            
            memcpy(envVar, environ[i], memNeeded);
            info->Envs[i] = envVar;
        }
        
        //Failed to allocate for all of the env strings
        if(i != *outCount)
        {
            for(int j = 0; j < i; ++j)
                free(info->Envs[j]);
            
            free(info->Envs);
            free(info);
            return SYSTEM2_RESULT_MALLOC_FAILED;
        }
        
        *outResource = info;
        return SYSTEM2_RESULT_SUCCESS;
    }
    
    SYSTEM2_FUNC_PREFIX SYSTEM2_RESULT System2GetEnvironmentVariablePosix(  const void* resource,
                                                                            const char** outName,
                                                                            int* outNameLength,
                                                                            const char** outValue,
                                                                            int* outValueLength,
                                                                            int index)
    {
        if(!outName || !outNameLength || !outValue || !outValueLength || !resource)
            return SYSTEM2_RESULT_INVALID_ARGUMENT;
        
        char* env = ((System2EnvVarInfoPosix*)resource)->Envs[index];
        *outName = env;
        char* equal = env;
        while(equal)
        {
            if(*equal == '=')
                break;
            ++equal;
        }
        *outNameLength = (int)(equal - env);
        
        env = equal + 1;
        *outValue = env;
        while(*env)
            ++env;
        
        *outValueLength = (int)(env - (equal + 1));
        return SYSTEM2_RESULT_SUCCESS;
    }
    
    SYSTEM2_FUNC_PREFIX SYSTEM2_RESULT System2EnvironmentVariableFreePosix(void** resource)
    {
        if(!resource)
            return SYSTEM2_RESULT_INVALID_ARGUMENT;
        
        if(!(*resource))
            return SYSTEM2_RESULT_SUCCESS;
        
        free(*resource);
        *resource = NULL;
        return SYSTEM2_RESULT_SUCCESS;
    }
    
    SYSTEM2_FUNC_PREFIX
    SYSTEM2_RESULT System2SetEnvironmentVariablePosix(const char* envName, const char* envValue)
    {
        if(!envName)
            return SYSTEM2_RESULT_INVALID_ARGUMENT;
        
        if(envValue)
        {
            int result = setenv(envName, envValue, 1);
            if(result != 0)
                return SYSTEM2_RESULT_INVALID_ARGUMENT;
        }
        else
        {
            int result = unsetenv(envName);
            if(result != 0)
                return SYSTEM2_RESULT_INVALID_ARGUMENT;
        }
        
        return SYSTEM2_RESULT_SUCCESS;
    }
#endif //defined(__unix__) || defined(__APPLE__)

#if defined(_WIN32)
    #include <strsafe.h>
    
    SYSTEM2_FUNC_PREFIX void PrintError(LPCTSTR lpszFunction)
    { 
        (void)lpszFunction;
        
        // Retrieve the system error message for the last-error code
        LPVOID lpMsgBuf;
        DWORD dw = GetLastError(); 
    
        FormatMessage(  FORMAT_MESSAGE_ALLOCATE_BUFFER | 
                        FORMAT_MESSAGE_FROM_SYSTEM |
                        FORMAT_MESSAGE_IGNORE_INSERTS,
                        NULL,
                        dw,
                        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                        (LPTSTR) &lpMsgBuf,
                        0, 
                        NULL );
    
        // Display the error message and exit the process
        printf("Error %d: %s\n", dw, (char*)lpMsgBuf);
        LocalFree(lpMsgBuf);
    }

    //https://learn.microsoft.com/en-gb/archive/blogs/twistylittlepassagesallalike/everyone-quotes-command-line-arguments-the-wrong-way
    SYSTEM2_FUNC_PREFIX int ConstructCommandLineWindows(const char* const* args,
                                                        size_t argsCount,
                                                        bool disableEscape,
                                                        int resultSize,
                                                        char* outResult)
    {
        size_t currentIndex = 0;
        
        if(argsCount == 0)
        {
            if(outResult != NULL)
            {
                if(resultSize < 1)
                    return -1;
                else
                {
                    outResult[0] = '\0';
                    return 1;
                }
            }
            else
                return 1;
        }
        
        for(size_t i = 0; i < argsCount; ++i)
        {
            size_t currentArgLength = strlen(args[i]);
            
            if( currentArgLength != 0 &&
                strchr(args[i], ' ') == NULL &&
                strchr(args[i], '\t') == NULL &&
                strchr(args[i], '\n') == NULL &&
                strchr(args[i], '\v') == NULL &&
                strchr(args[i], '"') == NULL)
            {
                if(outResult != NULL)
                {
                    memcpy(&outResult[currentIndex], args[i], currentArgLength);
                    currentIndex += currentArgLength;
                    
                    //Bound check
                    if(currentIndex >= resultSize)
                        return -1;
                    
                    if(i != argsCount - 1)
                        outResult[currentIndex++] = ' ';
                    else
                        outResult[currentIndex++] = '\0';
                }
                else
                {
                    currentIndex += currentArgLength;
                    ++currentIndex;
                }
            }
            else
            {
                if(outResult != NULL)
                {
                    //Bound check
                    if(currentIndex >= resultSize)
                        return -1;
                    
                    outResult[currentIndex++] = '"';
                }
                else
                    ++currentIndex;
                
                for(int j = 0; j < currentArgLength; ++j)
                {
                    int numberBackslashes = 0;
                
                    if(!disableEscape)
                    {
                        for(; j < currentArgLength && args[i][j] == '\\'; ++j)
                            ++numberBackslashes;
                    }
                
                    if(j == currentArgLength && !disableEscape)
                    {
                        // Escape all backslashes, but let the terminating
                        // double quotation mark we add below be interpreted
                        // as a metacharacter.
                        if(outResult != NULL)
                        {
                            //Bound check
                            if(currentIndex + numberBackslashes * 2 >= resultSize)
                                return -1;
                            
                            for(int k = 0; k < numberBackslashes * 2; ++k)
                                outResult[currentIndex++] = '\\';
                        }
                        else
                        {
                            for(int k = 0; k < numberBackslashes * 2; ++k)
                                ++currentIndex;
                        }
                    }
                    else if(args[i][j] == '"' && !disableEscape)
                    {
                        // Escape all backslashes and the following
                        // double quotation mark.
                        if(outResult != NULL)
                        {
                            //Bound check
                            if(currentIndex + numberBackslashes * 2 + 1 + 1 >= resultSize)
                                return -1;
                            
                            for(int k = 0; k < numberBackslashes * 2 + 1; ++k)
                                outResult[currentIndex++] = '\\';
                            
                            outResult[currentIndex++] = '"';
                        }
                        else
                        {
                            for(int k = 0; k < numberBackslashes * 2 + 1; ++k)
                                ++currentIndex;
                            
                            ++currentIndex;
                        }
                    }
                    else
                    {
                        if(outResult != NULL)
                        {
                            //Bound check
                            if(currentIndex + numberBackslashes + 1 >= resultSize)
                                return -1;
                            
                            // Backslashes aren't special here.
                            for(int k = 0; k < numberBackslashes; ++k)
                                outResult[currentIndex++] = '\\';
                            
                            outResult[currentIndex++] = args[i][j];
                        }
                        else
                        {
                            for(int k = 0; k < numberBackslashes; ++k)
                                ++currentIndex;
                            
                            ++currentIndex;
                        }
                    }
                }
                
                if(outResult != NULL)
                {
                    //Bound check
                    if(currentIndex + 2 > resultSize)
                        return -1;
                    
                    outResult[currentIndex++] = '"';
                    if(i != argsCount - 1)
                        outResult[currentIndex++] = ' ';
                    else
                        outResult[currentIndex++] = '\0';
                }
                else
                    currentIndex += 2;
            }
        }
        
        return (int)currentIndex;
    }
    
    SYSTEM2_FUNC_PREFIX SYSTEM2_RESULT 
    Internal_System2Utf8ToUtf16(const char* utf8, int inLen, wchar_t** outUtf16, int* outLen)
    {
        if(!utf8 || !outUtf16 || !outLen)
            return SYSTEM2_RESULT_INVALID_ARGUMENT;
        
        int wLen = MultiByteToWideChar(CP_UTF8, 0, utf8, inLen, NULL, 0);
        if(wLen <= 0)
            return SYSTEM2_RESULT_WINDOWS_UNICODE_FAILED;
        
        wchar_t* w = calloc(wLen, sizeof(wchar_t));
        if(!w)
            return SYSTEM2_RESULT_MALLOC_FAILED;
        
        wLen = MultiByteToWideChar(CP_UTF8, 0, utf8, inLen, w, wLen);
        if(wLen <= 0)
        {
            free(w);
            return SYSTEM2_RESULT_WINDOWS_UNICODE_FAILED;
        }
        
        *outUtf16 = w;
        *outLen = wLen;
        return SYSTEM2_RESULT_SUCCESS;
    }
    
    
    SYSTEM2_FUNC_PREFIX 
    SYSTEM2_RESULT System2RunSubprocessWindows( const char* executable,
                                                const char* const* args,
                                                int argsCount,
                                                System2CommandInfo* inOutCommandInfo)
    {
        if(!executable || !args || !inOutCommandInfo)
            return SYSTEM2_RESULT_INVALID_ARGUMENT;
        
        // Set the write handle to the pipe for STDOUT to be inherited.
        if(inOutCommandInfo->RedirectOutput)
        {
            // Create a pipe for the child process's STDOUT. 
            if(!CreatePipe( &inOutCommandInfo->ChildToParentPipes[SYSTEM2_FD_READ], 
                            &inOutCommandInfo->ChildToParentPipes[SYSTEM2_FD_WRITE], 
                            NULL, 
                            0))
            {
                return SYSTEM2_RESULT_PIPE_CREATE_FAILED;
            }
            
            if(!SetHandleInformation(   inOutCommandInfo->ChildToParentPipes[SYSTEM2_FD_WRITE], 
                                        HANDLE_FLAG_INHERIT, 
                                        HANDLE_FLAG_INHERIT))
            {
                return SYSTEM2_RESULT_PIPE_CREATE_FAILED;
            }
        }
        
        // Create a pipe for the child process's STDIN. 
        if(!CreatePipe( &inOutCommandInfo->ParentToChildPipes[SYSTEM2_FD_READ], 
                        &inOutCommandInfo->ParentToChildPipes[SYSTEM2_FD_WRITE], 
                        NULL, 
                        0))
        {
            return SYSTEM2_RESULT_PIPE_CREATE_FAILED;
        }

        // Set the read handle to the pipe for STDIN to be inherited. 
        if(!SetHandleInformation(   inOutCommandInfo->ParentToChildPipes[SYSTEM2_FD_READ], 
                                    HANDLE_FLAG_INHERIT, 
                                    HANDLE_FLAG_INHERIT))
        {
            return SYSTEM2_RESULT_PIPE_CREATE_FAILED;
        }
        
        PROCESS_INFORMATION processInfo;
        STARTUPINFOW startupInfo;
        BOOL success = FALSE; 
        
        // Set up members of the PROCESS_INFORMATION structure. 
        ZeroMemory( &processInfo, sizeof(PROCESS_INFORMATION) );
    
        // Set up members of the STARTUPINFO structure. 
        // This structure specifies the STDIN and STDOUT handles for redirection.
        ZeroMemory(&startupInfo, sizeof(STARTUPINFOW));
        startupInfo.cb = sizeof(STARTUPINFOW); 
        startupInfo.dwFlags |= STARTF_USESTDHANDLES;
        
        if(inOutCommandInfo->RedirectInput)
            startupInfo.hStdInput = inOutCommandInfo->ParentToChildPipes[SYSTEM2_FD_READ];
        else
            startupInfo.hStdInput = GetStdHandle(STD_INPUT_HANDLE);;
        
        if(inOutCommandInfo->RedirectOutput)
        {
            startupInfo.hStdError = inOutCommandInfo->ChildToParentPipes[SYSTEM2_FD_WRITE];
            startupInfo.hStdOutput = inOutCommandInfo->ChildToParentPipes[SYSTEM2_FD_WRITE];
        }
        else
        {
            startupInfo.hStdError = GetStdHandle(STD_ERROR_HANDLE);
            startupInfo.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
        }
        
        //Join and escape each argument together, then run it
        {
            //Calculating final command count
            int finalCommandSize = 0;
            
            const char** concatedArgs = calloc(argsCount + 1, sizeof(char*));
            concatedArgs[0] = executable;
            for(int i = 0; i < argsCount; ++i)
                concatedArgs[i + 1] = args[i];
            
            finalCommandSize = ConstructCommandLineWindows( concatedArgs, 
                                                            argsCount + 1, 
                                                            inOutCommandInfo->DisableEscapes,
                                                            0, 
                                                            NULL);
            
            if(finalCommandSize < 0)
            {
                free(concatedArgs);
                return SYSTEM2_RESULT_COMMAND_CONSTRUCT_FAILED;
            }
            
            char* commandCopy = (char*)malloc(finalCommandSize);
            if(commandCopy == NULL)
            {
                free(concatedArgs);
                return SYSTEM2_RESULT_COMMAND_CONSTRUCT_FAILED;
            }
            
            int wroteSize = ConstructCommandLineWindows(concatedArgs, 
                                                        argsCount + 1, 
                                                        inOutCommandInfo->DisableEscapes, 
                                                        finalCommandSize, 
                                                        commandCopy);
            
            free((void*)concatedArgs);
            
            if(wroteSize != finalCommandSize)
            {
                printf( "wroteSize and finalCommandSize mismatch, %d, %d\n", 
                        wroteSize, 
                        finalCommandSize);
                
                return SYSTEM2_RESULT_COMMAND_CONSTRUCT_FAILED;
            }
            
            //Construct final command
            int wideCommandSize = 0;
            wchar_t* commandCopyWide = NULL;
            SYSTEM2_RESULT result = Internal_System2Utf8ToUtf16(commandCopy, 
                                                                finalCommandSize,
                                                                &commandCopyWide,
                                                                &wideCommandSize);
            if(result != SYSTEM2_RESULT_SUCCESS)
                return result;
            
            free(commandCopy);
            
            //Convert working directory to wide chars
            wchar_t* workingDirectoryWide = NULL;
            int wideWorkingWideDirSize = 0;
            if(inOutCommandInfo->RunDirectory != NULL)
            {
                result = 
                    Internal_System2Utf8ToUtf16(inOutCommandInfo->RunDirectory, 
                                                (int)strlen(inOutCommandInfo->RunDirectory) + 1,
                                                &workingDirectoryWide,
                                                &wideWorkingWideDirSize);
                
                if(result != SYSTEM2_RESULT_SUCCESS)
                {
                    free(commandCopyWide);
                    return result;
                }
            }
            
            success = CreateProcessW(   NULL, 
                                        commandCopyWide,                // command line 
                                        //L"PrintArgs.exe a\\\\b d\"e f\"g h",
                                        NULL,                           // process securitye attributes 
                                        NULL,                           // primary thread security attributes 
                                        TRUE,                           // handles are inherited 
                                        CREATE_UNICODE_ENVIRONMENT,     // creation flags 
                                        NULL,                           // use parent's environment 
                                        workingDirectoryWide,           // use parent's current directory 
                                        &startupInfo,                   // STARTUPINFO pointer 
                                        &processInfo);                  // receives PROCESS_INFORMATION 
            free(commandCopyWide);
            if(workingDirectoryWide != NULL)
                free(workingDirectoryWide);
        }
        
        // If an error occurs, exit the application. 
        if(!success)
        {
            PrintError("CreateProcessW");
            return SYSTEM2_RESULT_CREATE_CHILD_PROCESS_FAILED;
        }
        else 
        {
            // Close handles to the child process and its primary thread.
            // Some applications might keep these handles to monitor the status
            // of the child process, for example. 
            inOutCommandInfo->ChildProcessHandle = processInfo.hProcess;
            
            // Close handles to the stdin and stdout pipes no longer needed by the child process.
            // If they are not explicitly closed, there is no way to recognize that the child process has ended.
            if(inOutCommandInfo->RedirectInput)
            {
                if(!CloseHandle(processInfo.hThread))
                    return SYSTEM2_RESULT_CREATE_CHILD_PROCESS_FAILED;
            }

            if(inOutCommandInfo->RedirectOutput)
            {
                if(!CloseHandle(inOutCommandInfo->ChildToParentPipes[SYSTEM2_FD_WRITE]))
                    return SYSTEM2_RESULT_PIPE_FD_CLOSE_FAILED;
            }
            
            if(!CloseHandle(inOutCommandInfo->ParentToChildPipes[SYSTEM2_FD_READ]))
                return SYSTEM2_RESULT_PIPE_FD_CLOSE_FAILED;

            return SYSTEM2_RESULT_SUCCESS;
        }
    }
    
    SYSTEM2_FUNC_PREFIX SYSTEM2_RESULT System2RunWindows(   const char* command, 
                                                            System2CommandInfo* outCommandInfo)
    {
        const char* args[] = {"/s", "/v", "/c", command};
        outCommandInfo->DisableEscapes = true;
        return System2RunSubprocessWindows( "cmd", 
                                            args, 
                                            sizeof(args) / sizeof(char*), 
                                            outCommandInfo);
    }
    
    //TODO: UTF-8 output?
    //TODO: Use peeknamedpipe to get number of bytes available before reading it 
    //      so that it doesn't block
    //https://learn.microsoft.com/en-us/windows/win32/api/namedpipeapi/nf-namedpipeapi-peeknamedpipe
    SYSTEM2_FUNC_PREFIX SYSTEM2_RESULT System2ReadFromOutputWindows(const System2CommandInfo* info, 
                                                                    char* outputBuffer, 
                                                                    uint32_t outputBufferSize,
                                                                    uint32_t* outBytesRead)
    {
        if(!info || !outputBuffer || !outBytesRead)
            return SYSTEM2_RESULT_INVALID_ARGUMENT;
        
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
        if(!info || !inputBuffer)
            return SYSTEM2_RESULT_INVALID_ARGUMENT;
        
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
    
    SYSTEM2_FUNC_PREFIX SYSTEM2_RESULT System2CleanupCommandWindows(const System2CommandInfo* info)
    {
        if(!info)
            return SYSTEM2_RESULT_INVALID_ARGUMENT;
        
        if(info->RedirectOutput)
        {
            if(!CloseHandle(info->ChildToParentPipes[SYSTEM2_FD_READ]))
                return SYSTEM2_RESULT_PIPE_FD_CLOSE_FAILED;
        }
        
        if(info->RedirectInput)
        {
            if(!CloseHandle(info->ParentToChildPipes[SYSTEM2_FD_WRITE]))
                return SYSTEM2_RESULT_PIPE_FD_CLOSE_FAILED;
        }

        CloseHandle(info->ChildProcessHandle);
        return SYSTEM2_RESULT_SUCCESS;
    }
    
    SYSTEM2_FUNC_PREFIX 
    SYSTEM2_RESULT System2GetCommandReturnValueAsyncWindows(const System2CommandInfo* info, 
                                                            int* outReturnCode,
                                                            bool manualCleanup)
    {
        if(!info || !outReturnCode)
            return SYSTEM2_RESULT_INVALID_ARGUMENT;
        
        DWORD exitCode;
        if(!GetExitCodeProcess(info->ChildProcessHandle, &exitCode))
            return SYSTEM2_RESULT_COMMAND_WAIT_ASYNC_FAILED;
        
        if( exitCode == STILL_ACTIVE && 
            WaitForSingleObject(info->ChildProcessHandle, 0) == WAIT_TIMEOUT)
        {
            return SYSTEM2_RESULT_COMMAND_NOT_FINISHED;
        }

        *outReturnCode = exitCode;
        
        if(!manualCleanup)
            return System2CleanupCommandWindows(info);
        else
            return SYSTEM2_RESULT_SUCCESS;
    }
    
    SYSTEM2_FUNC_PREFIX 
    SYSTEM2_RESULT System2GetCommandReturnValueSyncWindows( const System2CommandInfo* info, 
                                                            int* outReturnCode,
                                                            bool manualCleanup)
    {
        if(!info || !outReturnCode)
            return SYSTEM2_RESULT_INVALID_ARGUMENT;
        
        if(WaitForSingleObject(info->ChildProcessHandle, INFINITE) != 0)
            return SYSTEM2_RESULT_COMMAND_WAIT_SYNC_FAILED;
        
        DWORD exitCode;
        if(!GetExitCodeProcess(info->ChildProcessHandle, &exitCode))
        {
            *outReturnCode = -1;
            CloseHandle(info->ChildProcessHandle);
            return SYSTEM2_RESULT_COMMAND_TERMINATED;
        }
        
        *outReturnCode = exitCode;
        
        if(!manualCleanup)
            return System2CleanupCommandWindows(info);
        else
            return SYSTEM2_RESULT_SUCCESS;
    }
    
    SYSTEM2_FUNC_PREFIX char* Internal_System2GetEnvStringWindows()
    {
        wchar_t* envStrings = GetEnvironmentStringsW();
        wchar_t* envStringsStart = envStrings;
        int strCount = 0;
        
        while(*envStrings != L'\0' || *(envStrings + 1) != L'\0')
        {
            ++strCount;
            ++envStrings;
        }
        ++strCount;
        
        int requiredBytes = WideCharToMultiByte(CP_UTF8, 
                                                0, 
                                                envStringsStart, 
                                                strCount, 
                                                NULL, 
                                                0, 
                                                NULL, 
                                                NULL);
        if(requiredBytes <= 0)
        {
            FreeEnvironmentStringsW(envStringsStart);
            return NULL;
        }
        
        char* utf8EnvString = (char*)malloc(requiredBytes);
        if(!utf8EnvString)
        {
            FreeEnvironmentStringsW(envStringsStart);
            return NULL;
        }
        memset(utf8EnvString, 0, requiredBytes);
        
        int allocatedBytes = WideCharToMultiByte(   CP_UTF8, 
                                                    0, 
                                                    envStringsStart, 
                                                    strCount, 
                                                    utf8EnvString, 
                                                    requiredBytes, 
                                                    NULL, 
                                                    NULL);
        if(allocatedBytes <= 0)
        {
            FreeEnvironmentStringsW(envStringsStart);
            free(utf8EnvString);
            return NULL;
        }
        return utf8EnvString;
    }
    
    SYSTEM2_FUNC_PREFIX 
    SYSTEM2_RESULT System2GetEnvironmentVariablesCountWindows(int* outCount, void** outResource)
    {
        if(!outCount || !outResource)
            return SYSTEM2_RESULT_INVALID_ARGUMENT;
        
        *outResource = NULL;
        char* utf8EnvStringStart = Internal_System2GetEnvStringWindows();
        if(!utf8EnvStringStart)
            return SYSTEM2_RESULT_COMMAND_CONSTRUCT_FAILED;
        
        char* utf8EnvStrings = utf8EnvStringStart;
        if(*utf8EnvStrings == '\0' && *(utf8EnvStrings + 1) == '\0')
        {
            *outCount = 0;
            free(utf8EnvStringStart);
            return SYSTEM2_RESULT_SUCCESS;
        }
        *outResource = utf8EnvStringStart;
        *outCount = 0;
        
        /*
        Each environment block contains the environment variables in the following format:

        Var1=Value1\0
        Var2=Value2\0
        Var3=Value3\0
        ...
        VarN=ValueN\0\0
        */
        while(*utf8EnvStrings != '\0' || *(utf8EnvStrings + 1) != '\0')
        {
            if(*utf8EnvStrings == '\0')
                ++(*outCount);
            ++utf8EnvStrings;
        }
        
        return SYSTEM2_RESULT_SUCCESS;
    }
    
    SYSTEM2_FUNC_PREFIX SYSTEM2_RESULT System2GetEnvironmentVariableWindows(const void* resource,
                                                                            const char** outName,
                                                                            int* outNameLength,
                                                                            const char** outValue,
                                                                            int* outValueLength,
                                                                            int index)
    {
        if(!outName || !outNameLength || !outValue || !outValueLength || !resource)
            return SYSTEM2_RESULT_INVALID_ARGUMENT;
        
        static const char* lastResource = NULL;
        static int lastIndex = 0;
        static char* lastEnv = NULL;
        
        bool canUseCache = resource == lastResource && lastEnv && index > lastIndex;
        
        const char* envStringsStart = resource;
        char* envStrings = canUseCache ? lastEnv : (char*)envStringsStart;
        int currentIndex = canUseCache ? lastIndex : 0;
        
        lastResource = resource;
        do
        {
            if(currentIndex == index)
            {
                lastEnv = envStrings;
                lastIndex = index;
                
                *outName = envStrings;
                char* equal = envStrings;
                while(equal)
                {
                    if(*equal == '=')
                        break;
                    ++equal;
                }
                *outNameLength = (int)(equal - envStrings);
                
                envStrings = equal + 1;
                *outValue = envStrings;
                while(*envStrings)
                    ++envStrings;
                
                *outValueLength = (int)(envStrings - (equal + 1));
                break;
            }
            
            if(*envStrings == '\0')
                ++currentIndex;
            ++envStrings;
        }
        while(*envStrings != '\0' || *(envStrings + 1) != '\0');
        
        return SYSTEM2_RESULT_SUCCESS;
    }
    
    SYSTEM2_FUNC_PREFIX SYSTEM2_RESULT System2EnvironmentVariableFreeWindows(void** resource)
    {
        if(!resource)
            return SYSTEM2_RESULT_INVALID_ARGUMENT;
        
        if(!(*resource))
            return SYSTEM2_RESULT_SUCCESS;
        
        free(*resource);
        *resource = NULL;
        return SYSTEM2_RESULT_SUCCESS;
    }
    
    SYSTEM2_FUNC_PREFIX
    SYSTEM2_RESULT System2SetEnvironmentVariableWindows(const char* envName, const char* envValue)
    {
        if(!envName)
            return SYSTEM2_RESULT_INVALID_ARGUMENT;
        
        wchar_t* envNameW;
        int envNameWLen;
        SYSTEM2_RESULT result = Internal_System2Utf8ToUtf16(envName, 
                                                            (int)strlen(envName) + 1, 
                                                            &envNameW, 
                                                            &envNameWLen);
        if(result != SYSTEM2_RESULT_SUCCESS)
            return result;
        
        if(envValue)
        {
            wchar_t* envValueW;
            int envValueWLen;
            result = Internal_System2Utf8ToUtf16(   envValue, 
                                                    (int)strlen(envValue) + 1, 
                                                    &envValueW, 
                                                    &envValueWLen);
            if(result != SYSTEM2_RESULT_SUCCESS)
            {
                free(envNameW);
                return result;
            }
            
            if(SetEnvironmentVariableW(envNameW, envValueW) == 0)
            {
                free(envNameW);
                free(envValueW);
                return SYSTEM2_RESULT_WINDOWS_SET_ENV_FAILED;
            }
            free(envValueW);
        }
        else
        {
            if(SetEnvironmentVariableW(envNameW, NULL) == 0)
            {
                free(envNameW);
                return SYSTEM2_RESULT_WINDOWS_SET_ENV_FAILED;
            }
        }
        
        free(envNameW);
        return SYSTEM2_RESULT_SUCCESS;
    }
#endif //defined(_WIN32)

SYSTEM2_FUNC_PREFIX SYSTEM2_RESULT System2Run(  const char* command, 
                                                System2CommandInfo* inOutCommandInfo)
{
    #if defined(__unix__) || defined(__APPLE__)
        return System2RunPosix(command, inOutCommandInfo);
    #elif defined(_WIN32)
        return System2RunWindows(command, inOutCommandInfo);
    #else
        return SYSTEM2_RESULT_UNSUPPORTED_PLATFORM; 
    #endif
}

SYSTEM2_FUNC_PREFIX SYSTEM2_RESULT System2RunSubprocess(const char* executable,
                                                        const char* const* args,
                                                        int argsCount,
                                                        System2CommandInfo* inOutCommandInfo)
{
    #if defined(__unix__) || defined(__APPLE__)
        return System2RunSubprocessPosix(executable, args, argsCount, inOutCommandInfo);
    #elif defined(_WIN32)
        return System2RunSubprocessWindows(executable, args, argsCount, inOutCommandInfo);
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

SYSTEM2_FUNC_PREFIX SYSTEM2_RESULT System2CleanupCommand(const System2CommandInfo* info)
{
    #if defined(__unix__) || defined(__APPLE__)
        return System2CleanupCommandPosix(info);
    #elif defined(_WIN32)
        return System2CleanupCommandWindows(info);
    #else
        return SYSTEM2_RESULT_UNSUPPORTED_PLATFORM; 
    #endif
}

SYSTEM2_FUNC_PREFIX 
SYSTEM2_RESULT System2GetCommandReturnValueAsync(   const System2CommandInfo* info, 
                                                    int* outReturnCode,
                                                    bool manualCleanup)
{
    #if defined(__unix__) || defined(__APPLE__)
        return System2GetCommandReturnValueAsyncPosix(info, outReturnCode, manualCleanup);
    #elif defined(_WIN32)
        return System2GetCommandReturnValueAsyncWindows(info, outReturnCode, manualCleanup);
    #else
        return SYSTEM2_RESULT_UNSUPPORTED_PLATFORM; 
    #endif
}

SYSTEM2_FUNC_PREFIX 
SYSTEM2_RESULT System2GetCommandReturnValueSync(const System2CommandInfo* info, 
                                                int* outReturnCode,
                                                bool manualCleanup)
{
    #if defined(__unix__) || defined(__APPLE__)
        return System2GetCommandReturnValueSyncPosix(info, outReturnCode, manualCleanup);
    #elif defined(_WIN32)
        return System2GetCommandReturnValueSyncWindows(info, outReturnCode, manualCleanup);
    #else
        return SYSTEM2_RESULT_UNSUPPORTED_PLATFORM; 
    #endif
}

SYSTEM2_FUNC_PREFIX 
SYSTEM2_RESULT System2GetEnvironmentVariablesCount(int* outCount, void** outResource)
{
    #if defined(__unix__) || defined(__APPLE__)
        return System2GetEnvironmentVariablesCountPosix(outCount, outResource);
    #elif defined(_WIN32)
        return System2GetEnvironmentVariablesCountWindows(outCount, outResource);
    #else
        return SYSTEM2_RESULT_UNSUPPORTED_PLATFORM;
    #endif
}

SYSTEM2_FUNC_PREFIX SYSTEM2_RESULT System2GetEnvironmentVariable(   const void* resource,
                                                                    const char** outName,
                                                                    int* outNameLength,
                                                                    const char** outValue,
                                                                    int* outValueLength,
                                                                    int index)
{
    #if defined(__unix__) || defined(__APPLE__)
        return System2GetEnvironmentVariablePosix(  resource,
                                                    outName, 
                                                    outNameLength, 
                                                    outValue, 
                                                    outValueLength, 
                                                    index);
    #elif defined(_WIN32)
        return System2GetEnvironmentVariableWindows(resource,
                                                    outName, 
                                                    outNameLength, 
                                                    outValue, 
                                                    outValueLength, 
                                                    index);
    #else
        return SYSTEM2_RESULT_UNSUPPORTED_PLATFORM;
    #endif
}

SYSTEM2_FUNC_PREFIX SYSTEM2_RESULT System2EnvironmentVariableFree(void** resource)
{
    #if defined(__unix__) || defined(__APPLE__)
        return System2EnvironmentVariableFreePosix(resource);
    #elif defined(_WIN32)
        return System2EnvironmentVariableFreeWindows(resource);
    #else
        return SYSTEM2_RESULT_UNSUPPORTED_PLATFORM;
    #endif
}

SYSTEM2_FUNC_PREFIX 
SYSTEM2_RESULT System2SetEnvironmentVariable(const char* envName, const char* envValue)
{
    #if defined(__unix__) || defined(__APPLE__)
        return System2SetEnvironmentVariablePosix(envName, envValue);
    #elif defined(_WIN32)
        return System2SetEnvironmentVariableWindows(envName, envValue);
    #else
        return SYSTEM2_RESULT_UNSUPPORTED_PLATFORM;
    #endif
}


#if defined(_WIN32)
    #if INTERNAL_SYSTEM2_APPLY_NO_WARNINGS
        #undef _CRT_SECURE_NO_WARNINGS
    #endif
#endif

#endif //#if !SYSTEM2_DECLARATION_ONLY

#endif //#ifndef SYSTEM2_H


