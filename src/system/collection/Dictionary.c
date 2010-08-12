#include <stdlib.h>
#include <stdio.h>
#include <system/collection/Dictionary.h>

/* ========================================================================= */

DECLARE_CLASS(Dictionary);

static Symbol SMB_hash;
static Symbol SMB__equals_;

/* ========================================================================= */

Dictionary new_Dictionary()
{
    NEW_OBJECT(Dictionary);
    result->size      = 0;
    result->ratio     = 500;
    result->maxLinear = 20;
    result->data      = new_Array_withAll(1, (Optr)new_DictBucket(20 << 1));
    result->linear    = true;
    return result;
}

/* ========================================================================= */

static long get_hash(Dictionary self, Optr key)
{
    long hash;
    Optr tag = GETTAG(key);
    if (TAG_IS_LAYOUT(tag, Words)) {
        hash = Symbol_hash((Symbol)key)->value;
    } else if (TAG_IS_LAYOUT(tag, Int)) { 
        hash = ((SmallInt)key)->value;
    } else {
        assert1(NULL, "Quick dictionaries only support ints and strings!");
        // make the compiler happy :)
        return 0;
    }
    hash %= self->data->size;
    return hash;
}

long unwrap_hash(Dictionary self, Optr w_hash)
{
    return unwrap_int(w_hash) % self->data->size;
}

DictBucket * get_bucketp(Dictionary dictionary, long hash)
{
    return (DictBucket *)&dictionary->data->values[hash];
}

/* ========================================================================= *
 * Quick dictionary functions. Only for bootstrapping.                       *
 * ========================================================================= */

long Dictionary_grow_check(Dictionary self)
{
    self->size++;
    if (self->data->size == 1) {
        return self->size == self->maxLinear;
    }
    uns_int size = self->data->size;
    return (100 * self->size) / size > self->ratio;
}

Optr Dictionary_quick_lookup(Dictionary self, Optr key)
{
    long hash = get_hash(self, key);
    DictBucket * bucketp = get_bucketp(self, hash);
    DictBucket bucket = *bucketp;
    if (bucket == (DictBucket)nil) {
        return NULL;
    }
    long i;
    uns_int tally = bucket->tally;
    for (i = 0; i < tally; i=i+2) {
        switch (Bucket_quick_compare_key(key, bucket->values[i]))
        {
            case -1: assert1(NULL, "Invalid key for quickstore!\n");
            case 1:
                return bucket->values[i+1];
        }
    }
    return NULL;
}

void remove_from_bucket(uns_int idx, DictBucket bucket)
{
    uns_int tally = bucket->tally;
    bucket->values[idx]     = bucket->values[tally-2];
    bucket->values[idx+1]   = bucket->values[tally-1];
    bucket->values[tally-2] = nil;
    bucket->values[tally-1] = nil;
    bucket->tally = tally - 2;
}

void add_to_bucket(DictBucket * bucketp, Optr key, Optr value)
{
    if ((Optr)*bucketp == nil) {
        *bucketp = new_bucket();
    } else if ((*bucketp)->tally == (*bucketp)->size) {
        Bucket_grow(bucketp);
    }

    DictBucket b       = *bucketp;
    uns_int tally      = b->tally;
    b->values[tally]   = key;
    b->values[tally+1] = value;
    b->tally = tally + 2;
}

static void Dictionary_quick_check_grow(Dictionary self)
{
    if (!Dictionary_grow_check(self)) { return; }

    Array old = self->data;
    if (old->size == 1) {
        self->data = new_Array_withAll(32, nil);
        self->linear = false;
    } else {
        self->data = new_Array_withAll(old->size << 1, nil);
    }
    uns_int i;
    for (i = 0; i < old->size; i++) {
        DictBucket bucket = (DictBucket)old->values[i];
        if (bucket == (DictBucket)nil) { continue; }
        self->data->values[i] = (Optr)bucket;
        uns_int j;
        for (j = 0; j < bucket->tally;) {
            Optr key = bucket->values[j];
            long hash = get_hash(self, key);
            if (hash != i) {
                DictBucket * bucketp = get_bucketp(self, hash);
                add_to_bucket(bucketp, key, bucket->values[j+1]);
                remove_from_bucket(j, bucket);
            } else {
                j = j + 2;
            }
        }
    }
}

void Dictionary_quick_store(Dictionary self,
                                 Optr key, Optr value)
{
    assert0(self != (Dictionary)nil);
    long hash = get_hash(self, key);
    DictBucket * bucketp = get_bucketp(self, hash);
    if (*bucketp == (DictBucket)nil) {
        *bucketp                     = new_bucket();
        DictBucket bucket = *bucketp;
        bucket->values[0] = key;
        bucket->values[1] = value;
        bucket->tally     = 2;
        return Dictionary_quick_check_grow(self);
    }
    if (Bucket_quick_store(bucketp, key, value)) {
        Dictionary_quick_check_grow(self);
    }
}

int tpush_hash(Optr key)
{
    SmallInt hash;
    Optr tag = GETTAG(key);
    if (TAG_IS_LAYOUT(tag, Words)) {
        hash = Symbol_hash((Symbol)key);
    } else if (TAG_IS_LAYOUT(tag, Int)) { 
        hash = (SmallInt)key;
    } else {
        Class_direct_dispatch(key, HEADER(key), (Optr)SMB_hash, 0);
        return 1;
    }
    PUSH_EXP(hash);
    return 0;
}



NNATIVE(Dictionary_grow, 2,
    t_dict_grow,
    t_bucket_rehash)

void Dictionary_grow(Dictionary self)
{
    Array old  = self->data;
    if (old->size == 1) {
        self->data = new_Array_withAll(32, (Optr)nil);
        self->linear = false;
    } else {
        self->data = new_Array_withAll(old->size << 1, (Optr)nil);
    }

    uns_int i = 0;
    for (i = 0; i < old->size; i++) {
        self->data->values[i] = old->values[i];
    }
    
    CLAIM_EXP(5);
    POKE_EXP(4, old);
    POKE_EXP(3, 0);
    POKE_EXP(2, self);
    push_code(T_Dictionary_grow);
}

/* ========================================================================= */

int Bucket_compare_key(Optr inkey, Optr dictkey)
{
    long result = Bucket_quick_compare_key(inkey, dictkey);

    if (result == -1) {
        Class_direct_dispatch(inkey, HEADER(inkey),
                              (Optr)SMB__equals_, 1, dictkey);
        return 1;
    }
    PUSH_EXP(get_bool(result));
    return 0;
}


NNATIVE(iDictionary_at_, 5,
    t_push_hash,
    t_dictionary_bucket,
    t_bucket_lookup,
    t_return,
    t_return_null)

void Dictionary_lookup_push(Dictionary dict, Optr msg)
{
    CLAIM_EXP(2);
    POKE_EXP(1, dict);
    POKE_EXP(0, msg);
    push_code(T_iDictionary_at_);
}

/* ========================================================================= */

NNATIVE(Dictionary_at_, 5,
    t_push_hash,
    t_dictionary_bucket,
    t_bucket_lookup,
    t_return,
    t_return_nil)

NATIVE1(Dictionary_at_)
    push_code(T_Dictionary_at_);
}

NNATIVE(Dictionary_at_ifAbsent_, 5,
    t_push_hash,
    t_dictionary_bucket,
    t_bucket_lookup,
    t_pop_return,
    t_dictionary_ifAbsent_)

NATIVE2(Dictionary_at_ifAbsent_)
    Optr w_index = NATIVE_ARG(0);
    Optr w_block = NATIVE_ARG(1);
    POKE_EXP(2, w_block);
    POKE_EXP(1, self);
    POKE_EXP(0, w_index);
    push_code(T_Dictionary_at_ifAbsent_);
}

void Bucket_store(DictBucket bucket, Optr key, Optr value, uns_int idx)
{
    bucket->values[idx]   = key;
    bucket->values[idx+1] = value;
}


NNATIVE(Dictionary_at_put_, 5,
    t_peek1,
    t_push_hash,
    t_dictionary_store,
    t_bucket_store,
    t_dictionary_check_grow);

NATIVE2(Dictionary_at_put_)
    push_code(T_Dictionary_at_put_);
}

NATIVE0(Dictionary_grow)
    Dictionary_grow((Dictionary)self);
}

void post_init_Dictionary()
{
    SMB_hash     = new_Symbol_cached(L"hash");
    SMB__equals_ = new_Symbol_cached(L"=");

    change_slot_type(Dictionary_Class, UIntSlot_Class, 3, 0,1,2);
    
    INIT_NATIVE(Dictionary_at_);
    INIT_NATIVE(Dictionary_at_put_);
    INIT_NATIVE(Dictionary_at_ifAbsent_);
    INIT_NATIVE(iDictionary_at_);
    INIT_NATIVE(Dictionary_grow);

    Dictionary natives = add_plugin(L"Collection.Dictionary");
    store_native(natives, L"at:put:",      NM_Dictionary_at_put_);
    store_native(natives, L"at:",          NM_Dictionary_at_);
    store_native(natives, L"at:ifAbsent:", NM_Dictionary_at_ifAbsent_);
    store_native(natives, L"grow",         NM_Dictionary_grow);
}
