#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void main()
{
	int i=0;

	while(1)
	{
		printf("loop forever -- %d\n", i);
		i++;
		sleep(1);
	}
}
