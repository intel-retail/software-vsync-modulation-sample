This program allows two systems to synchronize their vertical sync firing times.

Building steps:
1) Type 'make release' from the main directory. It compiles everything and creates a 
   release folder which can then be copied to the target systems.

Building of this program has been succesfully tested on both Ubuntu 20 and Fedora 30.

Installing and running the programs:
1) Copy the release folder contents to both primary and secondary systems.
2) On both systems, go to the directory where these files exist and set environment variable:
export LD_LIBRARY_PATH=.
3) On the primary system, run it as follows:
	./vsync_test pri
3) On the secondary system, run it as follows:
	./vsync_test sec PRIMARY'S_NAME_OR_IP

This program runs in server mode on the primary system and in client mode on the 
secondary system.
