#ifndef FORMAT_TYPES_HPP
#define FORMAT_TYPES_HPP
/*------------------------------------------------------------------------------

  Author:    Andy Rushton
  Copyright: (c) Andy Rushton, 2004
  License:   BSD License, see ../docs/license.html

  ------------------------------------------------------------------------------*/

////////////////////////////////////////////////////////////////////////////////
// Integer radix display representations:
//   There are three ways in which the radix is represented:
//     - none - the number is just printed as a number (e.g. 12345). Can be confusing for non-decimal radix
//     - C style - for binary, octal and hex, the C-style prefices 0b, 0 and 0x are used - note that this is an unsigned
//     representation
//     - Hash style - in the form radix#value - the value may be signed, e.g. 10#-9

enum radix_display_t {
  radix_none,            // just print the number with no radix indicated
  radix_hash_style,      // none for decimal, hash style for all others
  radix_hash_style_all,  // hash style for all radices including decimal
  radix_c_style,         // C style for hex and octal, none for others
  radix_c_style_or_hash  // C style for hex and octal, none for decimal, hash style for others
};

////////////////////////////////////////////////////////////////////////////////
// Floating-point display representations:
// There are three formats for the printout:
//   - fixed - formatted as a fixed-point number, so no mantissa is printed (equivalent to %f in printf)
//   - floating - formatted as a normalised floating-point number (equivalent to %e in printf)
//   - mixed - formatted as fixed-point if this is appropriate, otherwise the floating format (equivalent to %g in
//   printf)

enum real_display_t {
  display_fixed,     // %f
  display_floating,  // %e
  display_mixed      // %g
};

////////////////////////////////////////////////////////////////////////////////
// Alignment:
//   There are three field alignments:
//     - left aligned - the value is to the left of the field which is padded to the right with spaces
//     - right aligned - the value is to the right of the field which is padded to the left with spaces
//     - centred - the value is in the centre of the field and spaces added to both left and right

enum alignment_t { align_left, align_right, align_centre };

////////////////////////////////////////////////////////////////////////////////
#endif
