#include <stdlib.h>
#include <stdio.h>
#include <system/collection/Dictionary.h>

/* ========================================================================= */

DECLARE_CLASS(Collection_Dictionary);

/* ========================================================================= */

Collection_Dictionary new_Collection_Dictionary()
{
    NEW_OBJECT(Collection_Dictionary);
    result->size  = new_Type_SmallInt(0);
    result->ratio = new_Type_Float(0.6);
    result->data  = new_Type_Array_withAll(DICTIONARY_SIZE, Nil);
    return result;
}

void pre_init_Collection_Dictionary()
{
    Collection_Dictionary_Class = new_Class(Type_Object_Class,
                                      CREATE_OBJECT_TAG(COLLECTION_DICTIONARY));
    REFER_TO(Collection_Dictionary);
}

/* ========================================================================= */

int get_hash(Collection_Dictionary self, Object key)
{
    int hash;
    Object tag = GETTAG(key);
    if (TAG_IS_LAYOUT(tag, Words)) {
        hash = Type_Symbol_hash((Type_Symbol)key)->value;
    } else if (TAG_IS_LAYOUT(tag, Int)) { 
        hash = ((Type_SmallInt)key)->value;
    } else {
        printf("Got key: %p\n", key);
        print_Class(key);
        assert1(NULL, "Dictionary currently only supports"
                      " SmallInt and Symbol as key\n");
        // make the compiler happy :)
        return 0;
    }
    hash %= self->data->size;
    return hash;
}

void push_hash(Object key)
{
    int hash;
    Object tag = GETTAG(key);
    if (TAG_IS_LAYOUT(tag, Words)) {
        hash = Type_Symbol_hash((Type_Symbol)key)->value;
    } else if (TAG_IS_LAYOUT(tag, Int)) { 
        hash = ((Type_SmallInt)key)->value;
    } else {
        return Type_Class_direct_dispatch(key, HEADER(key), (Object)SMB_hash, 0);
    }
    push_EXP(new_Type_SmallInt(hash));
}

int unwrap_hash(Collection_Dictionary self, Object w_hash)
{
    return unwrap_int(w_hash) % self->data->size;
}


/* ========================================================================= *
 * Bucket functions                                                          *
 * ========================================================================= */

static Type_Array * get_bucketp(Collection_Dictionary dictionary, int hash)
{
    return (Type_Array *)&dictionary->data->values[hash];
}

static Type_Array new_bucket()
{
    return new_Type_Array_withAll(DICTIONARY_BUCKET_SIZE * 2, Nil);
}

static void Bucket_grow(Type_Array * bucketp)
{
    Type_Array old_bucket = *bucketp;
    Type_Array new_bucket = new_Type_Array_raw(old_bucket->size << 1);
    int i;
    for(i = 0; i < old_bucket->size; i++) {
        new_bucket->values[i] = old_bucket->values[i];
    }
    for(; i < new_bucket->size; i++) {
        new_bucket->values[i] = (Object)Nil;
    }

    *bucketp = new_bucket;
}

static int Bucket_quick_compare_key(Object inkey, Object dictkey)
{
    if (HEADER(inkey) == Type_Symbol_Class) {
        return inkey == dictkey;
    }

    if (HEADER(inkey) == Type_SmallInt_Class) {
        if (HEADER(dictkey) == Type_SmallInt_Class) {
            return ((Type_SmallInt)inkey)->value ==
                   ((Type_SmallInt)dictkey)->value;
        } else {
            return 0;
        }
    }

    if (HEADER(inkey) == Type_String_Class) {
        if (TAG_IS_LAYOUT(GETTAG(dictkey), Words)) {
            return Words_compare((Type_Symbol)inkey, (Type_Symbol)dictkey);
        } else {
            return 0;
        }
    }
    return -1;
}

static int Bucket_quick_store(Type_Array * bucketp, Object key, Object value)
{
    int i;
    Type_Array bucket = *bucketp;
    for (i = 0; i < bucket->size; i = i+2) {
        if (bucket->values[i] == (Object)Nil) {
            bucket->values[i]   = key;
            bucket->values[i+1] = value;
            return 1;
        } else {
            switch (Bucket_quick_compare_key(key, bucket->values[i]))
            {
                case -1: assert1(NULL, "Invalid key for quickstore!\n");
                case 1:
                    bucket->values[i+1] = value;
                    return 0;
            }
        }
    }
    Bucket_grow(bucketp);
    bucket              = *bucketp;
    bucket->values[i]   = key;
    bucket->values[i+1] = value;
    return 1;
}

/* ========================================================================= *
 * Quick dictionary functions. Only for bootstrapping.                       *
 * ========================================================================= */

static int Collection_Dictionary_grow_check(Collection_Dictionary self)
{
    uns_int self_size = unwrap_int((Object)self->size);
    self->size        = new_Type_SmallInt(self_size + 1);
    float amount      = self_size + 1;
    float size        = self->data->size;
    return amount / size > unwrap_float((Object)self->ratio);
}

static void Collection_Dictionary_quick_check_grow(Collection_Dictionary self)
{
    if (!Collection_Dictionary_grow_check(self)) { return; }

    Type_Array old = self->data;
    self->data     = new_Type_Array_withAll(old->size << 1, (Object)Nil);
    self->size     = new_Type_SmallInt(0);
    int i;
    for (i = 0; i < old->size; i++) {
        Type_Array bucket = (Type_Array)old->values[i];
        if (bucket == (Type_Array)Nil) { continue; }
        int j;
        for (j = 0; j < bucket->size; j=j+2) {
            Object key = bucket->values[j];
            if (key == (Object)Nil) { break; }
            Collection_Dictionary_quick_store(self, key, bucket->values[j+1]);
        }
        
    }
}

void Collection_Dictionary_quick_store(Collection_Dictionary self,
                                 Object key, Object value)
{
    int hash             = get_hash(self, key);
    Type_Array * bucketp = get_bucketp(self, hash);
    if (*bucketp == (Type_Array)Nil) {
        *bucketp = new_bucket();
        Type_Array bucket = *bucketp;
        bucket->values[0] = key;
        bucket->values[1] = value;
        return Collection_Dictionary_quick_check_grow(self);
    }
    if (Bucket_quick_store(bucketp, key, value)) {
        Collection_Dictionary_quick_check_grow(self);
    }
}

Object Collection_Dictionary_quick_lookup(Collection_Dictionary self, Object key)
{
    int hash = get_hash(self, key);
    Type_Array * bucketp = get_bucketp(self, hash);
    Type_Array bucket = *bucketp;
    if (bucket == (Type_Array)Nil) {
        return NULL;
    }
    int i;
    for (i = 0; i < bucket->size; i=i+2) {
        if (bucket->values[i] == (Object)Nil) {
            return NULL;
        }
        switch (Bucket_quick_compare_key(key, bucket->values[i]))
        {
            case -1: assert1(NULL, "Invalid key for quickstore!\n");
            case 1:
                return bucket->values[i+1];
        }
    }
    return NULL;
}

/* ========================================================================= */

static void Bucket_compare_key(Object inkey, Object dictkey)
{
    int result = Bucket_quick_compare_key(inkey, dictkey);
    if (result == -1) {
        return Type_Class_direct_dispatch(inkey, HEADER(inkey),
                                          (Object)SMB__equal, 1, dictkey);
    }
    push_EXP(get_bool(result));
}

void CNT_bucket_lookup()
{
    Object boolean    = peek_EXP(0);
    Type_Array bucket = (Type_Array)peek_EXP(1);
    uns_int idx       = (uns_int)peek_EXP(2);
    
    if (boolean == (Object)True) {
        zapn_EXP(4);
        poke_EXP(0, bucket->values[idx + 1]);
        zap_CNT();
        return;
    }    

    idx += 2;

    if (idx >= bucket->size || bucket->values[idx] == (Object)Nil) {
        zapn_EXP(4);
        poke_EXP(0, NULL);
        zap_CNT();
        return;
    }

    zap_EXP();

    Object key = peek_EXP(2);
    poke_EXP(1, idx);
    Bucket_compare_key(key, bucket->values[idx]);
}

static void bucket_do_store(Type_Array bucket, uns_int idx, uns_int addition)
{
    Object value          = peek_EXP(3);
    bucket->values[idx+1] = value;
    zapn_EXP(3);
    poke_EXP(0, (Object)addition);
    zap_CNT();
}

static void bucket_store_new(Type_Array bucket, uns_int idx, Object key)
{
    bucket->values[idx] = key;
    bucket_do_store(bucket, idx, 1); 
}

static void CNT_Bucket_store()
{
    Object boolean       = pop_EXP();
    Type_Array * bucketp = (Type_Array *)peek_EXP(0);
    Type_Array bucket    = *bucketp;
    uns_int idx          = (uns_int)peek_EXP(1);

    if (boolean == (Object)True) {
        return bucket_do_store(bucket, idx, 0);
    }

    Object key = peek_EXP(2);
    idx += 2;

    if (idx >= bucket->size) {
        Bucket_grow(bucketp);
        return bucket_store_new(*bucketp, idx, key);
    }

    if (bucket->values[idx] == Nil) {
        return bucket_store_new(bucket, idx, key); 
    }

    poke_EXP(1, (Object)idx);
    Bucket_compare_key(key, bucket->values[idx]);
}

void Bucket_store_(Type_Array * bucketp, Object key, Object value)
{
    /* just store at the first empty location */
    Type_Array bucket = *bucketp;

    if (bucket->values[0] == Nil) {
        bucket->values[0] = key;
        bucket->values[1] = value;
        push_EXP((Object)0);
        return;
    }

    claim_EXP(4);
    poke_EXP(3, value);
    poke_EXP(2, key);
    poke_EXP(1, 0);
    poke_EXP(0, bucketp);
    push_CNT(Bucket_store);

    Bucket_compare_key(key, bucket->values[0]);
}

static void Bucket_lookup(Type_Array bucket, Object key)
{
    if (bucket->values[0] == (Object)Nil) {
        poke_EXP(0, NULL);
        return;
    }

    claim_EXP(3);
    poke_EXP(2, key);
    poke_EXP(1, 0);
    poke_EXP(0, bucket);
    push_CNT(bucket_lookup);
    Bucket_compare_key(key, bucket->values[0]);
}

static void CNT_bucket_rehash();
static void CNT_bucket_rehash_end()
{
    uns_int idx       = (uns_int)peek_EXP(0);
    Type_Array bucket = (Type_Array)peek_EXP(1);
    Object key        = bucket->values[idx];
    idx += 2;

    if (idx >= bucket->size || (key = bucket->values[idx]) == (Object)Nil) {
        zap_CNT();
        zapn_EXP(2);
        return;
    }

    poke_CNT(bucket_rehash);
    poke_EXP(0, idx);
    push_hash(key);
}

static void CNT_bucket_rehash()
{
    Object w_hash        = pop_EXP();
    Collection_Dictionary dict = (Collection_Dictionary)peek_EXP(2);
    int hash             = unwrap_hash(dict, w_hash);

    uns_int idx          = (uns_int)peek_EXP(0);
    Type_Array bucket    = (Type_Array)peek_EXP(1);
    Object key           = bucket->values[idx];

    poke_CNT(bucket_rehash_end);
    Collection_Dictionary_direct_store(dict, hash, key, bucket->values[idx + 1]);
}

/* ========================================================================= */

CNT(lookup_push)
    Object w_hash        = peek_EXP(0);
    Collection_Dictionary self = (Collection_Dictionary)peek_EXP(2);
    int hash             = unwrap_hash(self, w_hash);
    Object key           = peek_EXP(1);
    zapn_EXP(2);

    Type_Array * bucketp = get_bucketp(self, hash);
    if (*bucketp == (Type_Array)Nil) {
        poke_EXP(0, NULL);
        return;
    }
    Bucket_lookup(*bucketp, key);
}

void Collection_Dictionary_lookup_push(Collection_Dictionary self, Object key)
{
    push_CNT(lookup_push);
    claim_EXP(2);
    poke_EXP(1, self);
    poke_EXP(0, key);
    push_hash(key);
}

static CNT(dict_grow_end)
    zapn_EXP(3);
}

static void CNT_dict_grow()
{
    uns_int idx    = (uns_int)peek_EXP(1);
    Type_Array old = (Type_Array)peek_EXP(2);
    Object bucket  = old->values[idx];
    if (idx == 0) {
        poke_CNT(dict_grow_end);
    } else {
        poke_EXP(1, idx - 1);
    }
    if (bucket == (Object)Nil || ((Type_Array)bucket)->size == 0) {
        return;
    }
    Object key = ((Type_Array)bucket)->values[0];
    if (key == (Object)Nil) { return; }

    push_CNT(bucket_rehash);
    claim_EXP(2);
    poke_EXP(1, bucket);
    poke_EXP(0, 0);

    push_hash(key);
}

static void Collection_Dictionary_grow(Collection_Dictionary self)
{
    Type_Array old = self->data;
    self->data     = new_Type_Array_withAll(old->size << 1, (Object)Nil);
    self->size     = new_Type_SmallInt(0);
    
    push_CNT(dict_grow);
    claim_EXP(3);
    poke_EXP(2, old);
    poke_EXP(1, old->size - 1);
    poke_EXP(0, self);
}

static void Collection_Dictionary_check_grow(Collection_Dictionary self)
{
    if (Collection_Dictionary_grow_check(self)) {
        Collection_Dictionary_grow(self);
    }
}

static CNT(Collection_Dictionary_check_grow)
    // Check if new element
    Object test = pop_EXP();
    if (test) {
        Collection_Dictionary self = (Collection_Dictionary)pop_EXP();
        Collection_Dictionary_check_grow(self);
    } else {
        zap_EXP();
    }
}

void Collection_Dictionary_direct_store(Collection_Dictionary self, int hash,
                                  Object key, Object value) 
{
    Type_Array * bucketp = get_bucketp(self, hash);
    if (*bucketp == (Type_Array)Nil) { 
        *bucketp = new_bucket();
        Type_Array bucket = *bucketp;
        bucket->values[0] = key;
        bucket->values[1] = value;
        Collection_Dictionary_check_grow(self);
    } else {
        push_EXP((Object)self);
        push_CNT(Collection_Dictionary_check_grow);
        Bucket_store_(bucketp, key, value);
    }
}

/* ========================================================================= */

CNT(fix_dictionary_result)
    if (peek_EXP(0) == NULL) {
        poke_EXP(0, Nil);
    }
}

CNT(dictionary_check_absent)
    Object result = pop_EXP();
    if (result == NULL) {
        Object block = pop_EXP();
        Type_Class_direct_dispatch(block, HEADER(block), (Object)SMB_value, 0); 
        return;
    }
    poke_EXP(0, result);
}

NATIVE1(Collection_Dictionary_at_)
    Object w_index = NATIVE_ARG(0);
    zapn_EXP(3);
    push_CNT(fix_dictionary_result);
    Collection_Dictionary_lookup_push((Collection_Dictionary)self, w_index);
}

NATIVE2(Collection_Dictionary_at_ifAbsent_)
    Object w_index = NATIVE_ARG(0);
    Object w_block = NATIVE_ARG(1);
    zapn_EXP(4);
    push_EXP(w_block);
    push_CNT(dictionary_check_absent);
    Collection_Dictionary_lookup_push((Collection_Dictionary)self, w_index);
}

CNT(Collection_Dictionary_at_put_)
    Object w_hash        = peek_EXP(0);
    Collection_Dictionary self = (Collection_Dictionary)peek_EXP(3);
    int hash             = unwrap_hash(self, w_hash);
    Object new           = peek_EXP(1);
    Object w_index       = peek_EXP(2);
    zapn_EXP(4);
    poke_EXP(0, new);
    Collection_Dictionary_direct_store((Collection_Dictionary)self, hash, w_index, new);
}

NATIVE2(Collection_Dictionary_at_put_)
    push_CNT(Collection_Dictionary_at_put_);
    push_hash(NATIVE_ARG(0));
}

NATIVE(Collection_Dictionary_basicNew)
    zap_EXP();
    poke_EXP(0, new_Collection_Dictionary());
}

void post_init_Collection_Dictionary()
{
    Collection_Dictionary_Class->name = new_Type_Symbol_cached(L"Dictionary");
    store_native_method(Collection_Dictionary_Class, SMB_at_,              NM_Collection_Dictionary_at_);
    store_native_method(Collection_Dictionary_Class, SMB_at_ifAbsent_,     NM_Collection_Dictionary_at_ifAbsent_);
    store_native_method(Collection_Dictionary_Class, SMB_at_put_,          NM_Collection_Dictionary_at_put_);
    store_native_method(HEADER(Collection_Dictionary_Class), SMB_basicNew, NM_Collection_Dictionary_basicNew);
}