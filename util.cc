#ifndef __UTIL_CC__
#define __UTIL_CC__

static int util_debugmode = 0;
#define debug(...) do {if(util_debugmode)fprintf(stderr, __VA_ARGS__);} while(0)

// implements ceil(log2(x))
int log2(int value)
{
	value--;
	if (value == 0)
		return 1;
	for(int i = 32; i >= 1; i--) {
		if(0x80000000 & value)
			return i;
		value = value << 1;
	}
	return 0;
}

int factorial(int n){
	if(n <= 1)
		return 1;
	else 
		return n*factorial(n-1);
}

void getparam(const char* name, int *value)
{
	char* tmpval = NULL;
	tmpval = getenv(name);
	if (tmpval)
		*value = atoi(tmpval);
}


#endif
