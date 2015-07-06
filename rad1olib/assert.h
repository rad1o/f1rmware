#ifndef _ASSERT_H
#define _ASSERT_H 1

#define CHECK
#ifdef CHECK
#define ASSERT(x) do{if(!(x))assert_die();}while(0)
#else
#define ASSERT(x)
#endif

void assert_die(void);

#endif /* _ASSERT_H */
