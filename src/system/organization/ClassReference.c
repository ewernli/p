#include <stdlib.h>
#include <stdio.h>
#include <system/organization/ClassReference.h>

/* ========================================================================= */

Type_Class Organization_ClassReference_Class;

/* ========================================================================= */

Organization_ClassReference
    new_Organization_ClassReference(Type_Class class)
{
    
    NEW_OBJECT(Organization_ClassReference);
    result->class = (Object)class;
    result->package = Nil;
    // result->name = Nil;
    return result;
}

Organization_ClassReference new_Organization_ClassReference_empty()
{
    
    NEW_OBJECT(Organization_ClassReference);
    result->class = Nil;
    result->package = Nil;
    // result->name = Nil;
    return result;
}

/*
Organization_ClassReference
    new_Organization_ClassReference_named(Object class,
                                          Object package,
                                          const wchar_t* name)
{
    Organization_ClassReference result =
    new_Organization_ClassReference(class, package);
    result->name = (Object)new_Type_String(name);
    return result;
}
*/

/* ========================================================================= */

void pre_init_Organization_ClassReference()
{
    Organization_ClassReference_Class =
        new_Class_named((Object)Type_Object_Class,
                        L"Organization_ClassReference",
                        CREATE_OBJECT_TAG(ORGANIZATION_CLASSREFERENCE));
}

/* ========================================================================= */

void Organization_ClassReference_eval(Organization_ClassReference self)
{
    assert0(self->class != (Object)Nil);
    poke_EXP(0, self->class);
}

/* ========================================================================= */

void post_init_Organization_ClassReference()
{

}