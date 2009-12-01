#ifndef SEND_H
#define SEND_H

#include <pinocchio.h>

/* ========================================================================= */

#define AST_SEND_SIZE 4
#define AST_SEND_VARS L"cache", L"message", L"info", L"receiver"

struct AST_Send_t {
    uns_int             size;
    Runtime_InlineCache cache;
    Object              message;
    AST_Info            info;
    Object              receiver;
    Object              arguments[];
};

CREATE_INITIALIZERS(AST_Send)

extern AST_Send new_AST_Send_raw(Object receiver, Object msg, uns_int argc);
extern AST_Send new_AST_Send(Object receiver, Object msg, uns_int argc, ...);

/* ========================================================================= */
extern Type_Array AST_Send_args(AST_Send send);

extern void AST_Send_eval(AST_Send self);
extern void eval_store(uns_int idx);
extern void CNT_store_argument();

/* ========================================================================= */

#endif // SEND_H
