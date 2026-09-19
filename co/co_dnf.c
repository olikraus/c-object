#include "co.h"
#include <stdlib.h>
#include <assert.h>
#include <string.h>

static int compareANDTerms(const void *a, const void *b) {
  cco t1 = *(cco *)a;
  cco t2 = *(cco *)b;

  /* 1. Sort by map size (number of attributes) */
  long s1 = coMapSize(t1);
  long s2 = coMapSize(t2);
  if (s1 != s2)
    return (int)(s1 - s2);

  /* 2. Compare keys and values */
  coMapIterator it1, it2;
  int res1 = coMapLoopFirst(&it1, t1);
  int res2 = coMapLoopFirst(&it2, t2);

  while (res1 && res2) {
    const char *k1 = coMapLoopKey(&it1);
    const char *k2 = coMapLoopKey(&it2);
    int cmp = strcmp(k1, k2);
    if (cmp != 0)
      return cmp;
    
    /* Keys match, compare values (Int32Vectors) */
    cco v1 = coMapLoopValue(&it1);
    cco v2 = coMapLoopValue(&it2);
    long vs1 = coInt32VectorSize(v1);
    long vs2 = coInt32VectorSize(v2);
    if (vs1 != vs2)
      return (int)(vs1 - vs2);
    
    /* Sizes match, compare elements */
    long i;
    for (i = 0; i < vs1; i++) {
      int32_t val1 = coInt32VectorGet(v1, i);
      int32_t val2 = coInt32VectorGet(v2, i);
      if (val1 != val2)
        return (int)(val1 - val2);
    }

    res1 = coMapLoopNext(&it1);
    res2 = coMapLoopNext(&it2);
  }

  return 0;
}

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

int coDNFIsEmpty(cco dnf) {
  if (dnf == NULL)
    return 1;
  assert(coIsVector(dnf));
  return coVectorSize(dnf) == 0;
}

int coDNFIsUniversal(cco dnf) {
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

int coInt32VectorEquals(cco v1, cco v2) {
  if (v1 == v2) return 1;
  if (v1 == NULL || v2 == NULL) return 0;
  if (coInt32VectorSize(v1) != coInt32VectorSize(v2)) return 0;
  long i;
  for (i = 0; i < coInt32VectorSize(v1); i++) {
    if (!coInt32VectorExists((co)v2, coInt32VectorGet(v1, i))) return 0;
  }
  return 1;
}

void coDNFMinimizeClearFullDomainANDTerm(cco psd, co term) {
  assert(coIsMap(term));
  if (psd == NULL) return;
  cco psd_inner = coMapGet(psd, "psd");
  if (psd_inner == NULL || !coIsMap(psd_inner)) return;
  
  coMapIterator iter;
  if (coMapLoopFirst(&iter, term)) {
    do {
      const char *key = coMapLoopKey(&iter);
      cco val = coMapLoopValue(&iter);
      cco domain = coMapGet(psd_inner, key);
      if (domain != NULL && coInt32VectorEquals(val, domain)) {
        coMapErase(term, key);
        /* Restart iteration after erase to be safe */
        if (!coMapLoopFirst(&iter, term)) break;
      }
    } while (coMapLoopNext(&iter));
  }
}

void coDNFMinimizeClearFullDomain(cco psd, co dnf) {
  assert(coIsVector(dnf));
  long i;
  for (i = 0; i < coVectorSize(dnf); i++) {
    coDNFMinimizeClearFullDomainANDTerm(psd, (co)coVectorGet(dnf, i));
  }
}

int coDNFUnion(cco psd, co arg1, cco arg2) {
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
  coDNFMinimizeClearFullDomain(psd, arg1);
  coDNFMinimizeANDTermSubset(arg1);
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

co coNewDNFByIntersectionWithoutMinimization(cco psd, cco arg1, cco arg2) {
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

co coNewDNFByIntersection(cco psd, cco arg1, cco arg2) {
  assert(coIsVector(arg1));
  assert(coIsVector(arg2));

  co result = coNewVector(CO_FREE_VALS);
  if (result == NULL)
    return NULL;

  /* Parallel vector to track the volume of each term in 'result' */
  co volumes = coNewInt32Vector(CO_NONE);
  if (volumes == NULL) {
    coDelete(result);
    return NULL;
  }

  long i, j;
  long cnt1 = coVectorSize(arg1);
  long cnt2 = coVectorSize(arg2);
  int32_t max_vol = 0;
  int32_t min_vol = 2147483647;

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
        /* Online Subset Minimization with Dynamic Range Pruning: 
           Keep 'result' minimal during construction.
        */
        int32_t new_vol = coDNFGetVolumeANDTerm(psd, intersection);
        if (new_vol > max_vol) max_vol = new_vol;
        if (new_vol < min_vol) min_vol = new_vol;
        
        int32_t threshold = (max_vol - min_vol) / 8;
        int skip = 0;
        long k;
        long res_cnt = coVectorSize(result);
        
        /* Heuristic: Only perform expensive subset checks between extremes. */
        if (new_vol <= min_vol + threshold) {
          /* 1. Zone A: New term is small, check if it is a subset of any LARGE existing term */
          for (k = 0; k < res_cnt; k++) {
            cco existing = coVectorGet(result, k);
            if (existing == NULL) continue;
            int32_t existing_vol = coInt32VectorGet(volumes, k);
            if (existing_vol >= max_vol || existing_vol==new_vol) {
              if (coDNFIsSubsetANDTermANDTerm(intersection, existing)) {
                skip = 1;
                break;
              }
            }
          }
        } else if (new_vol >= max_vol - threshold) {
          /* 2. Zone B: New term is large, check if it is a superset of any SMALL existing terms */
          for (k = 0; k < res_cnt; k++) {
            cco existing = coVectorGet(result, k);
            if (existing == NULL) continue;
            int32_t existing_vol = coInt32VectorGet(volumes, k);            
            if (existing_vol <= min_vol || existing_vol==new_vol) {
              if (coDNFIsSubsetANDTermANDTerm(existing, intersection)) {
                coDelete((co)existing);
                result->v.list[k] = NULL;
              }
            }
          }
        }

        if (!skip) {
          coVectorAdd(result, intersection);
          coInt32VectorAdd(volumes, new_vol);
        } else {
          coDelete(intersection);
        }
      }
    }
  }

  /* Compaction Pass: Remove the NULL markers from both result and volumes */
  long write_idx = 0;
  long read_idx;
  long total = coVectorSize(result);
  for (read_idx = 0; read_idx < total; read_idx++) {
    if (result->v.list[read_idx] != NULL) {
      result->v.list[write_idx] = result->v.list[read_idx];
      coInt32VectorSet(volumes, write_idx, coInt32VectorGet(volumes, read_idx));
      write_idx++;
    }
  }
  result->v.cnt = write_idx;

  coDelete(volumes);
  return result;
}

int coDNFIntersection(cco psd, co arg1, cco arg2) {
  assert(coIsVector(arg1));
  assert(coIsVector(arg2));

  co res = coNewDNFByIntersection(psd, arg1, arg2);
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


int coDNFIsSubsetAttributeSelection(cco a, cco b) {
  if (a == NULL || b == NULL) return 0;
  assert(coIsInt32Vector(a));
  assert(coIsInt32Vector(b));
  long i;
  for (i = 0; i < coInt32VectorSize(a); i++) {
    if (!coInt32VectorExists((co)b, coInt32VectorGet(a, i))) return 0;
  }
  return 1;
}

int coDNFIsSubsetANDTermANDTerm(cco subset_and_term, cco superset_and_term) {
  assert(coIsMap(subset_and_term));
  assert(coIsMap(superset_and_term));
  coMapIterator iter_b;
  if (coMapLoopFirst(&iter_b, superset_and_term)) {
    do {
      const char *key = coMapLoopKey(&iter_b);
      cco val_b = coMapLoopValue(&iter_b);
      cco val_a = coMapGet(subset_and_term, key);
      if (val_a == NULL) return 0;
      if (!coDNFIsSubsetAttributeSelection(val_a, val_b)) return 0;
    } while (coMapLoopNext(&iter_b));
  }
  return 1;
}

static co coNewDNFBySubtractANDTermANDTerm(cco psd, cco left, cco right) {
  assert(coIsMap(left));
  assert(coIsMap(right));

  co result = coNewVector(CO_FREE_VALS);
  co effectiveLeft = coClone(left);
  cco psd_inner = (psd == NULL) ? NULL : coMapGet(psd, "psd");

  /* restrictedNames: sorted attribute names present in right */
  coMapIterator iter_b;
  if (coMapLoopFirst(&iter_b, right)) {
    do {
      const char *attrName = coMapLoopKey(&iter_b);
      cco rightValues = coMapLoopValue(&iter_b);
      cco leftValues = coMapGet(effectiveLeft, attrName);

      co leftValues_cloned;
      if (leftValues == NULL) {
        cco domain = (psd_inner == NULL) ? NULL : coMapGet(psd_inner, attrName);
        if (domain == NULL) {
          leftValues_cloned = coClone(rightValues);
        } else {
          leftValues_cloned = coClone(domain);
        }
      } else {
        leftValues_cloned = coClone(leftValues);
      }

      /* matchingValues := Intersection(leftValues, rightValues) */
      co matchingValues = coNewInt32Vector(CO_NONE);
      long i;
      for (i = 0; i < coInt32VectorSize(leftValues_cloned); i++) {
        int32_t v = coInt32VectorGet(leftValues_cloned, i);
        if (coInt32VectorExists((co)rightValues, v)) {
          coInt32VectorAdd(matchingValues, v);
        }
      }

      /* remainingValues := Difference(leftValues, rightValues) */
      co remainingValues = coNewInt32Vector(CO_NONE);
      for (i = 0; i < coInt32VectorSize(leftValues_cloned); i++) {
        int32_t v = coInt32VectorGet(leftValues_cloned, i);
        if (!coInt32VectorExists((co)rightValues, v)) {
          coInt32VectorAdd(remainingValues, v);
        }
      }
      coDelete(leftValues_cloned);

      if (coInt32VectorEmpty(matchingValues)) {
        /* Disjoint case: return accumulated result + elided effectiveLeft */
        coDelete(matchingValues);
        coDelete(remainingValues);
        coDNFMinimizeClearFullDomainANDTerm(psd, effectiveLeft);
        coVectorAdd(result, effectiveLeft);
        return result;
      }

      if (!coInt32VectorEmpty(remainingValues)) {
        co escapedTerm = coClone(effectiveLeft);
        coMapAdd(escapedTerm, attrName, remainingValues);
        coDNFMinimizeClearFullDomainANDTerm(psd, escapedTerm);
        coVectorAdd(result, escapedTerm);
      } else {
        coDelete(remainingValues);
      }

      /* effectiveLeft[attributeName] := matchingValues */
      coMapAdd(effectiveLeft, attrName, matchingValues);

    } while (coMapLoopNext(&iter_b));
  }

  coDelete(effectiveLeft);
  return result;
}

void coDNFMinimizeANDTermSubset(co dnf) {
  assert(coIsVector(dnf));
  long i = 0;
  while (i < coVectorSize(dnf)) {
    cco term_i = coVectorGet(dnf, i);
    int removed = 0;
    long j;
    for (j = 0; j < coVectorSize(dnf); j++) {
      if (i == j) continue;
      cco term_j = coVectorGet(dnf, j);
      if (coDNFIsSubsetANDTermANDTerm(term_i, term_j)) {
        coVectorErase(dnf, i);
        removed = 1;
        break;
      }
    }
    if (!removed) {
      i++;
    }
  }
}

void coDNFMinimizeByANDTermMerge(co dnf) {
  assert(coIsVector(dnf));

  int changed = 1;
  while (changed) {
    changed = 0;

    /* Merge terms differing in exactly one attribute */
    long i, j;
    for (i = 0; i < coVectorSize(dnf); i++) {
      for (j = i + 1; j < coVectorSize(dnf); j++) {
        cco t1 = coVectorGet(dnf, i);
        cco t2 = coVectorGet(dnf, j);

        if (coMapSize(t1) != coMapSize(t2))
          continue;

        const char *diff_key = NULL;
        int diff_count = 0;
        int mismatch = 0;

        coMapIterator it;
        if (coMapLoopFirst(&it, t1)) {
          do {
            const char *key = coMapLoopKey(&it);
            cco v1 = coMapLoopValue(&it);
            cco v2 = coMapGet(t2, key);
            if (v2 == NULL) {
              mismatch = 1;
              break;
            }
            if (!coInt32VectorEquals(v1, v2)) {
              diff_count++;
              diff_key = key;
            }
          } while (coMapLoopNext(&it));
        }

        if (mismatch)
          continue;

        if (diff_count == 1) {
          /* Merge t2 into t1 at diff_key */
          co v1 = (co)coMapGet(t1, diff_key);
          cco v2 = coMapGet(t2, diff_key);
          coInt32VectorAppendVector(v1, v2);
          coVectorErase(dnf, j);
          changed = 1;
          break;
        } else if (diff_count == 0) {
          /* Identical terms */
          coVectorErase(dnf, j);
          changed = 1;
          break;
        }
      }
      if (changed)
        break;
    }
  }
}

co coNewDNFBySubtraction(cco psd, cco left_dnf, cco right_dnf) {
  assert(coIsVector(left_dnf));
  assert(coIsVector(right_dnf));

  co result = coClone(left_dnf);
  coDNFMinimizeClearFullDomain(psd, result);
  coDNFMinimizeANDTermSubset(result);

  long j;
  for (j = 0; j < coVectorSize(right_dnf); j++) {
    cco rightTerm = coVectorGet(right_dnf, j);
    co nextResult = coNewVector(CO_FREE_VALS);
    
    long k;
    for (k = 0; k < coVectorSize(result); k++) {
      cco leftTerm = coVectorGet(result, k);
      co diff = coNewDNFBySubtractANDTermANDTerm(psd, leftTerm, rightTerm);
      coVectorAppendVector(nextResult, diff);
      coDelete(diff);
    }
    
    coDelete(result);
    result = nextResult;
    coDNFMinimizeClearFullDomain(psd, result);
    coDNFMinimizeANDTermSubset(result);

    if (coDNFIsEmpty(result)) break;
  }
  return result;
}

co coDNFComplementBySubtract(cco psd, cco dnf) {
  assert(coIsVector(dnf));
  co universal = coNewVector(CO_FREE_VALS);
  coVectorAdd(universal, coNewMap(CO_STRDUP | CO_FREE_VALS));
  co res = coNewDNFBySubtraction(psd, universal, dnf);
  coDelete(universal);
  return res;
}

co coDNFNewCofactor(cco psd, cco dnf, const char *attr_name, int32_t value) {
  assert(coIsVector(dnf));
  co result = coNewVector(CO_FREE_VALS);

  long i;
  for (i = 0; i < coVectorSize(dnf); i++) {
    cco term = coVectorGet(dnf, i);
    assert(coIsMap(term));
    cco values = coMapGet(term, attr_name);

    if (values == NULL) {
      /* Identity Case: The term remains unchanged */
      coVectorAdd(result, coClone(term));
    } else {
      assert(coIsInt32Vector(values));
      if (coInt32VectorExists((co)values, value)) {
        /* Fulfillment Case: Remove the attribute */
        co new_term = coClone(term);
        coMapErase(new_term, attr_name);
        coVectorAdd(result, new_term);
      } else {
        /* Conflict Case: Term becomes False, do nothing */
      }
    }
  }

  coDNFMinimizeClearFullDomain(psd, result);
  coDNFMinimizeANDTermSubset(result);
  return result;
}

const char *coDNFGetBestCofactorAttribute(cco psd, cco dnf) {
  assert(coIsVector(dnf));
  if (coDNFIsEmpty(dnf)) return NULL;

  co counts = coNewMap(CO_STRDUP | CO_FREE_VALS);
  const char *best_attr_name = NULL;
  double max_val = -1.0;
  
  long i;
  for (i = 0; i < coVectorSize(dnf); i++) {
    cco term = coVectorGet(dnf, i);
    assert(coIsMap(term));
    coMapIterator it;
    if (coMapLoopFirst(&it, term)) {
      do {
        const char *key = coMapLoopKey(&it);
        co d = (co)coMapGet(counts, key);
        double current_val;
        if (d == NULL) {
          current_val = 1.0;
          coMapAdd(counts, key, coNewDbl(current_val));
        } else {
          current_val = coDblGet(d) + 1.0;
          coDblSet(d, current_val);
        }
        
        if (current_val > max_val) {
          max_val = current_val;
          best_attr_name = key;
        }
      } while (coMapLoopNext(&it));
    }
  }

  const char *stable_ptr = NULL;
  if (best_attr_name != NULL) {
    cco psd_inner = (psd == NULL) ? NULL : coMapGet(psd, "psd");
    assert(psd_inner != NULL);
    stable_ptr = coMapGetKey(psd_inner, best_attr_name);
    assert(stable_ptr != NULL); /* Attribute MUST exist in PSD */
  }

  coDelete(counts);
  return stable_ptr;
}

int coDNFCheckUniversal(cco psd, cco dnf) {
  assert(coIsVector(dnf));
  
  if (coDNFIsEmpty(dnf)) return 0;
  if (coDNFIsUniversal(dnf)) return 1;

  /* Early Abort Check: Domain Coverage Rule 
     If any attribute A's union of values does not cover D_A, then NOT universal.
  */
  co counts = coNewMap(CO_STRDUP | CO_FREE_VALS);
  long i;
  for (i = 0; i < coVectorSize(dnf); i++) {
    cco term = coVectorGet(dnf, i);
    coMapIterator it;
    if (coMapLoopFirst(&it, term)) {
      do {
        const char *key = coMapLoopKey(&it);
        cco vals = coMapLoopValue(&it);
        co union_vec = (co)coMapGet(counts, key);
        if (union_vec == NULL) {
          coMapAdd(counts, key, coClone(vals));
        } else {
          coInt32VectorAppendVector(union_vec, vals);
        }
      } while (coMapLoopNext(&it));
    }
  }

  int abort = 0;
  cco psd_inner = (psd == NULL) ? NULL : coMapGet(psd, "psd");
  const char *best_attr_name = NULL;
  long max_count = -1;

  coMapIterator it;
  if (coMapLoopFirst(&it, counts)) {
    do {
      const char *key = coMapLoopKey(&it);
      cco union_vec = coMapLoopValue(&it);
      cco domain = (psd_inner == NULL) ? NULL : coMapGet(psd_inner, key);
      
      /* 1. Coverage Check */
      if (domain != NULL && !coInt32VectorEquals(union_vec, domain)) {
        abort = 1;
        break;
      }

      /* 2. Heuristic for Shannon expansion */
      long count = 0;
      for (i = 0; i < coVectorSize(dnf); i++) {
        if (coMapExists(coVectorGet(dnf, i), key)) count++;
      }
      if (count > max_count) {
        max_count = count;
        best_attr_name = key;
      }
    } while (coMapLoopNext(&it));
  }
  coDelete(counts);

  if (abort) return 0;
  if (best_attr_name == NULL) return 0;

  /* Stabilize attr name pointer from PSD */
  const char *stable_attr = coMapGetKey(psd_inner, best_attr_name);
  cco domain = coMapGet(psd_inner, stable_attr);

  for (i = 0; i < coInt32VectorSize(domain); i++) {
    int32_t val = coInt32VectorGet(domain, i);
    co cofactor = coDNFNewCofactor(psd, dnf, stable_attr, val);
    int res = coDNFCheckUniversal(psd, cofactor);
    coDelete(cofactor);
    if (res == 0) return 0;
  }

  return 1;
}

int coDNFCheckUniversalByComplement(cco psd, cco dnf) {
  assert(coIsVector(dnf));
  co complement = coDNFComplementBySubtract(psd, dnf);
  int res = coDNFIsEmpty(complement);
  coDelete(complement);
  return res;
}

int coDNFComplement(cco psd, co dnf) {
  assert(coIsVector(dnf));
  co res = coDNFComplementBySubtract(psd, dnf);
  if (res == NULL)
    return 0;

  coVectorClear(dnf);
  long cnt = coVectorSize(res);
  long i;
  for (i = 0; i < cnt; i++) {
    co element = (co)coVectorGet(res, i);
    if (coVectorAdd(dnf, element) < 0) {
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

int coDNFIsSubsetANDTerm(cco psd, cco subset_and_term, cco superset_dnf) {
  assert(coIsMap(subset_and_term));
  assert(coIsVector(superset_dnf));
  
  if (coDNFIsUniversal(superset_dnf)) return 1;
  if (coDNFIsEmpty(superset_dnf)) return 0;

  co subset_dnf = coNewVector(CO_FREE_VALS);
  coVectorAdd(subset_dnf, coClone(subset_and_term));
  co remainder = coNewDNFBySubtraction(psd, subset_dnf, superset_dnf);
  int res = coDNFIsEmpty(remainder);
  coDelete(remainder);
  coDelete(subset_dnf);
  return res;
}

int coDNFIsSubset(cco psd, cco subset_dnf, cco superset_dnf) {
  assert(coIsVector(subset_dnf));
  assert(coIsVector(superset_dnf));
  
  if (coDNFIsEmpty(subset_dnf)) return 1;
  
  long i;
  for (i = 0; i < coVectorSize(subset_dnf); i++) {
    cco a = coVectorGet(subset_dnf, i);
    if (!coDNFIsSubsetANDTerm(psd, a, superset_dnf)) return 0;
  }
  return 1;
}

int coDNFIsEqual(cco psd, cco dnf1, cco dnf2) {
  assert(coIsVector(dnf1));
  assert(coIsVector(dnf2));
  return coDNFIsSubset(psd, dnf1, dnf2) && coDNFIsSubset(psd, dnf2, dnf1);
}

int32_t coDNFGetVolumeANDTerm(cco psd, cco term) {
  assert(coIsMap(term));
  if (psd == NULL)
    return 1;
  cco psd_inner = coMapGet(psd, "psd");
  if (psd_inner == NULL || !coIsMap(psd_inner))
    return 1;

  int32_t volume = 1;
  coMapIterator it;
  if (coMapLoopFirst(&it, psd_inner)) {
    do {
      const char *key = coMapLoopKey(&it);
      cco domain = coMapLoopValue(&it);
      cco term_vals = coMapGet(term, key);
      if (term_vals == NULL) {
        volume *= (int32_t)coInt32VectorSize(domain);
      } else {
        volume *= (int32_t)coInt32VectorSize(term_vals);
      }
    } while (coMapLoopNext(&it));
  }
  return volume;
}

static int32_t coDNFGetVolumeRecursive(cco psd, cco psd_inner, coMapIterator *attr_iter, cco dnf) {
  if (coDNFIsEmpty(dnf)) return 0;
  if (coDNFIsUniversal(dnf)) {
    /* If universal, the volume is the product of all remaining attribute domains */
    int32_t remaining_vol = 1;
    coMapIterator it = *attr_iter;
    /* Note: the current attribute was already advanced in the caller, or we are at the end */
    while (it.current_node != NULL) {
      remaining_vol *= (int32_t)coInt32VectorSize(coMapLoopValue(&it));
      if (!coMapLoopNext(&it)) break;
    }
    return remaining_vol;
  }

  /* Get current attribute and advance iterator for next level */
  const char *attr = coMapLoopKey(attr_iter);
  cco domain = coMapLoopValue(attr_iter);
  
  coMapIterator next_iter = *attr_iter;
  int has_next = coMapLoopNext(&next_iter);

  long i;
  int32_t total_volume = 0;
  long domain_size = coInt32VectorSize(domain);
  for (i = 0; i < domain_size; i++) {
    int32_t val = coInt32VectorGet(domain, i);
    co cofactor = coDNFNewCofactor(psd, dnf, attr, val);
    
    if (has_next) {
      total_volume += coDNFGetVolumeRecursive(psd, psd_inner, &next_iter, cofactor);
    } else {
      /* Base case: no more attributes, check if cofactor is universal */
      if (coDNFIsUniversal(cofactor)) total_volume += 1;
    }
    coDelete(cofactor);
  }
  return total_volume;
}

int32_t coDNFGetVolume(cco psd, cco dnf) {
  assert(coIsVector(dnf));
  if (psd == NULL) return 0;
  cco psd_inner = coMapGet(psd, "psd");
  if (psd_inner == NULL || coMapSize(psd_inner) == 0) return 0;
  
  if (coDNFIsEmpty(dnf)) return 0;

  coMapIterator it;
  if (!coMapLoopFirst(&it, psd_inner)) return 0;

  return coDNFGetVolumeRecursive(psd, psd_inner, &it, dnf);
}
