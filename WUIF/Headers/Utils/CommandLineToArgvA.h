/*Copyright (c) 2018 Jonathan Campbell

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.*/
#pragma once
#include <Windows.h>
#include <mbstring.h>        //needed for _ismbblead in CommandLineToArgvA
#include "WUIF_Error.h"

/*LPSTR* CommandLineToArgvA(_In_ LPCSTR lpCmdLine, _Out_ int *pNumArgs)
Takes the ASCII command line string and splits it into separate args.
Equivalent to CommandLineToArgvW

LPCSTR lpCmdLine[in] - should be GetCommandLineA()
int *pNumArgs[out] - pointer to the number of args (ie. argc value of main(argc,argv[]) )

Return results
    LPSTR* - pointer to the argv[] array
*/
LPSTR* CommandLineToArgvA(_In_ LPCSTR lpCmdLine, _Out_ int *pNumArgs)
{
    PrintEnter(TEXT("::CommandLineToArgvA"));
    if (!pNumArgs)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        PrintExit(TEXT("::CommandLineToArgvA"));
        return NULL;
    }
    *pNumArgs = 0;
    /*follow CommandLinetoArgvW and if lpCmdLine is NULL return the path to the executable.
    Use 'programname' so that we don't have to allocate MAX_PATH * sizeof(CHAR) for argv
    every time. Since this is ANSI the return can't be greater than MAX_PATH (260
    characters)*/
    CHAR programname[MAX_PATH] = {};
    /*pnlength = the length of the string that is copied to the buffer, in characters, not
    including the terminating null character*/
    const DWORD pnlength = GetModuleFileNameA(NULL, programname, MAX_PATH);
    if (pnlength == 0) //error getting program name
    {
        //GetModuleFileNameA will SetLastError
        PrintExit(TEXT("::CommandLineToArgvA"));
        return NULL;
    }
    if (lpCmdLine == nullptr)
    {

        /*In keeping with CommandLineToArgvW the caller should make a single call to HeapFree
        to release the memory of argv. Allocate a single block of memory with space for two
        pointers (representing argv[0] and argv[1]). argv[0] will contain a pointer to argv+2
        where the actual program name will be stored. argv[1] will be nullptr per the C++
        specifications for argv. Hence space required is the size of a LPSTR (char*) multiplied
        by 2 [pointers] + the length of the program name (+1 for null terminating character)
        multiplied by the sizeof CHAR. HeapAlloc is called with HEAP_GENERATE_EXCEPTIONS flag,
        so if there is a failure on allocating memory an exception will be generated.*/
        LPSTR *argv = static_cast<LPSTR*>(HeapAlloc(GetProcessHeap(),
                                                    HEAP_ZERO_MEMORY | HEAP_GENERATE_EXCEPTIONS,
                                                    (sizeof(LPSTR) * 2) + ((static_cast<size_t>(pnlength) + 1) * sizeof(CHAR))));
        memcpy(argv + 2, programname, static_cast<size_t>(pnlength) + 1); //add 1 for the terminating null character
        argv[0] = reinterpret_cast<LPSTR>(argv + 2);
        argv[1] = nullptr;
        *pNumArgs = 1;
        PrintExit(TEXT("::CommandLineToArgvA"));
        return argv;
    }
    /*We need to determine the number of arguments and the number of characters so that the
    proper amount of memory can be allocated for argv. Our argument count starts at 1 as the
    first "argument" is the program name even if there are no other arguments per specs.*/
    int argc = 1;
    int numchars = 0;
    LPCSTR templpcl = lpCmdLine;
    bool in_quotes = false;  //'in quotes' mode is off (false) or on (true)
    /*first scan the program name and copy it. The handling is much simpler than for other
    arguments. Basically, whatever lies between the leading double-quote and next one, or a
    terminal null character is simply accepted. Fancier handling is not required because the
    program name must be a legal NTFS/HPFS file name. Note that the double-quote characters are
    not copied.*/
    do
    {
        if (*templpcl == '"')
        {
            //don't add " to character count
            in_quotes = !in_quotes;
            templpcl++; //move to next character
            continue;
        }
        ++numchars; //count character
        templpcl++; //move to next character
        if (_ismbblead(*templpcl) != 0) //handle MBCS
        {
            ++numchars;
            templpcl++; //skip over trail byte
        }
    } while (*templpcl != '\0' && (in_quotes || (*templpcl != ' ' && *templpcl != '\t')));
    //parsed first argument
    if (*templpcl == '\0')
    {
        /*no more arguments, rewind and the next for statement will handle*/
        templpcl--;
    }
    //loop through the remaining arguments
    int slashcount = 0; //count of backslashes
    bool countorcopychar = true; //count the character or not
    for (;;)
    {
        if (*templpcl)
        {
            //next argument begins with next non-whitespace character
            while (*templpcl == ' ' || *templpcl == '\t')
                ++templpcl;
        }
        if (*templpcl == '\0')
            break; //end of arguments

        ++argc; //next argument - increment argument count
        //loop through this argument
        for (;;)
        {
            /*Rules:
              2N     backslashes   + " ==> N backslashes and begin/end quote
              2N + 1 backslashes   + " ==> N backslashes + literal "
              N      backslashes       ==> N backslashes*/
            slashcount = 0;
            countorcopychar = true;
            while (*templpcl == '\\')
            {
                //count the number of backslashes for use below
                ++templpcl;
                ++slashcount;
            }
            if (*templpcl == '"')
            {
                //if 2N backslashes before, start/end quote, otherwise count.
                if (slashcount % 2 == 0) //even number of backslashes
                {
                    if (in_quotes && *(templpcl + 1) == '"')
                    {
                        in_quotes = !in_quotes; //NB: parse_cmdline omits this line
                        templpcl++; //double quote inside quoted string
                    }
                    else
                    {
                        //skip first quote character and count second
                        countorcopychar = false;
                        in_quotes = !in_quotes;
                    }
                }
                slashcount /= 2;
            }
            //count slashes
            while (slashcount--)
            {
                ++numchars;
            }
            if (*templpcl == '\0' || (!in_quotes && (*templpcl == ' ' || *templpcl == '\t')))
            {
                //at the end of the argument - break
                break;
            }
            if (countorcopychar)
            {
                if (_ismbblead(*templpcl) != 0) //should copy another character for MBCS
                {
                    ++templpcl; //skip over trail byte
                    ++numchars;
                }
                ++numchars;
            }
            ++templpcl;
        }
        //add a count for the null-terminating character
        ++numchars;
    }
    /*allocate memory for argv. Allocate a single block of memory with space for argc number of
    pointers. argv[0] will contain a pointer to argv+argc where the actual program name will be
    stored. argv[argc] will be nullptr per the C++ specifications. Hence space required is the
    size of a LPSTR (char*) multiplied by argc + 1 pointers + the number of characters counted
    above multiplied by the sizeof CHAR. HeapAlloc is called with HEAP_GENERATE_EXCEPTIONS
    flag, so if there is a failure on allocating memory an exception will be generated.*/
    LPSTR *argv = static_cast<LPSTR*>(HeapAlloc(GetProcessHeap(),
                                                HEAP_ZERO_MEMORY | HEAP_GENERATE_EXCEPTIONS,
                                                (sizeof(LPSTR) * (static_cast<size_t>(argc) + 1)) + (numchars * sizeof(CHAR))));
    //now loop through the command line again and split out arguments
    in_quotes = false;
    templpcl = lpCmdLine;
    //C6011 - Dereferencing null pointer - dereferencing NULL pointer 'argv'
    #pragma warning(suppress: 6011)
    argv[0] = reinterpret_cast<LPSTR>(argv + argc + 1);
    LPSTR tempargv = reinterpret_cast<LPSTR>(argv + argc + 1);
    do
    {
        if (*templpcl == '"')
        {
            in_quotes = !in_quotes;
            templpcl++; //move to next character
            continue;
        }
        *tempargv++ = *templpcl;
        templpcl++; //move to next character
        if (_ismbblead(*templpcl) != 0) //should copy another character for MBCS
        {
            *tempargv++ = *templpcl; //copy second byte
            templpcl++; //skip over trail byte
        }
    } while (*templpcl != '\0' && (in_quotes || (*templpcl != ' ' && *templpcl != '\t')));
    //parsed first argument
    if (*templpcl == '\0')
    {
        //no more arguments, rewind and the next for statement will handle
        templpcl--;
    }
    else
    {
        //end of program name - add null terminator
        *tempargv = '\0';
    }
    int currentarg = 1;
    argv[currentarg] = ++tempargv;
    //loop through the remaining arguments
    slashcount = 0; //count of backslashes
    countorcopychar = true; //count the character or not
    for (;;)
    {
        if (*templpcl)
        {
            //next argument begins with next non-whitespace character
            while (*templpcl == ' ' || *templpcl == '\t')
                ++templpcl;
        }
        if (*templpcl == '\0')
            break; //end of arguments
        argv[currentarg] = ++tempargv; //copy address of this argument string
        //next argument - loop through it's characters
        for (;;)
        {
            /*Rules:
              2N     backslashes   + " ==> N backslashes and begin/end quote
              2N + 1 backslashes   + " ==> N backslashes + literal "
              N      backslashes       ==> N backslashes*/
            slashcount = 0;
            countorcopychar = true;
            while (*templpcl == '\\')
            {
                //count the number of backslashes for use below
                ++templpcl;
                ++slashcount;
            }
            if (*templpcl == '"')
            {
                //if 2N backslashes before, start/end quote, otherwise copy literally.
                if (slashcount % 2 == 0) //even number of backslashes
                {
                    if (in_quotes && *(templpcl + 1) == '"')
                    {
                        in_quotes = !in_quotes; //NB: parse_cmdline omits this line
                        templpcl++; //double quote inside quoted string
                    }
                    else
                    {
                        //skip first quote character and count second
                        countorcopychar = false;
                        in_quotes = !in_quotes;
                    }
                }
                slashcount /= 2;
            }
            //copy slashes
            while (slashcount--)
            {
                *tempargv++ = '\\';
            }
            if (*templpcl == '\0' || (!in_quotes && (*templpcl == ' ' || *templpcl == '\t')))
            {
                //at the end of the argument - break
                break;
            }
            if (countorcopychar)
            {
                *tempargv++ = *templpcl;
                if (_ismbblead(*templpcl) != 0) //should copy another character for MBCS
                {
                    ++templpcl; //skip over trail byte
                    *tempargv++ = *templpcl;
                }
            }
            ++templpcl;
        }
        //null-terminate the argument
        *tempargv = '\0';
        ++currentarg;
    }
    argv[argc] = nullptr;
    *pNumArgs = argc;
    PrintExit(TEXT("::CommandLineToArgvA"));
    return argv;
}