#ifndef FILE_H
#define FILE_H

#include <stdio.h>
#include <pinocchio.h>

/* ========================================================================= */

struct IO_File_t {
    FILE *file;
    Type_SmallInt size;
    Type_String path;
};


CREATE_INITIALIZERS(IO_File);
IO_File new_IO_File();
IO_File new_IO_File_fromFile(FILE* file);
IO_File new_IO_File_fromPath(const wchar_t * path, char * mode);
IO_File new_IO_File_temp(char * mode);

/* ========================================================================= */

extern char* unicode_to_ascii(const wchar_t* str);

/* ========================================================================= */

#endif // FILE_H