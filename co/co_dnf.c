#include "co.h"
#include <stdlib.h>
#include <assert.h>

co coConvertToInt32Vector(co o) {
  if (o == NULL)
    return NULL;

  if (coIsMap(o)) {
    coMapIterator iter;
    if (coMapLoopFirst(&iter, o)) {
      do {
        co val_old = (co)(iter.current_node->value);
        co val_new = coConvertToInt32Vector(val_old);
        iter.current_node->value = val_new;
      } while (coMapLoopNext(&iter));
    }
    return o;
  }

  if (coIsVector(o)) {
    long i;
    long cnt = coVectorSize(o);
    
    /* 1. Recursively convert all children first */
    for (i = 0; i < cnt; i++) {
      co elem_old = (co)coVectorGet(o, i);
      co elem_new = coConvertToInt32Vector(elem_old);
      o->v.list[i] = elem_new;
    }

    /* 2. Check if this vector should be replaced by coInt32Vector */
    if (cnt > 0) {
      cco first = coVectorGet(o, 0);
      if (first != NULL && (coIsDbl(first) || coIsBool(first))) {
        co iv = coNewInt32Vector(CO_NONE);
        if (iv == NULL)
          return o; /* Allocation failure, keep original */
        
        for (i = 0; i < cnt; i++) {
          cco elem = coVectorGet(o, i);
          if (elem != NULL) {
            if (coIsDbl(elem)) {
              coInt32VectorAddUnique(iv, (int32_t)coDblGet(elem));
            } else if (coIsBool(elem)) {
              coInt32VectorAddUnique(iv, (int32_t)coBoolGet(elem));
            }
          }
        }
        
        /* Fully erase/delete the original vector o */
        coDelete(o);
        return iv;
      }
    }
  }

  return o;
}

int coDNFIsValid(co dnf) {
  if (dnf == NULL)
    return 0;
  if (!coIsVector(dnf))
    return 0;

  long i;
  long cnt = coVectorSize(dnf);
  for (i = 0; i < cnt; i++) {
    cco map_item = coVectorGet(dnf, i);
    if (map_item == NULL || !coIsMap(map_item))
      return 0;

    coMapIterator iter;
    if (coMapLoopFirst(&iter, map_item)) {
      do {
        cco val = coMapLoopValue(&iter);
        if (val == NULL || !coIsInt32Vector(val))
          return 0;
      } while (coMapLoopNext(&iter));
    }
  }
  return 1;
}

int coDNFIsEmpty(co dnf) {
  if (dnf == NULL)
    return 1;
  assert(coIsVector(dnf));
  return coVectorSize(dnf) == 0;
}

int coDNFIsUniversal(co dnf) {
  if (dnf == NULL)
    return 0;
  assert(coIsVector(dnf));
  if (coVectorSize(dnf) != 1)
    return 0;
  cco first = coVectorGet(dnf, 0);
  if (first == NULL || !coIsMap(first))
    return 0;
  return coMapSize(first) == 0;
}

int coDNFUnion(co arg1, cco arg2) {
  assert(coIsVector(arg1));
  assert(coIsVector(arg2));
  
  long i;
  long cnt = coVectorSize(arg2);
  long old_cnt = coVectorSize(arg1);
  for (i = 0; i < cnt; i++) {
    cco map_item = coVectorGet(arg2, i);
    if (map_item != NULL) {
      co map_clone = coClone(map_item);
      if (map_clone == NULL) {
        /* Roll back */
        while (coVectorSize(arg1) > old_cnt) {
          coVectorEraseLast(arg1);
        }
        return 0;
      }
      if (coVectorAdd(arg1, map_clone) < 0) {
        coDelete(map_clone);
        /* Roll back */
        while (coVectorSize(arg1) > old_cnt) {
          coVectorEraseLast(arg1);
        }
        return 0;
      }
    }
  }
  return 1;
}

co coNewPSD(void) {
  co wrapper = coNewMap(CO_STRDUP | CO_STRFREE | CO_FREE_VALS);
  if (wrapper == NULL)
    return NULL;
  co inner = coNewMap(CO_STRDUP | CO_STRFREE | CO_FREE_VALS);
  if (inner == NULL) {
    coDelete(wrapper);
    return NULL;
  }
  if (coMapAdd(wrapper, "psd", inner) == NULL) {
    coDelete(inner);
    coDelete(wrapper);
    return NULL;
  }
  return wrapper;
}

int coPSDExtendByDNF(co psd, cco dnf) {
  if (psd == NULL || dnf == NULL)
    return 0;
  
  assert(coIsMap(psd));
  assert(coIsVector(dnf));

  co psd_inner = (co)coMapGet(psd, "psd");
  if (psd_inner == NULL || !coIsMap(psd_inner))
    return 0;

  /* Force CO_STRDUP and CO_STRFREE to prevent key pointer sharing and double-frees */
  psd_inner->flags |= CO_STRDUP | CO_STRFREE;

  long i;
  long cnt = coVectorSize(dnf);
  for (i = 0; i < cnt; i++) {
    cco and_term = coVectorGet(dnf, i);
    if (and_term == NULL || !coIsMap(and_term))
      continue;

    coMapIterator iter;
    if (coMapLoopFirst(&iter, and_term)) {
      do {
        const char *attr = coMapLoopKey(&iter);
        cco payload = coMapLoopValue(&iter);
        if (payload == NULL)
          continue;

        /* Get or create the Int32Vector in PSD for this attribute */
        co psd_vec = (co)coMapGet(psd_inner, attr);
        if (psd_vec == NULL) {
          psd_vec = coNewInt32Vector(CO_NONE);
          if (psd_vec == NULL)
            return 0; /* Memory error */
          if (coMapAdd(psd_inner, attr, psd_vec) == NULL) {
            coDelete(psd_vec);
            return 0;
          }
        }
        
        assert(coIsInt32Vector(psd_vec));

        /* Add all values from payload to psd_vec if they don't exist yet */
        if (coIsInt32Vector(payload)) {
          long p_i;
          long p_cnt = coInt32VectorSize(payload);
          for (p_i = 0; p_i < p_cnt; p_i++) {
            int32_t val = coInt32VectorGet(payload, p_i);
            if (coInt32VectorAddUnique(psd_vec, val) < 0)
              return 0;
          }
        } else if (coIsVector(payload)) {
          /* Fallback for un-converted standard Vectors with numeric values */
          long p_i;
          long p_cnt = coVectorSize(payload);
          for (p_i = 0; p_i < p_cnt; p_i++) {
            cco elem = coVectorGet(payload, p_i);
            if (elem != NULL) {
              int32_t val = 0;
              int is_num = 0;
              if (coIsDbl(elem)) {
                val = (int32_t)coDblGet(elem);
                is_num = 1;
              } else if (coIsBool(elem)) {
                val = (int32_t)coBoolGet(elem);
                is_num = 1;
              }
              if (is_num) {
                if (coInt32VectorAddUnique(psd_vec, val) < 0)
                  return 0;
              }
            }
          }
        }
      } while (coMapLoopNext(&iter));
    }
  }
  return 1;
}

static co andTermIntersect(cco a, cco b) {
  assert(coIsMap(a));
  assert(coIsMap(b));

  co c = coNewMap(CO_STRDUP | CO_FREE_VALS);
  if (c == NULL)
    return NULL;

  /* 1. For each key in a */
  coMapIterator iter_a;
  if (coMapLoopFirst(&iter_a, a)) {
    do {
      const char *key = coMapLoopKey(&iter_a);
      cco val_a = coMapLoopValue(&iter_a);
      if (val_a == NULL || !coIsInt32Vector(val_a))
        continue;

      cco val_b = coMapGet(b, key);
      if (val_b != NULL) {
        /* Key is in both */
        assert(coIsInt32Vector(val_b));
        co val_c = coNewInt32Vector(CO_NONE);
        if (val_c == NULL) {
          coDelete(c);
          return NULL;
        }

        long i;
        long cnt_a = coInt32VectorSize(val_a);
        for (i = 0; i < cnt_a; i++) {
          int32_t val = coInt32VectorGet(val_a, i);
          if (coInt32VectorExists((co)val_b, val)) {
            if (coInt32VectorAddUnique(val_c, val) < 0) {
              coDelete(val_c);
              coDelete(c);
              return NULL;
            }
          }
        }

        if (coInt32VectorEmpty(val_c)) {
          /* Intersection on this attribute is empty -> the entire term is empty */
          coDelete(val_c);
          coDelete(c);
          return NULL;
        }

        if (coMapAdd(c, key, val_c) == NULL) {
          coDelete(val_c);
          coDelete(c);
          return NULL;
        }
      } else {
        /* Key is in a but not in b */
        co val_c = coClone(val_a);
        if (val_c == NULL) {
          coDelete(c);
          return NULL;
        }
        if (coMapAdd(c, key, val_c) == NULL) {
          coDelete(val_c);
          coDelete(c);
          return NULL;
        }
      }
    } while (coMapLoopNext(&iter_a));
  }

  /* 2. For each key in b */
  coMapIterator iter_b;
  if (coMapLoopFirst(&iter_b, b)) {
    do {
      const char *key = coMapLoopKey(&iter_b);
      cco val_b = coMapLoopValue(&iter_b);
      if (val_b == NULL || !coIsInt32Vector(val_b))
        continue;

      if (!coMapExists(a, key)) {
        /* Key is in b but not in a */
        co val_c = coClone(val_b);
        if (val_c == NULL) {
          coDelete(c);
          return NULL;
        }
        if (coMapAdd(c, key, val_c) == NULL) {
          coDelete(val_c);
          coDelete(c);
          return NULL;
        }
      }
    } while (coMapLoopNext(&iter_b));
  }

  return c;
}

co coNewDNFByIntersection(cco arg1, cco arg2) {
  assert(coIsVector(arg1));
  assert(coIsVector(arg2));

  co result = coNewVector(CO_FREE_VALS);
  if (result == NULL)
    return NULL;

  long i, j;
  long cnt1 = coVectorSize(arg1);
  long cnt2 = coVectorSize(arg2);

  for (i = 0; i < cnt1; i++) {
    cco a = coVectorGet(arg1, i);
    if (a == NULL || !coIsMap(a))
      continue;

    for (j = 0; j < cnt2; j++) {
      cco b = coVectorGet(arg2, j);
      if (b == NULL || !coIsMap(b))
        continue;

      co intersection = andTermIntersect(a, b);
      if (intersection != NULL) {
        if (coVectorAdd(result, intersection) < 0) {
          coDelete(intersection);
          coDelete(result);
          return NULL;
        }
      }
    }
  }

  return result;
}

int coDNFIntersection(co arg1, cco arg2) {
  assert(coIsVector(arg1));
  assert(coIsVector(arg2));

  co res = coNewDNFByIntersection(arg1, arg2);
  if (res == NULL)
    return 0;

  coVectorClear(arg1);

  long cnt = coVectorSize(res);
  long i;
  for (i = 0; i < cnt; i++) {
    co element = (co)coVectorGet(res, i);
    if (coVectorAdd(arg1, element) < 0) {
      long k;
      for (k = 0; k < i; k++) {
        res->v.list[k] = NULL;
      }
      coDelete(res);
      return 0;
    }
  }

  for (i = 0; i < cnt; i++) {
    res->v.list[i] = NULL;
  }
  coDelete(res);
  return 1;
}
