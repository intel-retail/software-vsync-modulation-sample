/*
 * Copyright © 2024 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 */

#ifndef _MESSAGE_H
#define _MESSAGE_H

#include <sys/time.h>
#include <debug.h>

enum header_t {
	ACK,
	NACK,
	VSYNC_MSG,
	CLOSE_MSG,
};

class msg {
protected:
	header_t header;
	timeval tv;
	long vsync_array[MAX_TIMESTAMPS];
	int vblank_count;
public:
	void ack() {
		header = ACK;
	}
	void nack() {
		header = NACK;
	}
	void add_vsync() {
		header = VSYNC_MSG;
	}
	void close() {
		header = CLOSE_MSG;
	}
	void add_time() {
		gettimeofday(&tv, 0);
	}
	void set_vblank_count(int vbl) {
		vblank_count = vbl;
	}

	void compare_time() {
		timeval tv_now, res;
		gettimeofday(&tv_now, 0);
		timersub(&tv_now, &tv, &res);
		DBG("Reached in %li secs, %li us\n", res.tv_sec, res.tv_usec);
	}

	header_t get_type() { return header; }
	long *get_va() { return vsync_array; }
	int get_size() { return MAX_TIMESTAMPS; }
	int is_client_present() { return header != CLOSE_MSG; }
	int get_vblank_count() { return vblank_count; }	
};

#endif
