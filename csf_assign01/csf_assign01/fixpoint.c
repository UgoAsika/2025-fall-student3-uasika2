// Implements MS2 for CSF Fixed-Point assignment.
// References: tests & API in project files.  (citations below code)

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "fixpoint.h"

////////////////////////////////////////////////////////////////////////
// Helper functions
////////////////////////////////////////////////////////////////////////


/**
 * Compare magnitudes of two fixpoint_t numbers.
 * Returns -1 iff a < b, 0 if equal and + 1 if a > b.
 */
static inline int cmp_magnitude(const fixpoint_t *a, const fixpoint_t *b) {
  // Compare magnitudes (ignore sign)
  if (a->whole != b->whole) {
    return (a->whole < b->whole) ? -1 : 1;
  }
  if (a->frac != b->frac) {
    return (a->frac < b->frac) ? -1 : 1;
  }
  return 0;
}

/**
 * Add magnitudes of a and b
 * Stores result in out, sets *overflow if whole part overflowed 32 bits.
 */
static inline void add_magnitude(fixpoint_t *out, const fixpoint_t *a, const fixpoint_t *b, int *overflow) {
  // 32.32 magnitude addition; sets overflw if whole part overflowd 32 bits
  uint32_t af = a->frac, bf = b->frac;
  uint32_t aw = a->whole, bw = b->whole;

  uint32_t rf = af + bf;
  uint32_t carry = (rf < af) ? 1u : 0u;

  uint32_t rw = aw + bw;
  uint32_t carry2 = (rw < aw) ? 1u : 0u;
  rw = rw + carry;
  uint32_t carry3 = (rw < carry) ? 1u : 0u;

  out->frac = rf;
  out->whole = rw;
  if (overflow) *overflow = (carry2 | carry3) ? 1 : 0;
}

/**
 * Substract magnitudes: result = big - small, assuming big >= small
 */

static inline void sub_magnitude(fixpoint_t *out, const fixpoint_t *big, const fixpoint_t *small) {
  // big >= small by magnitude
  uint32_t borrow = 0;
  uint32_t rf, rw;

  if (big->frac < small->frac) {
    rf = (uint32_t)( (uint64_t)big->frac + 0x100000000ULL - small->frac );
    borrow = 1;
  } else {
    rf = big->frac - small->frac;
  }

  rw = big->whole - small->whole - borrow;

  out->whole = rw;
  out->frac = rf;
}

////////////////////////////////////////////////////////////////////////
// Public API functions
////////////////////////////////////////////////////////////////////////

/**
 * initialise a fixpoint t with given whole, frac and sign
 * make sure 0  is always a non negative when stored
 */

void
fixpoint_init( fixpoint_t *val, uint32_t whole, uint32_t frac, bool negative ) {
  val->whole = whole;
  val->frac = frac;
  if (whole == 0 && frac == 0) {
    val->negative = false;
  } else {
    val->negative = negative;
  }
}

/**Return the whole part (unsigned 32-bit). */

uint32_t
fixpoint_get_whole( const fixpoint_t *val ) {
  return val->whole;
}

/** Return the fractional part (unsigned 32-bit). */
uint32_t
fixpoint_get_frac( const fixpoint_t *val ) {
  return val->frac;
}

/** Return true if the number is negative (0 is always treated as non-negative). */

bool
fixpoint_is_negative( const fixpoint_t *val ) {
  if (val->whole == 0 && val->frac == 0) return false;
  return val->negative;
}

/** flip sign, zero always remains non negative */
void
fixpoint_negate( fixpoint_t *val ) {
  if (val->whole == 0 && val->frac == 0) {
    val->negative = false;
  } else {
    val->negative = !val->negative;
  }
}

/**
 * Addition: result = left + right
 * Handles same sign addition with overflow and different sign subtraction
 */
result_t
fixpoint_add( fixpoint_t *result, const fixpoint_t *left, const fixpoint_t *right ) {
  result_t status = RESULT_OK;

  if (left->negative == right->negative) {
    int of = 0;
    add_magnitude(result, left, right, &of);
    result->negative = left->negative; // both same signn
    if (of) status |= RESULT_OVERFLOW;
  } else {
    int cmp = cmp_magnitude(left, right);
    if (cmp == 0) {
      result->whole = 0;
      result->frac = 0;
      result->negative = false; // exact zero
      return RESULT_OK;
    }
    const fixpoint_t *big = (cmp > 0) ? left : right;
    const fixpoint_t *small = (cmp > 0) ? right : left;
    sub_magnitude(result, big, small);
    result->negative = big->negative;
  }

  return status;
}

/**
 * Subtraction: result = left - right, implemented as left + (-right)
 */

result_t
fixpoint_sub( fixpoint_t *result, const fixpoint_t *left, const fixpoint_t *right ) {
  // left - right == left + (-right)
  fixpoint_t neg_right = *right;
  fixpoint_negate(&neg_right);
  return fixpoint_add(result, left, &neg_right);
}

/**
 * Multiplication: result = left * right.
 * Uses 64 by 64 -> 128 multiplication with 32.32 fixed point rules
 * Deteces overflow and underflow
 */

result_t
fixpoint_mul( fixpoint_t *result, const fixpoint_t *left, const fixpoint_t *right ) {
  uint64_t a = left->whole, b = left->frac;
  uint64_t c = right->whole, d = right->frac;

  uint64_t t0 = b * d;        
  uint64_t t1 = a * d;        
  uint64_t t2 = b * c;        
  uint64_t t3 = a * c;        

  uint64_t sum_ad_bc = t1 + t2;

  uint64_t mid = (t3 << 32);
  uint64_t carry_from_mid = 0;

  uint64_t tmp = mid + sum_ad_bc;
  if (tmp < mid) carry_from_mid++;
  mid = tmp;

  uint64_t bd_hi = (t0 >> 32);
  tmp = mid + bd_hi;
  if (tmp < mid) carry_from_mid++;
  mid = tmp;

  uint64_t high64 = t3 + (sum_ad_bc >> 32);


  uint64_t low_add    = (sum_ad_bc << 32);
  uint64_t low64_tmp  = t0 + low_add;
  uint64_t carry_from_low = (low64_tmp < t0) ? (uint64_t)1 : 0;


  high64 += carry_from_low + carry_from_mid;

  result_t status = RESULT_OK;
  if ((high64 >> 32) != 0ULL) status |= RESULT_OVERFLOW;      
  if ((t0 & 0xFFFFFFFFULL) != 0ULL) status |= RESULT_UNDERFLOW; 

  result->whole = (uint32_t)(mid >> 32);
  result->frac  = (uint32_t)(mid & 0xFFFFFFFFu);

  bool neg_xor       = (left->negative ^ right->negative);
  bool res_is_zero   = (result->whole == 0 && result->frac == 0);
  bool left_is_zero  = (left->whole  == 0 && left->frac  == 0);
  bool right_is_zero = (right->whole == 0 && right->frac == 0);

  if (res_is_zero) {
    if (left_is_zero || right_is_zero) {
      result->negative = false;      
    } else {
      result->negative = neg_xor;    
    }
  } else {
    result->negative = neg_xor;       
  }

  return status;
}



/**
 * Comparison: returns -1 if left < right, 0 if equal, +1 if left > right.
 */

int
fixpoint_compare( const fixpoint_t *left, const fixpoint_t *right ) {
  bool ln = fixpoint_is_negative(left);
  bool rn = fixpoint_is_negative(right);

  if (ln != rn) {
    return ln ? -1 : 1; // negative < non-negative
  }

  int m = cmp_magnitude(left, right);
  if (m == 0) return 0;
  return ln ? -m : m; 
}

/**
 * Format fixpoint_t into hex string
 * Whole has no leading zeros (aty leasy 1 digit)
 * Fraction has no trailing zeroes (at least 1 digit)
 */

void
fixpoint_format_hex( fixpoint_str_t *s, const fixpoint_t *val ) {
  // [-]W.F — W no leading zeroees (>=1 digit), F no trailing zeroes (>=1 dgit)
  char buf[FIXPOINT_STR_MAX_SIZE];
  char *p = buf;
  size_t rem = sizeof(buf);

  bool neg = fixpoint_is_negative(val);

  if (neg) {
    *p++ = '-';
    rem--;
  }

  // whole part
  if (val->whole == 0) {
    *p++ = '0';
    rem--;
  } else {
    int n = snprintf(p, rem, "%x", val->whole);
    p += n;
    rem -= (size_t)n;
  }

  // decimal point
  *p++ = '.';
  rem--;

  // fractioal part as 8 hex digits, trm trailing zeroees (keep >=1 digit)
  char fracbuf[9];
  snprintf(fracbuf, sizeof(fracbuf), "%08x", val->frac);

  int end = 7;
  while (end > 0 && fracbuf[end] == '0') end--;
  int len = end + 1;

  for (int i = 0; i < len && rem > 1; i++) {
    *p++ = fracbuf[i];
    rem--;
  }
  *p = '\0';

  strncpy(s->str, buf, FIXPOINT_STR_MAX_SIZE);
  s->str[FIXPOINT_STR_MAX_SIZE - 1] = '\0';
}

/**
 * Parses string of form -w.f into fixpoint t
 * returns true on success, false on invalid format
 */

bool
fixpoint_parse_hex( fixpoint_t *val, const fixpoint_str_t *s ) {
  // [ - ] <1..8 hex> . <1..8 hex>, no extra chars
  const char *str = s->str;
  if (!str || *str == '\0') return false;

  bool neg = false;
  size_t i = 0;

  if (str[i] == '-') {
    neg = true;
    i++;
    if (str[i] == '\0') return false;
  }

  // whole
  uint32_t whole = 0;
  int ndig_whole = 0;
  while (str[i] != '\0' && str[i] != '.') {
    char c = str[i];
    int v;
    if (c >= '0' && c <= '9') v = c - '0';
    else if (c >= 'a' && c <= 'f') v = 10 + (c - 'a');
    else if (c >= 'A' && c <= 'F') v = 10 + (c - 'A');
    else return false;
    if (ndig_whole >= 8) return false;
    whole = (whole << 4) | (uint32_t)v;
    ndig_whole++;
    i++;
  }
  if (ndig_whole == 0) return false;
  if (str[i] != '.') return false;
  i++; // skp '.'
  if (str[i] == '\0') return false;

  // frc
  uint32_t frac_val = 0;
  int ndig_frac = 0;
  while (str[i] != '\0') {
    char c = str[i];
    int v;
    if (c >= '0' && c <= '9') v = c - '0';
    else if (c >= 'a' && c <= 'f') v = 10 + (c - 'a');
    else if (c >= 'A' && c <= 'F') v = 10 + (c - 'A');
    else return false;
    if (ndig_frac >= 8) return false;
    frac_val = (frac_val << 4) | (uint32_t)v;
    ndig_frac++;
    i++;
  }
  if (ndig_frac == 0) return false;

  // pad right to 8 hex digits
  int pad_nibbles = 8 - ndig_frac;
  if (pad_nibbles > 0) frac_val <<= (4 * pad_nibbles);

  fixpoint_init(val, whole, frac_val, neg);
  return true;
}
