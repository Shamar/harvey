#include <u.h>
#include <libc.h>
#define RET 0xc3

int
printFirst(void *v, char *s)
{
	/* just not exit, please */
	if(strcmp(s, "alarm") == 0){
		fprint(2, "%d: noted: %s at %lld\n", getpid(), s, nsec());
		atnotify(printFirst, 0);
		return 1;
	}
	return 0;
}

int
failOnSecond(void *v, char *s)
{
	/* just not exit, please */
	if(strcmp(s, "alarm") == 0){
		print("FAIL\n");
		exits("FAIL");
	}
	return 0;
}

void
main(void)
{
	int64_t a2000, a500;
	if (!atnotify(printFirst, 1) || !atnotify(failOnSecond, 1)){
		fprint(2, "%r\n");
		exits("atnotify fails");
	}

	alarm(2000);
	a2000 = nsec();
	alarm(500);
	a500 = nsec();
	fprint(2, "%d: alarm(2000)@%lld, alarm(500)@%lld\n", getpid(), a2000, a500);
	while(sleep(5000) < 0)
		;
	
	print("PASS\n");
	exits("PASS");
}
