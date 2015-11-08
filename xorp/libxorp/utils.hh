// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2011 XORP, Inc and Others
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License, Version
// 2.1, June 1999 as published by the Free Software Foundation.
// Redistribution and/or modification of this program under the terms of
// any other version of the GNU Lesser General Public License is not
// permitted.
// 
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. For more details,
// see the GNU Lesser General Public License, Version 2.1, a copy of
// which can be found in the XORP LICENSE.lgpl file.
// 
// XORP, Inc, 2953 Bunker Hill Lane, Suite 204, Santa Clara, CA 95054, USA;
// http://xorp.net

#ifndef __LIBXORP_UTILS_HH__
#define __LIBXORP_UTILS_HH__


#include "utility.h"

//
// Set of utilities
//

#define	PATH_CURDIR			"."
#define	PATH_PARENT			".."


#define	UNIX_PATH_DELIMITER_CHAR	'/'
#define	UNIX_PATH_DELIMITER_STRING	"/"
#define	UNIX_PATH_ENV_DELIMITER_CHAR	':'
#define	UNIX_PATH_ENV_DELIMITER_STRING	":"
#define	UNIX_EXECUTABLE_SUFFIX		""

#define	PATH_DELIMITER_CHAR		UNIX_PATH_DELIMITER_CHAR
#define	PATH_DELIMITER_STRING		UNIX_PATH_DELIMITER_STRING
#define	PATH_ENV_DELIMITER_CHAR		UNIX_PATH_ENV_DELIMITER_CHAR
#define	PATH_ENV_DELIMITER_STRING	UNIX_PATH_ENV_DELIMITER_STRING
#define	EXECUTABLE_SUFFIX		UNIX_EXECUTABLE_SUFFIX


/**
 * Determine if a provided native path string is an absolute path, or
 * possibly relative to a user's home directory under UNIX.
 *
 * @param path a path in native format to inspect.
 * @param homeok allow paths relative to a home directory to be regarded
 * as absolute paths by this function.
 * @return true if the path if satisfies the criteria for an absolute path.
 */
inline bool
is_absolute_path(const string& path, bool homeok = false)
{
    if (path.empty())
	return false;

    if (path[0] == '/')
        return true;
    if (homeok && path[0] == '~')
        return true;
    return false;
}

/**
 * Tokenize a string by breaking into separate strings when separator
 * character is encountered.
 *
 * @param s string to be tokenized.
 * @param sep separator to break string it.
 * @return list of tokens.
 */
list<string> split(const string& s, char sep);

/**
 * Remove the heading and trailing empty spaces from string value.
 *
 * @param s string that may have heading and trailing empty spaces.
 * @return copy of the string with heading and trailing empty spaces removed.
 */
string strip_empty_spaces(const string& s);

/**
 * Test if a string contains an empty space (i.e., <SPACE> or <TAB>).
 * 
 * @param s the string to test.
 * @return true if the string contains an empty space, otherwise false.
 */
bool has_empty_space(const string& s);

/**
 * Return basename of a path.
 *
 * @param argv0 the argv[0] supplied to main().
 */
const char* xorp_basename(const char* argv0);

/**
 * Template to delete a list of pointers, and the objects pointed to.
 * 
 * @param delete_list the list of pointers to objects to delete.
 */
template<class T> void
delete_pointers_list(list<T *>& delete_list)
{
    list<T *> tmp_list;
    
    // Swap the elements, so the original container never contains
    // entries that point to deleted elements.
    swap(tmp_list, delete_list);
    
    for (typename list<T *>::iterator iter = tmp_list.begin();
	 iter != tmp_list.end();
	 ++iter) {
	T *elem = (*iter);
	delete elem;
    }
    tmp_list.clear();
}

/**
 * Template to delete an array of pointers, and the objects pointed to.
 * 
 * @param delete_vector the vector of pointers to objects to delete.
 */
template<class T> void
delete_pointers_vector(vector<T *>& delete_vector)
{
    vector<T *> tmp_vector;
    // Swap the elements, so the original container never contains
    // entries that point to deleted elements.
    swap(tmp_vector, delete_vector);
    
    for (typename vector<T *>::iterator iter = tmp_vector.begin();
	 iter != tmp_vector.end();
	 ++iter) {
	T *elem = (*iter);
	delete elem;
    }
    tmp_vector.clear();
}

/**
 * Create a temporary file with unique name.
 * 
 * This function takes the given file name template, and adds a suffix
 * to it to create an unique file name in a chosen temporary directory.
 * The temporary directory is selected from the following list by choosing
 * the first directory that allows us the create the temporary file
 * (in the given order):
 * (a) The value of one of the following environment variables if defined
 *     (in the given order):
 *     - "TMPDIR"
 *     - "TEMP"   (Windows only)
 *     - "TMP"    (Windows only)
 * (b) Argument @ref tmp_dir if it is not an empty string.
 * (c) The system-specific path of the directory designated for
 *     temporary files:
 *     - GetTempPath() (Windows only)
 * (d) The "P_tmpdir" directory if this macro is defined (typically in
 *     the <stdio.h> include file).
 * (e) The first directory from the following list we can write to:
 *     - "C:\TEMP"  (Windows only)
 *     - "/tmp"     (UNIX only)
 *     - "/usr/tmp" (UNIX only)
 *     - "/var/tmp" (UNIX only)
 *
 * For example, if the file name template is "foo", and if the chosen
 * temporary directory is "/tmp", then the temporary file name would be
 * like "/tmp/foo.XXXXXX" where "XXXXXX" is alpha-numerical suffix
 * chosen automatically to compose the unique file name. The created file
 * has mode 0600 (i.e., readable and writable only by the owner).
 *
 * @param tmp_dir the preferred directory to store the file.
 * @param filename_template the file name template.
 * @param final_filename a return-by-reference argument that on success
 * returns the final name of the temporary file (including the directory name).
 * @param errmsg the error message (if error).
 * @return a file descriptor pointer for the temporary file (opened for
 * reading and writing) on success, otherwise NULL.
 */
FILE*	xorp_make_temporary_file(const string& tmp_dir,
				 const string& filename_template,
				 string& final_filename,
				 string& errmsg);


/**
 * Count the number of bits that are set in a 32-bit wide integer.
 *
 * Code taken from the following URL (the "Population Count (Ones Count)"
 * algorithm):
 * http://aggregate.org/MAGIC/
 *
 * Note: this solution appears to be faster even compared to the
 * "Parallel Count" and "MIT HACKMEM Count" from:
 * http://www-db.stanford.edu/~manku/bitcount/bitcount.html
 *
 * @param x the value to test.
 * @return the number of bits that are set in @ref x.
 */
inline uint32_t
xorp_bit_count_uint32(uint32_t x)
{
    //
    // 32-bit recursive reduction using SWAR...
    // but first step is mapping 2-bit values
    // into sum of 2 1-bit values in sneaky way.
    //
    x -= ((x >> 1) & 0x55555555);
    x = (((x >> 2) & 0x33333333) + (x & 0x33333333));
    x = (((x >> 4) + x) & 0x0f0f0f0f);
    x += (x >> 8);
    x += (x >> 16);
    x &= 0x0000003f;

    return (x);
}

/**
 * Count the number of leading zeroes in a 32-bit wide integer.
 *
 * Code taken from the following URL (the "Leading Zero Count" algorithm):
 * http://aggregate.org/MAGIC/
 *
 * @param x the value to test.
 * @return the number of leading zeroes in @ref x.
 */
inline uint32_t
xorp_leading_zero_count_uint32(uint32_t x)
{
    x |= (x >> 1);
    x |= (x >> 2);
    x |= (x >> 4);
    x |= (x >> 8);
    x |= (x >> 16);

    return (32 - xorp_bit_count_uint32(x));
}

#endif // __LIBXORP_UTILS_HH__
