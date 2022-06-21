#ifndef UNITOOL_H
#define UNITOOL_H

#ifdef __cplusplus
extern "C" {
#endif


/* This module is based on Unicode 2.1 */


/* parse_utf8_char parses the next utf-8 character, placing its
 * unicode equivalent in *value. The length of the utf-8 character
 * is returned. end is the address of the last character. */
int parse_utf8_char (const unsigned char *here,
		     const unsigned char *end,
		     unsigned short *value);

/* output_utf8_char encodes a unicode character as a UTF-8 character.
 * The length of the encoding is returned. If the string was not
 * long enough to encode the character 0 is returned. end is the
 * address of the last character */
int output_utf8_char (unsigned short value,
		      unsigned char *here,
		      unsigned char *end);

/* decompose_str will decompose a unicode string into its canonical
 * equivalents. NULL is returned if the input array was not 
 * large enough to contain the fully decomposed string (the array
 * will be in a correct, but partially decomposed state). The input 
 * must be null-terminated. */
unsigned short *decompose_str (unsigned short *input,
			       int max_output_len);

/* tests to see whether 'value' is a valid Unicode letter */
int is_unicode_letter (unsigned short value);

/* tests to see whether 'value' is a valid Unicode digit */
int is_unicode_digit (unsigned short value);

/* tests to see whether 'value' is a valid Unicode letter or 
 * digit */
int is_unicode_letdig (unsigned short value);

/* tests to see whether 'value' is a valid space
 * The test includes both "C" spaces and "Unicode" spaces, i.e. 
 * form-feed, newline, carriage return, horizontal tab,
 * vertical tab, and the Zs, Zl, and Zp Unicode categorizations */
int is_unicode_space (unsigned short value);

/* returns the length of the unicode string */
int unicode_strlen (const unsigned short *str);

/* returns the length of the unicode string, up to a maximum */
int unicode_strnlen (const unsigned short *str, int max_length);

/* returns the upper-case equivalent of value */
unsigned short unicode_toupper (unsigned short value);

/* returns the lower-case equivalent of value */
unsigned short unicode_tolower (unsigned short value);

/* returns the simplified Chinese character equivalent of 
 * another Chinese character */
unsigned short unicode_tosimplified (unsigned short value);


/* converts a utf-8 word (string with length stored in the first byte
 * to a Unicode array. To handle all situations the output buffer should
 * be 256 unsigned shorts long. The output will also have the length as
 * the first entry. */
unsigned short *utf8_word_to_unicode (const unsigned char *input,
				      unsigned short *output,
				      int max_output_length);

/* converts a unicode word buffer (with the length stored in the
 * entry) to a utf8 encoded word output (with the length stored in
 * the first byte. Only 255 bytes (not characters) can be stored 
 * in the output. */
unsigned char *unicode_to_utf8_word (const unsigned short *input,
				     unsigned char *output,
				     int max_output_length);

#ifdef __cplusplus
	   }
#endif

#endif
