
void test_ast_continue()
{
    Type_SmallInt * integer1      = new_SmallInt(1);
    Type_SmallInt * integer7      = new_SmallInt(7);
    AST_Constant * integer1_const = new_Constant((Object)integer1);
    AST_Constant * integer7_const = new_Constant((Object)integer7);
    
    AST_Callec * callec = new_Callec();
    callec->target = (Object)integer7_const;
    Object result = Eval((Object)callec);
    //printf("%ls\n", Object_classname(result));
    assert(result == (Object)integer7);
    
    
    Type_Array * body = new_Array_With(2, (Object)integer7_const);
    AST_Method * send = new_Method(1, body);
    AST_Constant* send_const = new_Constant((Object)send);
    callec->target    = (Object)new_Send((Object)send_const, 
                                           (Object)Symbol_eval_, 
                                           new_Array_With(1, (Object)new_Constant((Object)callec->cont)));
    
    //(assert (= 1
    //     (callec (lambda (cont)
    //                 7
    //                 7))))
    //
    result = Eval((Object)callec);
    //printf("%ls\n", Object_classname(result));
    assert(result == (Object)integer7);
    
    result = Eval((Object)callec);
    // printf("%ls\n", Object_classname(result));
    assert(result == (Object)integer7);
    
    //(assert (= 1
    //     (callec (lambda (cont)
    //                 (cont 1)
    //                 7))))
    //
    body->values[0]    = (Object)new_Send((Object)new_Constant((Object)callec->cont), 
                                          Symbol_eval, 
                                          new_Array_With(1, (Object)integer1_const));
    
    result = Eval((Object)callec);
    printf("%ls\n", Object_classname(result));
    assert(result == (Object)integer1);
}


/* ========================================================================== */

void test_Continue()
{
    test_ast_continue();
}