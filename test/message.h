// Copyright (C) 2023 Intel Corporation
// SPDX-License-Identifier: MIT

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
