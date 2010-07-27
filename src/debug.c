#include <stdarg.h>
#include <wchar.h>
#include <pinocchio.h>
#include <debug.h>

/* ========================================================================= */

void help() {
    fwprintf(stderr, L"    at              (Object, uns_int)      \n");
    fwprintf(stderr, L"    atn             (Object, wchar_t *)    \n");
    fwprintf(stderr, L"    atX             (Object, count, idx...)\n");
    fwprintf(stderr, L"    pclass          (Object)               \n");
    fwprintf(stderr, L"    i,inspect       (Object)               \n");
    fwprintf(stderr, L"    inspect_at      (Object, uns_int)      \n");
    fwprintf(stderr, L"    inspect_atn     (Object, wchar_t *)    \n");
    fwprintf(stderr, L"    methods         (Object)               \n");
    fwprintf(stderr, L"    exps                                   \n");
    fwprintf(stderr, L"    sends                                  \n");
}

void h() {
    return help();
}

Class pclass(Optr o)
{
    return HEADER(o);
}

void _indent_(uns_int i)
{
    uns_int todo = 0;
    while (todo++ != i) {
        if (todo % 4) {
            fwprintf(stderr, L"  ");
        } else {
            fwprintf(stderr, L"| ");
        }
    }
}

void print_Class(Optr obj)
{
    if (obj == NULL) {
        fwprintf(stderr, L"NULL\n");
        return;
    }
    if (obj == nil) {
        fwprintf(stderr, L"Nil\n");
        return;
    }
    Class class = pclass(obj);
    assert(class != NULL, fwprintf(stderr, L"%p\n", obj));
    assert0((Optr)class != nil);
    if (pclass((Optr)class) == metaclass) {
        fwprintf(stderr, L"Class class: %ls\n", ((Class)obj)->name->value);
        return;
    }
    fwprintf(stderr, L"%p Class: %p %ls\n", obj, class, class->name->value);
}

void print_EXP()
{
    long size = EXP_SIZE();
    uns_int cur = 0;
    if (size < 0) {
        fwprintf(stderr, L"!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! Stack underflow!\n");
        return;
    }
    while (cur < size) {
        Optr c = tget(Double_Stack)[cur++];
        if (c > (Optr)10000) {
            print_Class(c);
        } else {
            fwprintf(stderr, L"%li\n", (uns_int)c);
        }
    }
}

void exps() {
    print_EXP();
}


void print_Symbol(Optr s)
{
    Optr tag = GETTAG(s);
    if (TAG_IS_LAYOUT(tag, Words)) {
        fwprintf(stderr, L"\"%ls\"\n", ((Symbol)s)->value);
    } else {
        fwprintf(stderr, L"Not a symbol: %p\n", s);
        print_Class(s);
    }
}

uns_int nrsends()
{
    uns_int size = EXP_SIZE();
    uns_int cur = 0;
    uns_int nr = 0;
    while (cur < size) {
        Optr c = tget(Double_Stack)[cur++];
        if (c > (Optr)10000 && pclass(c) == Send_Class) {
            nr++;
        }
    }
    return nr;
}

void sends()
{
    uns_int size = EXP_SIZE();
    uns_int cur = 0;
    while (cur < size) {
        Optr c = tget(Double_Stack)[cur++];
        if (c > (Optr)10000 && pclass(c) == Send_Class) {
            Send send = (Send)c;
            print_Info(send->info);
            print_Symbol((Optr)send->message);
        }
    }
}


Optr atn(Optr o, const wchar_t * s)
{
    Optr tag = GETTAG(o);
    if (TAG_IS_LAYOUT(tag, Object)) {
        int i;
        uns_int size = ((Array)tag)->size;
        for (i = 0; i < size; i++) {
            Slot v = (Slot)((Array)tag)->values[i];
            Symbol sym = (Symbol)v->name;
            if (wcsncmp(sym->value, s, sym->size)) { continue; }
            return ((Object)o)->ivals[i];
        }
        assert(NULL, fwprintf(stderr, L"Var not found: %ls\n", s););
    }
    if (TAG_IS_LAYOUT(tag, Array)) {
        uns_int size = ((Array)tag)->size;
        int i;
        for (i = 0; i < size; i++) {
            Slot v = (Slot)((Array)tag)->values[i];
            Symbol sym = (Symbol)v->name;
            if (wcsncmp(sym->value, s, sym->size)) { continue; }
            return ((Array)o)->values[i];
        }
        assert(NULL, fwprintf(stderr, L"Var not found: %ls\n", s););
    }
    assert(NULL, fwprintf(stderr, L"Non-indexable object\n"););
    return NULL;
}

Optr dict_at(Optr o, uns_int at)
{
    Dictionary dict = (Dictionary)o;
    uns_int ds = dict->data->size;
    uns_int i;
    uns_int idx = 0;
    for (i = 0; i < ds; i++) {
        Array bucket = (Array)dict->data->values[i];
        if (bucket == (Array)nil) { continue; }
        uns_int j;
        for (j = 0; j < bucket->size; j+=2) {
            if (bucket->values[j] == nil) { break; }
            if (idx == at) {
                return bucket->values[j+1];
            }
            idx++;
        }
    }
    return NULL;
}

Optr at(Optr o, uns_int i)
{
    if (pclass(o) == Dictionary_Class) {
        return dict_at(o, i);
    }
    Optr tag = GETTAG(o);
    if (TAG_IS_LAYOUT(tag, Object)) {
        uns_int size = ((Array)tag)->size;
        assert0(i < size);
        return ((Object)o)->ivals[i];
    }
    if (TAG_IS_LAYOUT(tag, Array)) {
        uns_int size = ((Array)tag)->size;
        uns_int isize = ((Array)o)->size;
        assert0(i < size + isize);
        return ((Array)o)->values[i];
    }
    assert(NULL, fwprintf(stderr, L"Non-indexable object\n"););
    return NULL;
}

Optr atx(Optr o, uns_int argc, ...)
{
    va_list args;
    va_start(args, argc);
    int index;
    for (index = 0; index < argc; index++) {
        o = at(o, va_arg(args, uns_int));
    }
    va_end(args);
    return o;
}

void shallow_inspect(Optr o)
{
    if (o == NULL) {
        fwprintf(stderr, L"NULL object\n");
        return;
    }
    if (o < (Optr)100000) {
        fwprintf(stderr, L"Object probably uns_int: %lu\n", (uns_int)o);
        return;
    } 
    if (pclass(o) == NULL) {
        fwprintf(stderr, L"Object with NULL class\n");
        return;
    }
    if (o == nil) {
        fwprintf(stderr, L"nil\n");
        return;
    }
    if (o == (Optr)true) {
        fwprintf(stderr, L"true\n");
        return;
    }
    if (o == (Optr)false) {
        fwprintf(stderr, L"false\n");
        return;
    }
    Class cls = pclass(o);

    if (pclass((Optr)cls) == metaclass) {
        fwprintf(stderr, L"%ls", ((Class)o)->name->value);
        fwprintf(stderr, L" (%lu)", (uns_int)o);
    } else {
        fwprintf(stderr, L"Instance of %ls", cls->name->value);
        fwprintf(stderr, L" (%p)", o);
    }

    Optr tag = GETTAG(o);
    if (TAG_IS_LAYOUT(tag, Words)) {
        fwprintf(stderr, L": '%ls'\n", ((Symbol)o)->value);
        return;
    }
    if (TAG_IS_LAYOUT(tag, Int)) {
        fwprintf(stderr, L": %i\n", ((SmallInt)o)->value);
        return;
    }
    fwprintf(stderr, L"\n");
}

void inspect_dict(Optr o)
{
    Dictionary dict = (Dictionary)o;
    uns_int ds      = dict->data->size;
    uns_int i;
    uns_int idx = 0;
    for (i = 0; i < ds; i++) {
        Array bucket = (Array)dict->data->values[i];
        if (bucket == (Array)nil) { continue; }
        uns_int j;
        for (j = 0; j < bucket->size; j+=2) {
            Optr key = bucket->values[j];
            if (key == nil) { break; }
            fwprintf(stderr, L"%lu ", idx++);
            shallow_inspect(key);
            fwprintf(stderr, L" -> ");
            shallow_inspect(bucket->values[j+1]);
        }
    }
}

void inspect(Optr o)
{
    shallow_inspect(o);
    if (o == NULL || pclass(o) == NULL) {
        return;
    }
    if (pclass(o) == Dictionary_Class) {
        inspect_dict(o);
        return;
    }
    Optr tag = GETTAG(o);
    if (TAG_IS_LAYOUT(tag, Object)) {
        uns_int size = ((Array)tag)->size;
        int i;
        for (i = 0; i < size; i++) {
            Slot v = (Slot)((Array)tag)->values[i];
            fwprintf(stderr, L"%i %15ls:\t", i, ((Symbol)v->name)->value);
            shallow_inspect(((Object)o)->ivals[i]);
        }
        return;
    }

    if (TAG_IS_LAYOUT(tag, Array)) {
        uns_int size  = ((Array)tag)->size;
        uns_int isize = ((Array)o)->size;
        int i;
        for (i = 0; i < size; i++) {
            Slot v = (Slot)((Array)tag)->values[i];
            fwprintf(stderr, L"%i %15ls:\t", i, ((Symbol)v->name)->value);
            shallow_inspect(((Array)o)->values[i]);
        }
        for (; i < size + isize; i++) {
            fwprintf(stderr, L"%i:\t", i);
            shallow_inspect(((Array)o)->values[i]);
        }
        return;
    }
}

void i(Optr o) 
{
    inspect(o);
}


void inspect_at(Optr o, uns_int i)
{
    inspect(at(o, i));
}

void i_at(Optr o, uns_int i) {
    return inspect_at(o, i);
}


void inspect_atn(Optr o, const wchar_t * s)
{
    inspect(atn(o, s));
}

void i_atn(Optr o, const wchar_t * s) {
    return inspect_atn(o, s);
}

void i_atx(Optr o, uns_int argc, ...) {
    return inspect(atx(o, argc));
}

Optr methods(Optr o) {
    Class class = pclass(o);
    return (Optr)class->methods;
}
