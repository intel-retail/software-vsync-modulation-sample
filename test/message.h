#ifndef _MESSAGE_H
#define _MESSAGE_H

enum header_t {
	ACK,
	NACK,
	VSYNC_MSG,
	CLOSE_MSG,
};

class msg {
protected:
	header_t header;
	long vsync_array[MAX_TIMESTAMPS];
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
	header_t get_type() { return header; }
	long *get_va() { return vsync_array; }
	int get_size() { return MAX_TIMESTAMPS; }
	int is_client_present() { return header != CLOSE_MSG; }
};

#endif
