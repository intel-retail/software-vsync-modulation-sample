#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <vsyncalter.h>
#include <debug.h>
#include <math.h>

using namespace std;

/*******************************************************************************
 * Description
 *	find_avg - This function finds the average of all the vertical syncs that
 *	have been provided by the primary system to the secondary.
 * Parameters
 *	long *va - The array holding all the vertical syncs of the primary system
 *	int sz - The size of this array
 * Return val
 *	long - The average of the vertical syncs
 ******************************************************************************/
long find_avg(long *va, int sz)
{
	int avg = 0;
	for(int i = 0; i < sz - 1; i++) {
		avg += va[i+1] - va[i];
	}
	return avg / ((sz == 1) ? sz : (sz - 1));
}

/*******************************************************************************
 * Description
 *	print_vsyncs - This function prints out the last N vsyncs that the system has
 *	received either on its own or from another system. It will only print in DBG
 *	mnode
 * Parameters
 *	char *msg - Any prefix message to be printed.
 *	long *va - The array of vsyncs
 *	int sz - The size of this array
 * Return val
 *	void
 ******************************************************************************/
void print_vsyncs(char *msg, long *va, int sz)
{
	INFO("%s VSYNCS\n", msg);
	for(int i = 0; i < sz; i++) {
		INFO("%6f\n", ((double) va[i])/1000000);
	}
}

/*******************************************************************************
 * Description
 *	main - This is the main function
 * Parameters
 *	int argc - The number of command line arguments
 *	char *argv[] - Each command line argument in an array
 * Return val
 *	int - 0 = SUCCESS, 1 = FAILURE
 ******************************************************************************/
int main(int argc, char *argv[])
{
	int ret = 0, size;
	long *client_vsync, avg;

	if(argc != 2) {
		ERR("Usage: %s <number of vsyncs to get timestamp for>\n", argv[0]);
		return 1;
	}

	size = atoi(argv[1]);
	if(size < 0 || size > 6000) {
		ERR("Specify the number of vsyncs to be between 1 and 600\n");
		return 1;
	}

	if(vsync_lib_init()) {
		ERR("Couldn't initialize vsync lib\n");
		return 1;
	}

	client_vsync = new long[size];

	if(get_vsync(client_vsync, size)) {
		delete [] client_vsync;
		vsync_lib_uninit();
		return 1;
	}

	avg = find_avg(&client_vsync[0], size);
	print_vsyncs((char *) "", client_vsync, size);
	INFO("Time average of the vsyncs on the primary system is %ld\n", avg);

	delete [] client_vsync;
	vsync_lib_uninit();
	return ret;
}
