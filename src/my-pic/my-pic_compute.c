#include <stdio.h>
int main(int argc, char *argv[])
{
		//printf("[%s]\n", argv[1]);
		int i;
		printf("Computing my-pic \n");
		printf("argc %d\n", argc);
		for(i=0;i<argc;i++){
				printf("%d. %s\n", i, argv[i]);
		}
		return 0;
}
