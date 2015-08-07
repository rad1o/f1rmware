#include <stdint.h>
#include <stdlib.h>

void randomInit(void) {
    srand(0); // adcRead!
}

int getRandom(void){
    return rand();
}

