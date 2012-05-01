#include <stdio.h>
int main(int argc, char *argv[])
{
		//printf("[%s]\n", argv[1]);
		int i;
		char hostName[100];
		gethostname(hostName,100);
		printf("Computing my-pic %s on %s\n ", argv[1],hostName);
		printf("finish my-pic \n");
		sleep(2);
		return 0;
}
