#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <pinocchio.h>

/* ========================================================================= */

DECLARE_CLASS(Array);
Array empty_Array;

/* ========================================================================= */

Array new_Array_raw(uns_int c)
{
    if (c == 0) { return empty_Array; }
    Array result = NEW_ARRAYED(struct Array_t, Object[c]);
    HEADER(result)    = Array_Class;
    result->size      = c;
    return result;
}

Array new_Array(uns_int c, Object v[])
{
    if (c == 0) { return empty_Array; }
    Array result = new_Array_raw(c);
    while (0 < c) {
        c--;
        result->values[c] = v[c];
    }
    return result;
}

Array new_Array_with(uns_int c, ...)
{
    if (c == 0) { return empty_Array; }
    Array result = new_Array_raw(c);
    va_list args;
    va_start(args, c);
    int index;
    for (index = 0; index < c; index++) {
        result->values[index] = va_arg(args, Object);
    }
    va_end(args);
    return result;
}

Array new_Array_withAll(uns_int c, Object element)
{
    if (c == 0) { return empty_Array; }
    Array result = new_Array_raw(c);
    while (0 < c) {
        c--;
        result->values[c] = element;
    }
    return result;
}

/* ========================================================================= */

Object raw_Array_instAt(Array o, Object tag, int index)
{
    ASSERT_TAG_SIZE(tag, index);
    return o->values[index];
}

void raw_Array_instAt_put(Array o, Object tag,
                               int index, Object value)
{
    ASSERT_TAG_SIZE(tag, index);
    o->values[index] = value;
}

NATIVE1(Array_instVarAt_)
    Object w_index  = NATIVE_ARG(0);
    int index       = unwrap_int(w_index) - 1;
    Object tag      = GETTAG(self);
    ASSERT_TAG_LAYOUT(tag, Array);
    ASSERT_TAG_SIZE(tag, index);
    RETURN_FROM_NATIVE(((Array)self)->values[index]);
}

NATIVE2(Array_instVarAt_put_)
    Object w_index = NATIVE_ARG(0);
    Object w_arg   = NATIVE_ARG(1);
    int index      = unwrap_int(w_index) - 1;
    Object tag   = GETTAG(self);
    ASSERT_TAG_LAYOUT(tag, Array);
    ASSERT_TAG_SIZE(tag, index);
    ((Array)self)->values[index] = w_arg;
    RETURN_FROM_NATIVE(w_arg);
}

/* ========================================================================= */

void post_init_Array()
{
    Dictionary natives = add_plugin(L"Collection.Array");
    store_native(natives, SMB_instVarAt_,     NM_Array_instVarAt_);
    store_native(natives, SMB_instVarAt_put_, NM_Array_instVarAt_put_);
}
