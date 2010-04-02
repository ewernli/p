#include <plugin/Plugin.h>

NATIVE0(Process_popen)
    //TODO typecheck
    Type_Object process = (Type_Object) self;
    char * command      = unicode_to_ascii(((Type_String)process->ivals[0])->value);
    char * mode         = unicode_to_ascii(((Type_String)process->ivals[1])->value);
    FILE * stream       = popen(command, mode);
    process->ivals[2]   = (Object)new_IO_WriteFile_from(stream);
    process->ivals[3]   = (Object)new_IO_ReadFile_from(stream);
    RETURN_FROM_NATIVE(self);
}

NATIVE0(Process_pclose)
    IO_File stream  = (IO_File)((Type_Object)self)->ivals[2];
    int returnValue = pclose(stream->file);
    RETURN_FROM_NATIVE(new_Type_SmallInt(returnValue));
}

PLUGIN()
    EXPORT(new_Type_Symbol_cached(L"pclose"), Process_pclose);
    EXPORT(new_Type_Symbol_cached(L"popen"), Process_popen);
}

void unload_plugin()
{

}
