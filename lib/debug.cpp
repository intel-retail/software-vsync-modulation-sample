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

#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <string>
#include "debug.h"


log_level dbg_lvl = LOG_LEVEL_INFO;
std::string mode_str = "";

/**
* @brief
* This function sets the logging level for the application.
* The logging level determines the severity of messages that will be logged.
*
* @param level - The logging level to set, specified as a LogLevel enum value.
* @return void
*/
void set_log_level(log_level level)
{

	dbg_lvl = level;
}
/**
* @brief
* This function sets the logging level for the application via string parameter.
* The logging level determines the severity of messages that will be logged.
*
* @param level - The logging level to set, specified as a string value.
* @return void
*/
void set_log_level(const char* log_level)
{

	if (log_level == NULL) {
		return;
	}

	if (strcasecmp(log_level, "error") == 0) {
		set_log_level(LOG_LEVEL_ERROR);
	} else if (strcasecmp(log_level, "warning") == 0) {
		set_log_level(LOG_LEVEL_WARNING);
	} else if (strcasecmp(log_level, "info") == 0) {
		set_log_level(LOG_LEVEL_INFO);
	} else if (strcasecmp(log_level, "debug") == 0) {
		set_log_level(LOG_LEVEL_DEBUG);
	} else if (strcasecmp(log_level, "trace") == 0) {
		set_log_level(LOG_LEVEL_TRACE);
	}
}

/**
* @brief
* This function sets the logging mode for the application.
* The logging mode determines the source for the log messages.
* (PRIMARY, SECONDARY, VBLTEST, SYNCTEST).
*
* @param mode - A string representing the logging mode (e.g., "console", "file").
* @return void
*/
void set_log_mode(const char* mode)
{

	if (mode != NULL) {
		mode_str = mode;
	}
}

/**
* @brief
* This function logs a message at the specified logging level.
* It supports formatted output similar to printf-style formatting.
*
* @param level - The logging level of the message, specified as a LogLevel enum value.
* @param format - A format string that specifies how to format the message.
* @param ... - Additional arguments to be formatted according to the format string.
* @return void
*/
void log_message(log_level level, const char* format, ...)
{

	if (level > dbg_lvl) {
		return;
	}

	const char* level_str = nullptr;
	switch (level) {
		case LOG_LEVEL_NONE:
			level_str = "";
			break;
		case LOG_LEVEL_ERROR:
			level_str = "[ERR ]";
			break;
		case LOG_LEVEL_WARNING:
			level_str = "[WARN]";
			break;
		case LOG_LEVEL_INFO:
			level_str = "[INFO]";
			break;
		case LOG_LEVEL_DEBUG:
			level_str = "[DBG ]";
			break;
		case LOG_LEVEL_TRACE:
			level_str = "[TRACE]";
			break;
		default:
			level_str = "";
			break;
	}

	printf("%s", mode_str.c_str());
	printf("%s ", level_str);
	va_list args;
	va_start(args, format);
	vprintf(format, args);
	va_end(args);
	fflush(stdout);
}
