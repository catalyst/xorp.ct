// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2011 XORP, Inc and Others
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License, Version 2, June
// 1991 as published by the Free Software Foundation. Redistribution
// and/or modification of this program under the terms of any other
// version of the GNU General Public License is not permitted.
// 
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. For more details,
// see the GNU General Public License, Version 2, a copy of which can be
// found in the XORP LICENSE.gpl file.
// 
// XORP Inc, 2953 Bunker Hill Lane, Suite 204, Santa Clara, CA 95054, USA;
// http://xorp.net



#include "rtrmgr_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/timeval.hh"
#include "libxorp/timer.hh"

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif

// XXX: Needs HAVE_OPENSSL_MD5_H
#include <openssl/md5.h>

#include "randomness.hh"


RandomGen::RandomGen()
    : _random_exists(false),
      _urandom_exists(false),
      _random_data(NULL),
      _counter(0)
{
    bool not_enough_randomness = false;
    FILE* file;

    file = fopen("/dev/urandom", "r");
    if (file == NULL) {
	_urandom_exists = false;
    } else {
	_urandom_exists = true;
	fclose(file);
	return;		// XXX: reading /dev/urandom is good enough method
    }

    file = fopen("/dev/random", "r");
    if (file == NULL) {
	_random_exists = false;
    } else {
	_random_exists = true;
	fclose(file);
    }

    if (_random_exists) {
	file = fopen("/dev/random", "r");
#ifdef O_NONBLOCK
	// We need non-blocking reads
	fcntl(fileno(file), F_SETFD, O_NONBLOCK);
#endif

	// Use /dev/random to initialized the random pool
	_random_data = new uint8_t[RAND_POOL_SIZE];
	int bytes = fread(_random_data, 1, RAND_POOL_SIZE, file);
	if (bytes < 16) {
	    // We didn't get enough randomness to be useful
	    not_enough_randomness = true;
	}
	fclose(file);
    }

    if ((!_urandom_exists && !_random_exists) || not_enough_randomness) {
	//
	// We need to generate our own randomness.  This is hard.
	// General strategy:
	//   1. Read a bunch of stuff that the attacker can't read
	//      (this assumes we're running as root).
	//   2. Read a bunch of stuff that the attacker can read, but
	//      that will be different each time the system starts.
	//   3. Build a big buffer of all this data, XORed together.
	//   4. Use MD5 to extract data from the buffer.
	//   5. Add timing-based randomness every opportunity we can.
	//
	if (_random_data == NULL)
	    _random_data = new uint8_t[RAND_POOL_SIZE];

	// Current time - this is mostly predictable, but a few less
	// significant bits are hard to predict.
	TimeVal now;
	TimerList::system_gettimeofday(&now);
	double d = now.get_double();
	for (size_t i = 0; i < sizeof(d); i++) {
	    _random_data[i] ^= *(((char *)(&d))+i);
	}

	int count = 0;
	int scount = 0;

	//
	// Read host-based secrets - good source of unpredictable data,
	// but we need to be very careful not to reveal the secrets.
	// Note: these so won't vary between iterations, so they're
	// definitely not good enough by themselves.
	//
	if (read_file("/etc/ssh_host_key")) {
	    scount++; count++;
	}
	if (read_file("/etc/ssh/ssh_host_key")) {
	    scount++; count++;
	}
	if (read_file("/etc/ssh_host_dsa_key")) {
	    scount++; count++;
	}
	if (read_file("/etc/ssh/ssh_host_dsa_key")) {
	    scount++; count++;
	}
	if (read_file("/etc/ssh_host_rsa_key")) {
	    scount++; count++;
	}
	if (read_file("/etc/ssh/ssh_host_rsa_key")) {
	    scount++; count++;
	}
	if (read_file("/etc/master.passwd")) {
	    scount++; count++;
	}
	if (read_file("/etc/shadow")) {
	    scount++; count++;
	}
	if (read_file("/etc/ssh_random_seed")) {
	    scount++; count++;
	}
	if (read_file("/tmp/rtrmgr-seed")) {
	    scount++; count++;
	}
	if (read_file("/usr/tmp/rtrmgr-seed")) {
	    scount++; count++;
	}
	if (read_file("/var/tmp/rtrmgr-seed")) {
	    scount++; count++;
	}

	//
	// Read packet counts - another source of hopefully
	// non-repeating but predictable data.
	//
	file = popen("/usr/bin/netstat -i", "r");
	if (file != NULL) {
	    count++;
	    read_fd(file);
	    pclose(file);
	} else {
	    XLOG_ERROR("popen of \"netstat -i\" failed");
	}

	//
	// Read last log - another source of hopefully
	// non-repeating but predictable data.
	//
	file = popen("/usr/bin/last", "r");
	if (file != NULL) {
	    count++;
	    read_fd(file);
	    pclose(file);
	} else {
	    debug_msg("popen of \"last\" failed\n");
	}

	//
	// Read current processes - another source of hopefully
	// non-repeating but predictable data.
	//
	file = popen("/usr/bin/ps -elf", "r");
	if (file != NULL) {
	    count++;
	    read_fd(file);
	    pclose(file);
	} else {
	    debug_msg("popen of \"ps -elf\" failed\n");
	}

	//
	// Read current processes, attempt 2 - another source of hopefully
	// non-repeating but predictable data.
	//
	file = popen("/usr/bin/ps -auxw", "r");
	if (file != NULL) {
	    count++;
	    read_fd(file);
	    pclose(file);
	} else {
	    debug_msg("popen of \"ps -auxw\" failed\n");
	}
	debug_msg("random data pool initialized: count=%d scount=%d\n",
		  count, scount);
	debug_msg("\n");
	for (size_t i = 0; i < 80; i++) {
	    debug_msg("%x", _random_data[i]);
	}
	debug_msg("\n");

	file = fopen("/tmp/rtrmgr-seed", "w");
	if (file == NULL)
	    file = fopen("/usr/tmp/rtrmgr-seed", "w");
	if (file == NULL)
	    file = fopen("/var/tmp/rtrmgr-seed", "w");
	if (file != NULL) {
	    uint8_t outbuf[16];
	    MD5_CTX md5_context;
	    MD5_Init(&md5_context);
	    MD5_Update(&md5_context, _random_data, RAND_POOL_SIZE);
	    MD5_Final(outbuf, &md5_context);
	    fwrite(outbuf, 1, 16, file);
	    fwrite(&d, 1, sizeof(d), file);
	    fclose(file);
	}
    }
}

RandomGen::~RandomGen()
{
    if (_random_data != NULL) {
	delete[] _random_data;
	_random_data = NULL;
    }
}

bool
RandomGen::read_file(const string& filename)
{
    bool result = false;
    FILE* file = fopen(filename.c_str(), "r");

    debug_msg("RNG: reading %s\n", filename.c_str());
    if (file == NULL) {
	debug_msg("failed to read %s\n", filename.c_str());
    } else {
	result = read_fd(file);
	fclose(file);
    }
    return result;
}

bool
RandomGen::read_fd(FILE* file)
{
    size_t bytes = 0;

    if (file != NULL) {
	char ixb[4];
	uint32_t ix;
	u_char tbuf[RAND_POOL_SIZE];

	bytes = fread(tbuf, 1, RAND_POOL_SIZE, file);
	debug_msg("RNG: read %u bytes\n", XORP_UINT_CAST(bytes));
	if (bytes > 0) {
	    size_t i;

	    debug_msg("orig tbuf:\n");
	    for (i = 0; i < min((size_t)80, bytes); i++)
		debug_msg("%2x", tbuf[i]);
	    debug_msg("\n");

	    //
	    // Use MD5 to self-encrypt the data.  We don't want to risk
	    // sensitive data being able to be read from a memory image.
	    //
	    MD5_CTX md5_context;
	    u_char chain[16];
	    MD5_Init(&md5_context);
	    MD5_Update(&md5_context, tbuf, bytes);
	    MD5_Final(chain, &md5_context);
	    for (i = 0; i < RAND_POOL_SIZE/16; i++) {
		MD5_Init(&md5_context);
		MD5_Update(&md5_context, chain, 16);
		MD5_Update(&md5_context, tbuf + (i*16), 16);
		MD5_Final(chain, &md5_context);
		// Overwrite in situe
		memcpy(tbuf + (i * 16), chain, 16);
		if (i * 16 > bytes) {
		    debug_msg("i=%u\n", XORP_UINT_CAST(i));
		    break;
		}
	    }

	    debug_msg("tbuf:\n");
	    for (i = 0; i < min((size_t)80, bytes); i++)
		debug_msg("%2x", tbuf[i]);
	    debug_msg("\n");

	    for (i = 0; i < bytes; i++) {
		// XOR the two together
		_random_data[i] ^= tbuf[i];
		ixb[i%4] ^= _random_data[i];
	    }
	    // Zero the temporary buffer, just to be safe
	    memset(tbuf, 0, bytes);

	    // Add more time bits - don't get a lot from this but it's cheap
	    TimeVal now;
	    TimerList::system_gettimeofday(&now);
	    double d = now.get_double();
	    memcpy(&ix, ixb, 4);
	    ix ^= now.usec();
	    ix = ix % (RAND_POOL_SIZE - sizeof(d));
	    // Add them at a "random" location - not truely random
	    // because we may not have enough entropy in the pool yet.
	    for (i = 0; i < sizeof(d); i++) {
		_random_data[i] ^= *(((char *)(&d)) + i + ix);
	    }
	}
    }
    if (bytes > 0)
	return true;
    return false;
}

void
RandomGen::add_buf_to_randomness(uint8_t* buf, size_t len)
{
    if (len > RAND_POOL_SIZE)
	len = RAND_POOL_SIZE;

    XLOG_ASSERT(_random_data != NULL);
    for (size_t i = 0; i < len; i++) {
	_random_data[i] ^= buf[i];
    }
}

// get_random_bytes is guaranteed to never block
void
RandomGen::get_random_bytes(size_t len, uint8_t* buf)
{
    // Don't allow stupid usage
    XLOG_ASSERT(len < RAND_POOL_SIZE/100);

    if (_urandom_exists) {
	FILE *file = fopen("/dev/urandom", "r");    
	if (file == NULL) {
	    XLOG_FATAL("Failed to open /dev/urandom");
	} else {
	    size_t bytes_read = fread(buf, 1, len, file);
	    if (bytes_read < len) {
		XLOG_ERROR("Failed read on randomness source; read %u words",
			   XORP_UINT_CAST(bytes_read));
	    }
	}
	fclose(file);
	return;
    }
    if (_random_exists) {
	FILE *file = fopen("/dev/random", "r");   
	if (file == NULL) {
	    XLOG_FATAL("Failed to open /dev/random");
	} else {
#ifdef O_NONBLOCK
	    fcntl(fileno(file), F_SETFD, O_NONBLOCK);
#endif
	    size_t bytes_read = fread(buf, 1, len, file);
	    if (len > 0)
		add_buf_to_randomness(buf, len);
	    if (bytes_read == len) {
		fclose(file);
		return;
	    }
	    fclose(file);
	    // else fall through
	}
    }

    //
    // Generate the random data using repeated MD5 hashing of the random pool
    //
    uint8_t outbuf[16];
    MD5_CTX md5_context;
    size_t bytes_written = 0;

    while (bytes_written < len) {
	TimeVal now;
	TimerList::system_gettimeofday(&now);
	double d = now.get_double();
	MD5_Init(&md5_context);
	_counter++;
	add_buf_to_randomness((uint8_t*)(&_counter), sizeof(_counter));
	add_buf_to_randomness((uint8_t*)(&d), sizeof(d));
	MD5_Update(&md5_context, _random_data, RAND_POOL_SIZE);
	MD5_Final(outbuf, &md5_context);
	memcpy(buf + bytes_written, outbuf,
	       min((size_t)16, len - bytes_written));
	bytes_written += min((size_t)16, len - bytes_written);
    }
}
