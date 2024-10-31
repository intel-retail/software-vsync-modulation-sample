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

#ifndef _DEBUG_H
#define _DEBUG_H

enum log_level {
	LOG_LEVEL_NONE,
	LOG_LEVEL_ERROR,
	LOG_LEVEL_WARNING,
	LOG_LEVEL_INFO,
	LOG_LEVEL_DEBUG,
	LOG_LEVEL_TRACE,
};

extern log_level dbg_lvl;

void set_log_mode(const char* mode);
void set_log_level(log_level level);
void set_log_level(const char* log_level);
void log_message(log_level level, const char* format, ...);

#define PRINT(format, ...) \
	log_message(LOG_LEVEL_NONE, format, ##__VA_ARGS__)

#define ERR(format, ...) \
	log_message(LOG_LEVEL_ERROR, format, ##__VA_ARGS__)

#define WARNING(format, ...) \
	log_message(LOG_LEVEL_WARNING, format, ##__VA_ARGS__)

#define INFO(format, ...) \
	log_message(LOG_LEVEL_INFO, format, ##__VA_ARGS__)

#define DBG(format, ...) \
	log_message(LOG_LEVEL_DEBUG, format, ##__VA_ARGS__)

#define TRACE(format, ...) \
	log_message(LOG_LEVEL_TRACE, format, ##__VA_ARGS__)


#ifdef __cplusplus
class tracer {
private:
	char *m_func_name;
public:
	tracer(const char *func_name)
	{
		TRACE(">>> %s\n", func_name);
		m_func_name = (char *) func_name;
	}
	~tracer() { TRACE("<<< %s\n", m_func_name); }
};
#endif

#ifdef __cplusplus
#define TRACING()  tracer trace(__FUNCTION__);
#else
#define TRACING()
#endif

#endif // _DEBUG_H
