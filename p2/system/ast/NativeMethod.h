
/* ======================================================================== */

extern AST_Native_Method * new_Native_Method(native code);

/* ======================================================================== */

extern void AST_Native_Method_invoke(AST_Native_Method * method, Object self, Object class, Type_Array * args);

extern void AST_Native_Method_eval(Object self, Object class, Type_Array * args);

extern void AST_Native_Method_eval_(Object self, Object class, Type_Array * args);

/* ======================================================================== */

extern void pre_initialize_Native_Method();
extern void post_initialize_Native_Method();