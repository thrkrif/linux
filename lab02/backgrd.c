#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
// 현재 시각을 출력한다.
void main()
{
	int i=0;
	time_t t;
	struct tm *tm;

	while(1)
	{
		sleep(1);
		i++;
		// time, localtime 쌍으로 나와야 우리가 아는 시간이 나온다. + asctime도 써야함.
		t = time(NULL);
		tm = localtime(&t);
		printf("%d from background job at %s\n", i, asctime(tm)); 
	}
}
