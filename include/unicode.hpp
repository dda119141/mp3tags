/*******************************************************************************
 *                                                                             *
 * THIS FILE IS INCLUDED IN THIS LIBRARY, BUT IS NOT COPYRIGHTED BY THE TAGLIB *
 * AUTHORS, NOT PART OF THE TAG HANDLING API AND COULD GO AWAY AT ANY POINT IN
 *TIME. * AS SUCH IT SHOULD BE CONSIERED FOR INTERNAL USE ONLY. *
 *                                                                             *
 *******************************************************************************/

/*
 * Copyright 2001 Unicode, Inc.
 *
 * Disclaimer
 *
 * This source code is provided as is by Unicode, Inc. No claims are
 * made as to fitness for any particular purpose. No warranties of any
 * kind are expressed or implied. The recipient agrees to determine
 * applicability of information provided. If this file has been
 * purchased on magnetic or optical media from Unicode, Inc., the
 * sole remedy for any claim will be exchange of defective media
 * within 90 days of receipt.
 *
 * Limitations on Rights to Redistribute This Code
 *
 * Unicode, Inc. hereby grants the right to freely use the information
 * supplied in this file in the creation of products supporting the
 * Unicode Standard, and to make copies of this file in any form
 * for internal or external distribution as long as this notice
 * remains attached.
 */

/*
 * This file has been modified by Scott Wheeler <wheeler@kde.org> to remove
 * the UTF32 conversion functions and to place the appropriate functions
 * in their own C++ namespace.
 */

/* ---------------------------------------------------------------------

    Conversions between UTF32, UTF-16, and UTF-8. Source code file.
        Author: Mark E. Davis, 1994.
        Rev History: Rick McGowan, fixes & updates May 2001.
        Sept 2001: fixed const & error conditions per
                mods suggested by S. Parent & A. Lillich.

    See the header file "ConvertUTF.h" for complete documentation.

------------------------------------------------------------------------ */

#include <stdio.h>

#define UNI_SUR_HIGH_START (UTF32)0xD800
#define UNI_SUR_HIGH_END (UTF32)0xDBFF
#define UNI_SUR_LOW_START (UTF32)0xDC00
#define UNI_SUR_LOW_END (UTF32)0xDFFF

#define UNI_MAX_UTF16 (UTF32)0x0010FFFF
#define UNI_MAX_UTF32 (UTF32)0x7FFFFFFF
#define UNI_MAX_BMP (UTF32)0x0000FFFF
#define UNI_REPLACEMENT_CHAR (UTF32)0x0000FFFD

#define false 0
#define true 1

namespace Unicode
{

typedef unsigned long UTF32; /* at least 32 bits */
typedef wchar_t UTF16; /* We assume that wchar_t is sufficient for UTF-16. */
typedef unsigned char Boolean; /* 0 or 1 */

typedef enum {
  conversionOK = 0,    /* conversion successful */
  sourceExhausted = 1, /* partial character in source, but hit end */
  targetExhausted = 2, /* insuff. room in target for conversion */
  sourceIllegal = 3    /* source sequence is illegal/malformed */
} ConversionResult;

typedef enum { strictConversion = 0, lenientConversion } ConversionFlags;

const int halfShift = 10; /* used for shifting by 10 bits */
const UTF32 halfBase = 0x0010000UL;
const UTF32 halfMask = 0x3FFUL;

/* --------------------------------------------------------------------- */

/* The interface converts a whole buffer to avoid function-call overhead.
 * Constants have been gathered. Loops & conditionals have been removed as
 * much as possible for efficiency, in favor of drop-through switches.
 * (See "Note A" at the bottom of the file for equivalent code.)
 * If your compiler supports it, the "isLegalUTF8" call can be turned
 * into an inline function.
 */

/* --------------------------------------------------------------------- */

ConversionResult ConvertUTF16toUTF8(const UTF16 **sourceStart,
                                    const UTF16 *sourceEnd,
                                    unsigned char **targetStart,
                                    unsigned char *targetEnd,
                                    ConversionFlags flags)
{
  ConversionResult result = conversionOK;
  const UTF16 *source = *sourceStart;
  unsigned char *target = *targetStart;

  /*
   * Once the bits are split out into bytes of UTF-8, this is a mask OR-ed
   * into the first byte, depending on how many bytes follow.  There are
   * as many entries in this table as there are UTF-8 sequence types.
   * (I.e., one byte sequence, two byte... six byte sequence.)
   */
  const unsigned char firstByteMark[7] = {0x00, 0x00, 0xC0, 0xE0,
                                          0xF0, 0xF8, 0xFC};

  while (source < sourceEnd) {
    UTF32 ch;
    unsigned short bytesToWrite = 0;
    const UTF32 byteMask = 0xBF;
    const UTF32 byteMark = 0x80;
    const UTF16 *oldSource =
        source; /* In case we have to back up because of target overflow. */
    ch = *source++;
    /* If we have a surrogate pair, convert to UTF32 first. */
    if (ch >= UNI_SUR_HIGH_START && ch <= UNI_SUR_HIGH_END &&
        source < sourceEnd) {
      UTF32 ch2 = *source;
      if (ch2 >= UNI_SUR_LOW_START && ch2 <= UNI_SUR_LOW_END) {
        ch = ((ch - UNI_SUR_HIGH_START) << halfShift) +
             (ch2 - UNI_SUR_LOW_START) + halfBase;
        ++source;
      } else if (flags ==
                 strictConversion) { /* it's an unpaired high surrogate */
        --source;                    /* return to the illegal value itself */
        result = sourceIllegal;
        break;
      }
    } else if ((flags == strictConversion) &&
               (ch >= UNI_SUR_LOW_START && ch <= UNI_SUR_LOW_END)) {
      --source; /* return to the illegal value itself */
      result = sourceIllegal;
      break;
    }
    /* Figure out how many bytes the result will require */
    if (ch < (UTF32)0x80) {
      bytesToWrite = 1;
    } else if (ch < (UTF32)0x800) {
      bytesToWrite = 2;
    } else if (ch < (UTF32)0x10000) {
      bytesToWrite = 3;
    } else if (ch < (UTF32)0x200000) {
      bytesToWrite = 4;
    } else {
      bytesToWrite = 2;
      ch = UNI_REPLACEMENT_CHAR;
    }
    // printf("bytes to write = %i\n", bytesToWrite);
    target += bytesToWrite;
    if (target > targetEnd) {
      source = oldSource; /* Back up source pointer! */
      target -= bytesToWrite;
      result = targetExhausted;
      break;
    }
    switch (bytesToWrite) { /* note: everything falls through. */
    case 4:
      *--target = (ch | byteMark) & byteMask;
      ch >>= 6;
      [[fallthrough]];
    case 3:
      *--target = (ch | byteMark) & byteMask;
      ch >>= 6;
      [[fallthrough]];
    case 2:
      *--target = (ch | byteMark) & byteMask;
      ch >>= 6;
      [[fallthrough]];
    case 1:
      *--target = (unsigned char)(ch | firstByteMark[bytesToWrite]);
    }
    target += bytesToWrite;
  }
  *sourceStart = source;
  *targetStart = target;
  return result;
}

/* --------------------------------------------------------------------- */

/*
 * Utility routine to tell whether a sequence of bytes is legal UTF-8.
 * This must be called with the length pre-determined by the first byte.
 * If not calling this from ConvertUTF8to*, then the length can be set by:
 *	length = trailingBytesForUTF8[*source]+1;
 * and the sequence is illegal right away if there aren't that many bytes
 * available.
 * If presented with a length > 4, this returns false.  The Unicode
 * definition of UTF-8 goes up to 4-byte sequences.
 */

static Boolean isLegalUTF8(const unsigned char *source, int length)
{
  unsigned char a;
  const unsigned char *srcptr = source + length;
  switch (length) {
  default:
    return false;
    /* Everything else falls through when "true"... */
    [[fallthrough]];
  case 4:
    if ((a = (*--srcptr)) < 0x80 || a > 0xBF)
      return false;
    [[fallthrough]];
  case 3:
    if ((a = (*--srcptr)) < 0x80 || a > 0xBF)
      return false;
    [[fallthrough]];
  case 2:
    if ((a = (*--srcptr)) > 0xBF)
      return false;
    switch (*source) {
    /* no fall-through in this inner switch */
    case 0xE0:
      if (a < 0xA0)
        return false;
      break;
    case 0xF0:
      if (a < 0x90)
        return false;
      break;
    case 0xF4:
      if (a > 0x8F)
        return false;
      break;
    default:
      if (a < 0x80)
        return false;
    }
    [[fallthrough]];
  case 1:
    if (*source >= 0x80 && *source < 0xC2)
      return false;
    if (*source > 0xF4)
      return false;
  }
  return true;
}

/* --------------------------------------------------------------------- */

/*
 * Exported function to return whether a UTF-8 sequence is legal or not.
 * This is not used here; it's just exported.
 */
Boolean isLegalUTF8Sequence(const unsigned char *source,
                            const unsigned char *sourceEnd)
{
  /*
   * Index into the table below with the first byte of a UTF-8 sequence to
   * get the number of trailing bytes that are supposed to follow it.
   */
  const char trailingBytesForUTF8[256] = {
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
      1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
      3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5};

  int length = trailingBytesForUTF8[*source] + 1;
  if (source + length > sourceEnd) {
    return false;
  }
  return isLegalUTF8(source, length);
}

/* --------------------------------------------------------------------- */

ConversionResult ConvertUTF8toUTF16(const unsigned char **sourceStart,
                                    const unsigned char *sourceEnd,
                                    UTF16 **targetStart, UTF16 *targetEnd,
                                    ConversionFlags flags)
{

  ConversionResult result = conversionOK;
  const unsigned char *source = *sourceStart;
  UTF16 *target = *targetStart;

  /*
   * Magic values subtracted from a buffer value during unsigned char
   * conversion. This table contains as many values as there might be trailing
   * bytes in a UTF-8 sequence.
   */
  const UTF32 offsetsFromUTF8[6] = {0x00000000UL, 0x00003080UL, 0x000E2080UL,
                                    0x03C82080UL, 0xFA082080UL, 0x82082080UL};

  /*
   * Index into the table below with the first byte of a UTF-8 sequence to
   * get the number of trailing bytes that are supposed to follow it.
   */
  const char trailingBytesForUTF8[256] = {
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
      1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
      3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5};

  while (source < sourceEnd) {
    UTF32 ch = 0;
    unsigned short extraBytesToRead = trailingBytesForUTF8[*source];
    if (source + extraBytesToRead >= sourceEnd) {
      result = sourceExhausted;
      break;
    }
    /* Do this check whether lenient or strict */
    if (!isLegalUTF8(source, extraBytesToRead + 1)) {
      result = sourceIllegal;
      break;
    }
    /*
     * The cases all fall through. See "Note A" below.
     */
    switch (extraBytesToRead) {
    case 3:
      ch += *source++;
      ch <<= 6;
      [[fallthrough]];
    case 2:
      ch += *source++;
      ch <<= 6;
      [[fallthrough]];
    case 1:
      ch += *source++;
      ch <<= 6;
      [[fallthrough]];
    case 0:
      ch += *source++;
    }
    ch -= offsetsFromUTF8[extraBytesToRead];

    if (target >= targetEnd) {
      source -= (extraBytesToRead + 1); /* Back up source pointer! */
      result = targetExhausted;
      break;
    }
    if (ch <= UNI_MAX_BMP) { /* Target is a character <= 0xFFFF */
      if ((flags == strictConversion) &&
          (ch >= UNI_SUR_HIGH_START && ch <= UNI_SUR_LOW_END)) {
        source -=
            (extraBytesToRead + 1); /* return to the illegal value itself */
        result = sourceIllegal;
        break;
      } else {
        *target++ = (UTF16)ch; /* normal case */
      }
    } else if (ch > UNI_MAX_UTF16) {
      if (flags == strictConversion) {
        result = sourceIllegal;
        source -= (extraBytesToRead + 1); /* return to the start */
        break;                            /* Bail out; shouldn't continue */
      } else {
        *target++ = UNI_REPLACEMENT_CHAR;
      }
    } else {
      /* target is a character in range 0xFFFF - 0x10FFFF. */
      if (target + 1 >= targetEnd) {
        source -= (extraBytesToRead + 1); /* Back up source pointer! */
        result = targetExhausted;
        break;
      }
      ch -= halfBase;
      *target++ = (UTF16)((ch >> halfShift) + UNI_SUR_HIGH_START);
      *target++ = (ch & halfMask) + UNI_SUR_LOW_START;
    }
  }
  *sourceStart = source;
  *targetStart = target;
  return result;
}

} // namespace Unicode

/* ---------------------------------------------------------------------

        Note A.
        The fall-through switches in UTF-8 reading code save a
        temp variable, some decrements & conditionals.  The switches
        are equivalent to the following loop:
                {
                        int tmpBytesToRead = extraBytesToRead+1;
                        do {
                                ch += *source++;
                                --tmpBytesToRead;
                                if (tmpBytesToRead) ch <<= 6;
                        } while (tmpBytesToRead > 0);
                }
        In UTF-8 writing code, the switches on "bytesToWrite" are
        similarly unrolled loops.

   --------------------------------------------------------------------- */
