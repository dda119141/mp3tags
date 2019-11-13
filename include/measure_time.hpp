#ifndef _MEASURE_TIME
#define _MEASURE_TIME


void measure_clock()
{
    clock_t t = clock();

    while( (clock() - t) * 1000/(CLOCKS_PER_SEC) < 0);

    printf("hjjjjjj\n");
}

#endif