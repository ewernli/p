#include <stdlib.h>
#include <stdio.h>
#include <system/slot/UIntSlot.h>

/* ========================================================================= */

UIntSlot new_UIntSlot(uns_int index, const wchar_t * name) 
{
    NEW_OBJECT(UIntSlot);
    result->index   = index;
    result->name    = (Object)new_Symbol(name);
    result->package = (Object)Nil;
    return result;
}

/* ========================================================================= */


static Object UIntSlot_readFrom_(UIntSlot var, Object self)
{
    return wrap_int((uns_int)Object_instVarAt_(
						self,
						var->index));
}

static void UIntSlot_assign_on_(UIntSlot var,
                                              Object value,
                                              Object self)
{
    Object_instVarAt_put_(self,
						  var->index,
						  (Object)(uns_int)unwrap_int(value));
}
void UIntSlot_eval(UIntSlot var)
{
    poke_EXP(0, UIntSlot_readFrom_(var,
                    current_env()->home_context->self));
}

void UIntSlot_assign(UIntSlot var, Object value)
{
    UIntSlot_assign_on_(var, value, current_env()->home_context->self);
}

/* ========================================================================= */

NATIVE1(UIntSlot_readFrom_)
    Object receiver = NATIVE_ARG(0);
    RETURN_FROM_NATIVE(
        UIntSlot_readFrom_((UIntSlot)self,
                                         receiver));
}

NATIVE2(UIntSlot_assign_on_)
    Object value = NATIVE_ARG(0);
    Object o = NATIVE_ARG(1);
    UIntSlot_assign_on_((UIntSlot)self,
                                value, o);
    RETURN_FROM_NATIVE(value);
}

/* ========================================================================= */

void post_init_UIntSlot()
{
    Dictionary natives = add_plugin(L"Slot.UIntSlot");
    store_native(natives, SMB_assign_on_, NM_UIntSlot_assign_on_);
    store_native(natives, SMB_readFrom_ , NM_UIntSlot_readFrom_);
}
