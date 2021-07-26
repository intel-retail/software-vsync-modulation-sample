#ifndef _DEBUG_H
#define _DEBUG_H

#include <stdio.h>

enum {
	ERR,
	INFO,
	DBG,
	TRACE,
};

/*
 * Set it to 0 or higher with 0 being lowest number of messages
 * 0 = Only errors show up
 * 1 = Errors + Info messages
 * 2 = Errors + Info messages + Debug messages
 * 3 = Errors + Info messages + Debug messages + Trace calls
 */
static int dbg_lvl = INFO;

#define _PRINT(prefix, fmt, ...)  \
	if(1) {\
		printf("%s", prefix); \
		printf(fmt, ##__VA_ARGS__); \
		fflush(stdout); \
	}

#define PRINT(fmt, ...)    _PRINT("", fmt, ##__VA_ARGS__)
#define ERR(fmt, ...)    _PRINT("[Error] ", fmt, ##__VA_ARGS__)
#define WARNING(fmt, ...)  _PRINT("[Warning] ", fmt, ##__VA_ARGS__)
#define DBG(fmt, ...)      if(dbg_lvl >= DBG) _PRINT("[DBG] ", fmt, ##__VA_ARGS__)
#define INFO(fmt, ...)     if(dbg_lvl >= INFO) _PRINT("[Info] ", fmt, ##__VA_ARGS__)
#define TRACE(fmt, ...)    if(dbg_lvl >= TRACE) PRINT(fmt, ##__VA_ARGS__)

#ifdef __cplusplus
#define TRACING()  tracer trace(__FUNCTION__);
#else
#define TRACING()
#endif

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

inline int set_dbg_lvl(int lvl)
{
	int ret = 1;
	if(lvl >= ERR && lvl <= TRACE) {
		dbg_lvl = lvl;
		ret = 0;
	}
	return ret;
}

#endif
