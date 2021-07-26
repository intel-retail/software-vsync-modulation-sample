#ifndef _VSYNCALTER_H
#define _VSYNCALTER_H

#define MAX_TIMESTAMPS                10

int vsync_lib_init();
void vsync_lib_uninit();
void synchronize_vsync(double time_diff);
int get_vsync(long *vsync_array, int size);

#endif
