#ifndef SCRATCHCOMPILER_H
#define SCRATCHCOMPILER_H

#include <stdio.h>

int compile_file(const char* filename, const char* out_filename, int file);

struct compile_process* compile_process_create(const char* filename, const char* filename_out, int flags);

enum {
    COMPILER_FILE_COMPILED_OK,
    COMPILER_FAILED_WITH_ERRORS
};

struct compile_process
{
    //The flags to determine how this file should be compiled
    int flags;

    struct compile_process_input_file
    {
        FILE* fp;
        const char* abs_path;
    } cfile;


    FILE* ofile;
        
};

#endif