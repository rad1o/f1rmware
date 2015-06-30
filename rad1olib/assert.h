#define CHECK
#ifdef CHECK
#define ASSERT(x) do{if(!(x))assert_die();}while(0)
#else
#define ASSERT(x)
#endif

void assert_die(void);
