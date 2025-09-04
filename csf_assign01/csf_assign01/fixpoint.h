#ifndef FIXPOINT_H
#define FIXPOINT_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

////////////////////////////////////////////////////////////////////////
// Data types
////////////////////////////////////////////////////////////////////////

// Important: do not make any changes to the definitions of the
// fixpoint_t, result_t, and/or fixpoint_str_t data types

//! Type representing a "fixed point" value, with 32 unsigned
//! integers representing (respectively) the whole and fractional
//! parts of the value, and a boolean indicating whether or not
//! the value is negative.
typedef struct {
  uint32_t whole;  //!< whole part of the value
  uint32_t frac;   //!< fractional part of the value
  bool negative;   //!< true if negative
} fixpoint_t;

//! Bit values for result_t values.
enum {
  RESULT_OVERFLOW = 1,  //!< Operation overflowed
  RESULT_UNDERFLOW = 2, //!< Operation underflowed
};

typedef int result_t;

//! Maximum number of characters needed to represent a fixpoint_t
//! value as a string of base 16 or base 10 digits, including
//!
//! - optional leading minus sign (1)
//! - whole part (20)
//! - decimal point (1)
//! - fractional part (20)
//! - NUL terminator (1)
#define FIXPOINT_STR_MAX_SIZE (1 + 20 + 1 + 20 + 1)

//! Data type to use for representing a fixpoint_t value as a
//! base 16 or base 10 string.
typedef struct {
  char str[ FIXPOINT_STR_MAX_SIZE ]; //!< base 10 or base 16 string
} fixpoint_str_t;

#define RESULT_OK 0

////////////////////////////////////////////////////////////////////////
// Public API functions
////////////////////////////////////////////////////////////////////////

//! Initialize a fixpoint_t value.
//! Note that if the value is being initialized to 0 (i.e., both whole
//! and frac are 0), then negative should be false.
//!
//! @param val pointer to the fixpoint_t instance to initialize
//! @param whole 32 bit unsigned integer representing the whole part
//!              of the value
//! @param frac  32 bit unsigned integer representing the fractional
//!              part of the value
//! @param negative true if the value is negative, false if non-negative
void
fixpoint_init( fixpoint_t *val, uint32_t whole, uint32_t frac, bool negative );

//! Get the 32 bit unsigned integer value representing the whole part
//! of the value.
//!
//! @param val pointer to a fixpoint_t instance
//! @return the whole part of the value
uint32_t
fixpoint_get_whole( const fixpoint_t *val );

//! Get the 32 bit unsigned integer value representing the fractional part
//! of the value.
//!
//! @param val pointer to a fixpoint_t instance
//! @return the fractional part of the value
uint32_t
fixpoint_get_frac( const fixpoint_t *val );

//! Return whether a fixpoint_t value is negative.
//!
//! @param val pointer to a fixpoint_t instance
//! @return true if the value is negative, false if it is non-negative
bool
fixpoint_is_negative( const fixpoint_t *val );

//! Negate a fixpoint_t value.
//! If the value is not equal to 0 (i.e., either the whole part
//! or the fractional part is non-zero), then the value's sign is
//! flipped. If the value is equal to 0 (both whole and fractional
//! parts are 0), then the value remains non-negative.
//!
//! @param val pointer to the fixpoint_t instance to negate
void
fixpoint_negate( fixpoint_t *val );

//! Compute the sum of two fixpoint_t values.
//! If the exact sum can be represented in a fixpoint_t value,
//! *result is set to the sum and RESULT_OK is returned.
//! If the magnitude of the sum is too large to fix in a fixpoint_t value,
//! the truncated sum is stored in *result and RESULT_OVERFLOW is
//! returned.
//!
//! @param result pointer to result fixpoint_t instance (where the sum is stored)
//! @param left the left value to be added
//! @param right the right value to be added
//! @return RESULT_OK or RESULT_OVERFLOW
result_t
fixpoint_add( fixpoint_t *result, const fixpoint_t *left, const fixpoint_t *right );

//! Compute the difference of two fixpoint_t values.
//! If the exact difference can be represented in a fixpoint_t value,
//! *result is set to the difference and RESULT_OK is returned.
//! If the magnitude of the difference is too large to fix in a fixpoint_t value,
//! the truncated difference is stored in *result and RESULT_OVERFLOW is
//! returned.
//!
//! @param result pointer to result fixpoint_t instance (where the difference is stored)
//! @param left the left value in the subtraction (the minuend)
//! @param right the right value in the subtraction (the subtrahend)
//! @return RESULT_OK or RESULT_OVERFLOW
result_t
fixpoint_sub( fixpoint_t *result, const fixpoint_t *left, const fixpoint_t *right );

//! Compute the product of two fixpoint_t values.
//! The multiplication should take the two 64-bit input values (*left and *right),
//! compute an exact 128-bit intermediate product, discard the high and low
//! 32 bits of the intermediate product, and store the middle 64 bits of the
//! intermediate product in *result. If all of the discarded bits in the
//! intermediate product were 0, then RESULT_OK is returned. Otherwise,
//! a non-zero value with either RESULT_OVERFLOW, RESULT_UNDERFLOW, or both
//! should be returned. In the return value, RESULT_OVERFLOW indicates that
//! the high 32 bits of the intermediate product were not all 0, and
//! RESULT_UNDERFLOW indicates that the low 32 bits of the intermediate product
//! were not all 0.
//!
//! @param result pointer to result fixpoint_t instance (where product is stored)
//! @param left pointer to left value to be multiplied
//! @param right pointer to right value to be multiplied
//! @return RESULT_OK, or RESULT_OVERFLOW, or RESULT_UNDERFLOW,
//!         or (RESULT_OVERFLOW|RESULT_UNDERFLOW)
result_t
fixpoint_mul( fixpoint_t *result, const fixpoint_t *left, const fixpoint_t *right );

//! Compare two fixpoint_t values.
//!
//! @param left pointer to the left fixpoint_t instance to be compared
//! @param right pointer to the right fixpoint_t instance to be compared
//! @return -1 if *left < *right, 0 if *left == *right, and -1 if *left > *right
int
fixpoint_compare( const fixpoint_t *left, const fixpoint_t *right );

//! Format a fixpoint_t value as hexadecimal (base 16.)
//! The formatted string is stored in the fixpoint_str_t instance
//! pointed-to by s. The formatted string should have a leading
//! minus sign ("-") if the fixpoint_t value is negative.
//! The magnitude should be formatted as "XXXX.YYYY", where
//! "XXXX" is the hexadecimal representation of the whole part
//! without any leading zeroes, and "YYYY" is the hexadecimal
//! representation of the fractional part, without any trailing
//! zeroes. As an exception to the suppression of leading zeroes
//! of the whole part and trailing zeroes of the fractional part,
//! the whole part and the fractional part must each be represented
//! using at least one hexadecimal digit. I.e., the representation
//! of fixpoint_t value equal to 0 should be "0.0".
//!
//! @param s pointer to the fixpoint_str_t instance where the formatted
//!          base 16 string should be stored
//! @param val pointer to a fixpoint_t instance to be converted
//!            to base 16
void
fixpoint_format_hex( fixpoint_str_t *s, const fixpoint_t *val );

//! Convert a formatted base-16 string, in the format specified
//! in the documentation for the fixpoint_format_hex function,
//! to a fixpoint_t value. Note that if the string does not
//! contain a valid formatted base-16 value, the function
//! should return false, and there are no guarantees about what
//! is stored in the fixpoint_t instance.
//!
//! @param val pointer to the fixpoint_t instance where the converted
//!            value should be stored
//! @param s pointer to a fixpoint_str_t instance containing a
//!          formatted base-16 string
//! @return true if the string had a properly formed base-16
//!         value and the converted value was successfully
//!         stored in the fixpoint_t instance, false if
//!         the string was not well-formed
bool
fixpoint_parse_hex( fixpoint_t *val, const fixpoint_str_t *s );

// TODO: add prototypes for helper functions you want to test using unit tests

#endif // FIXPOINT_H
