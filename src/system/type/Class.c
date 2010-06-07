#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <stdarg.h>
#include <pinocchio.h>
#include <lib/lib.h>
#include <system/runtime/InlineCache.h>

/* ========================================================================= */

Type_Class Metaclass;
Type_Class Class;
Type_Class Behavior;

/* ========================================================================= */

#include <system/type/Instantiate.ci>

/* ========================================================================= */

Type_Class new_Bootstrapping_Class()
{
    Type_Class mcls =
        (Type_Class)basic_instantiate_Object(Metaclass, METACLASS_SIZE);
    Type_Class cls  = (Type_Class)basic_instantiate_Object(mcls, CLASS_SIZE);
    // Name of metaclass is its instance pointer.
    mcls->name = (Symbol)cls;
    return cls;
}

Type_Class new_Class(Type_Class superclass, Object metatype)
{    
    ASSERT_TAG_LAYOUT(metatype, Object);
    uns_int meta_size = ((Array)metatype)->size;
    assert0(meta_size >= 4); // we need at least place for
                             // methods, super, layout and instance.

    Type_Class metaclass    = (Type_Class)basic_instantiate_Object(Metaclass, METACLASS_SIZE);
    metaclass->layout       = metatype;
    Type_Class result       = (Type_Class)basic_instantiate_Object(metaclass, meta_size);
    metaclass->name         = (Symbol)result;

    result->methods         = new_Dictionary();
    metaclass->methods      = new_Dictionary();
    Type_Class_set_superclass(result, superclass);

    return result;
}

void Type_Class_set_superclass(Type_Class cls, Type_Class superclass)
{
    Type_Class metaclass = HEADER(cls);
    cls->super           = superclass;
    if (superclass != (Type_Class)Nil) {
        metaclass->super = HEADER(superclass);
    } else {
        metaclass->super = Class;
    }
}

/* ========================================================================= */

void assert_class(Object class)
{
    assert0(HEADER(class) == Metaclass ||        /* if metaclass */
            HEADER(HEADER(class)) == Metaclass); /* if class */
}

CNT(Class_super)
    Object class = peek_EXP(0);
    assert_class(class);
    poke_EXP(0, ((Type_Class)class)->super);
}


void Method_invoke(Object method, Object self, uns_int argc) {
    if (HEADER(method) == Runtime_MethodClosure_Class) {
        Runtime_MethodClosure_invoke((Runtime_MethodClosure)method, self, argc);
    } else {
        // inspect(method);
        assert1(NULL, "Unknown type of method installation");
    }
}

void does_not_understand(Object self, Type_Class class, Object msg, uns_int argc)
{
    if (msg == (Object)SMB_doesNotUnderstand_) {
        Runtime_Message message = (Runtime_Message)pop_EXP();
        // For now keep on printing the message.
        fwprintf(stderr, L"\033[33mDNU: %ls >> %ls\033[0m\n",
                 class->name->value,
                 message->selector->value);
        fail(Exception_RecursiveDoesNotUnderstand_Class, 3,
             self, class, message);
    }

	Runtime_Message message = new_Runtime_Message(msg, argc);

	while (argc--) {
		message->arguments[argc] = pop_EXP();
	}

    zapn_EXP(2);

    Type_Class_direct_dispatch(self, class, (Object)SMB_doesNotUnderstand_, 1, message);
}

static CNT(Class_lookup_cache_invoke)
    Object method    = peek_EXP(0);
    uns_int argc     = (uns_int)peek_EXP(3);
    Type_Class class = (Type_Class)peek_EXP(4);
    Object self      = peek_EXP(2);
    if (method == NULL) {
        Object msg  = peek_EXP(1);
        zapn_EXP(5);
        return does_not_understand(self, class, msg, argc);
    }
    zapn_EXP(5);
    AST_Send send       = (AST_Send)peek_EXP(argc + 1);
    Array cache    = send->cache;
    Runtime_InlineCache_store(cache, (Object)class, method);
    
    Method_invoke(method, self, argc);
}

static CNT(Class_lookup_invoke)
    Object method = peek_EXP(0);
    uns_int argc  = (uns_int)peek_EXP(3);
    Object self   = peek_EXP(2);
    if (method == NULL) {
        Type_Class class = (Type_Class)peek_EXP(4);
        Object msg       = peek_EXP(1);
        zapn_EXP(5);
        return does_not_understand(self, class, msg, argc);
    }
    zapn_EXP(5);
    Method_invoke(method, self, argc);
}

void Class_lookup(Type_Class class, Object msg)
{
    // TODO pass along the hash value
    if (class == (Type_Class)Nil) {
        poke_EXP(0, NULL);
        zap_CNT();
        return;
    }
    assert_class((Object)class);
    Dictionary mdict = class->methods;
    Dictionary_lookup_push(mdict, msg);
}

void CNT_Class_lookup_loop()
{
    Object method = peek_EXP(0);
    if (method != NULL) {
        zap_EXP();
        zap_CNT();
        poke_EXP(0, method);
        return;
    }
    zap_EXP();
    Type_Class class = (Type_Class)peek_EXP(0);
    Object msg       = peek_EXP(1);
    Type_Class next  = class->super;
    poke_EXP(0, next);
    return Class_lookup(next, msg);
}

static void Class_direct_dispatch(Object self, Type_Class class,
                                  Object msg, uns_int argc)
{
    push_EXP(class);
    push_EXP(argc);
    push_EXP(self);

    push_EXP(msg);
    push_EXP(class);
    push_CNT(Class_lookup_loop);
    Class_lookup(class, msg);
}

CNT(restore_iss)
    Object return_value = pop_EXP();
    tset(_ISS_, peek_EXP(0));
    poke_EXP(0, return_value);
}

void Type_Class_tower_dispatch(Object self, Object class,
                               Type_Object iss, Runtime_Message message)
{
    tset(_ISS_, Nil);
    push_EXP(iss);
    push_CNT(restore_iss);
    push_CNT(Class_lookup_invoke);
    Type_Object tower = (Type_Object)Nil;
    while (iss != (Type_Object)Nil) {
        Type_Object newtower = (Type_Object)instantiate((Type_Class)Collection_Link_Class);
        newtower->ivals[0] = iss->ivals[0];
        newtower->ivals[1] = (Object)tower;
        tower = newtower;
        iss = (Type_Object)iss->ivals[1];
    }
    push_EXP(Nil);
    push_EXP(tower->ivals[0]); // self, bottom interpreter
    push_EXP(message);
    push_EXP(self); // receiver
    push_EXP(class);
    push_EXP(tower->ivals[1]); // tower of interpreters
    self = tower->ivals[0];
    Class_direct_dispatch(
        self,
        HEADER(self),
        (Object)SMB_send_to_class_inInterpreterChain_,
        4);
}

void Type_Class_direct_dispatch(Object self, Type_Class class, Object msg,
                                uns_int argc, ...)
{
    va_list args;
    va_start(args, argc);
    int idx;
    /* Send obj. TODO update Send>>eval to be able to remove this */
    /* TODO optimize by claim + poke instead of push */
    Type_Object iss = (Type_Object)tget(_ISS_);
    if ((Object)iss == Nil) {
        push_EXP(Nil);
        push_EXP(self);
        for (idx = 0; idx < argc; idx++) {
            push_EXP(va_arg(args, Object));
        }
        va_end(args);
        push_CNT(Class_lookup_invoke);
        Class_direct_dispatch(self, class, msg, argc);
    } else {
        Runtime_Message message = new_Runtime_Message(msg, argc);
        for (idx = 0; idx < argc; idx++) {
            message->arguments[idx] = va_arg(args, Object);
        }
        va_end(args);
        Type_Class_tower_dispatch(self, (Object)class, iss, message);
    }
}

void Type_Class_direct_dispatch_withArguments(Object self, Type_Class class,
                                              Object msg, Array args)
{
    /* Send obj. TODO update Send>>eval to be able to remove this */
    int idx;
    Type_Object iss = (Type_Object)tget(_ISS_);
    if ((Object)iss == Nil) {
        push_EXP(Nil);
        push_EXP(self);
        for (idx = 0; idx < args->size; idx++) {
            push_EXP(args->values[idx]);
        }
        push_CNT(Class_lookup_invoke);
        Class_direct_dispatch(self, class, msg, args->size);
    } else {
        Runtime_Message message = new_Runtime_Message(msg, args->size);
        for (idx = 0; idx < args->size; idx++) {
            message->arguments[idx] = args->values[idx];
        }
        Type_Class_tower_dispatch(self, (Object)class, iss, message);
    }
}

void Type_Class_dispatch(Object self, Type_Class class, uns_int argc)
{
    AST_Send send       = (AST_Send)peek_EXP(argc + 1); // + self
    Array cache    = send->cache;
    Object msg          = send->message;
    assert0(msg != Nil);

    #ifdef PRINT_DISPATCH_TRACE
    Symbol clsname;
    if (HEADER(class) != Metaclass) {
        clsname = String_concat_(((Type_Class)class)->name,
                                      new_String(L">>"));
    } else {
        clsname = String_concat_(((Type_Class)self)->name,
                                      new_String(L" class>>"));
    }
    Symbol msgname = (Symbol)msg;
    Symbol method  = String_concat_(clsname, msgname);
    LOG("%ls (%"F_I"u)\n", method->value, self);
    #endif // PRINT_DISPATCH_TRACE
    
    // TODO properly initialize the inlinecache when creating new sends
    if ((Object)cache != Nil) {
        Object method = Runtime_InlineCache_lookup(cache, (Object)class);
        if (method) {
            return Method_invoke(method, self, argc);
        }
    } else {
        send->cache = new_Runtime_InlineCache();
    }
    assert_class((Object)class);
    
    push_CNT(Class_lookup_cache_invoke);
    return Class_direct_dispatch(self, class, msg, argc);
}
