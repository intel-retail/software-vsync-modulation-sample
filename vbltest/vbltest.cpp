#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <vsyncalter.h>
#include <debug.h>

using namespace std;
#define SIZE 10

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
		INFO("%ld\n", va[i]);
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
	int ret = 0;
	long client_vsync[SIZE], avg;
	if(vsync_lib_init()) {
		ERR("Couldn't initialize vsync lib\n");
		return 1;
	}

	if(get_vsync(client_vsync, SIZE)) {
		return 1;
	}

	avg = find_avg(client_vsync, SIZE);
	print_vsyncs((char *) "", client_vsync, SIZE);
	INFO("Time average of the vsyncs on the primary system is %ld\n", avg);

	vsync_lib_uninit();
	return ret;
}
