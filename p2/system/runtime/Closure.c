
#include <stdlib.h>
#include <stdio.h>
#include <system/runtime/Closure.h>

/* ========================================================================= */

Type_Class Runtime_Closure_Class;

/* ========================================================================= */

Runtime_Closure new_Runtime_Closure(AST_Block code, Runtime_BlockContext context) {
    NEW_OBJECT(Runtime_Closure);
    result->code    = code;
    result->context = context;
    result->info    = empty_AST_Info;
    assert0(empty_AST_Info != NULL);
    assert0(result->info != NULL);
    return result;
}

void pre_init_Runtime_Closure()
{
    Runtime_Closure_Class = new_Named_Class((Object)Type_Object_Class,
                                            L"Runtime_Closure",
                                            create_type(3, OBJECT));
}

/* ========================================================================= */

void Runtime_Closure_invoke(Runtime_Closure closure, Object self,
                            Object class, Type_Array args)
{
    LOG_AST_INFO("Closure Invoke: ", closure->info);
     
    if (closure->code->body->size == 0) { 
        poke_EXP(1, self);
        return; 
    }
    
    push_restore_env(); // pokes EXP
    
    Runtime_MethodContext env =
        new_Runtime_MethodContext(closure, self, class, NULL, args);

    env->home_context = env;
    
    Env = (Object)env;

    
    if (1 < closure->code->body->size) {
        push_CNT(AST_Block_continue);
    }
    
    push_EXP(closure->code->body->values[0]);
    push_CNT(send_Eval);
}

Type_Array expand_for_locals(Type_Array args, int locals)
{
    if (locals == 0) { return args; }

    Type_Array scope = new_Raw_Type_Array(locals + args->size);
    int i;
    for (i = 0; i < args->size; i++) {
        scope->values[i] = args->values[i];
    }
    for (; i < args->size + locals; i++) {
        scope->values[i] = Nil;
    }
    return scope;
}

void Runtime_Closure_apply(Runtime_Closure closure, Type_Array args)
{
    #ifdef DEBUG
    LOG("Closure Apply\n");
    #endif // DEBUG
    
    ASSERT_ARG_SIZE(closure->code->paramCount->value);

    if (closure->code->body->size == 0) { 
        poke_EXP(1, Nil);
        return; 
    }
    
    // TODO check if we call closure from source location. if so just pop
    // env-frame.
    push_restore_env();  // pokes EXP
    args = expand_for_locals(args, closure->code->localCount->value);
    Runtime_BlockContext env = new_Runtime_BlockContext(closure, args);
    
    Env = (Object)env;
    
    if (1 < closure->code->body->size) {
        push_CNT(AST_Block_continue);
    }
    
    push_EXP(closure->code->body->values[0]);
    push_CNT(send_Eval);
}

NATIVE(Runtime_Closure_apply_)
    Runtime_Closure closure = (Runtime_Closure)self;
    Runtime_Closure_apply(closure, args);
}

/* ========================================================================= */

void post_init_Runtime_Closure()
{
    store_native_method(Runtime_Closure_Class, SMB_apply_, NM_Runtime_Closure_apply_);
    store_native_method(Runtime_Closure_Class, SMB_apply,  NM_Runtime_Closure_apply_);
}
