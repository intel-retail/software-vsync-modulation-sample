#ifndef _VSYNCALTER_H
#define _VSYNCALTER_H

#define ONE_VSYNC_PERIOD_IN_MS        16.666
#define MAX_TIMESTAMPS                100

int vsync_lib_init();
void vsync_lib_uninit();
void synchronize_vsync(double time_diff);
int get_vsync(long *vsync_array, int size, int pipe = 0);

#endif
