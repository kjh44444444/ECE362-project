//This file is used to generate the wavetable.c file
//Use the following commands in the workspace/SoundGen_1 directory:
//
// % gcc -o util/genwave util/genwave.c -lm
// % ./util/genwave > src/wavetable.c
#include <stdio.h>
#include <math.h>
#include "/Users/coreyhackler/Documents/workspace/SoundGen_1/inc/constants.h"

int main(void) {
    int x;
    printf("//This file is only used for the purpose of storing the wavetable\n");
//    printf("//Uses 1/4 of a sine wave to conserve Flash ROM space"); //uncomment for 1/4 wavetable
    printf("#include \"wavetable.h\"\n\n");
    printf("const short int wavetable[%d] = {\n", N); //change to N/4 to implement 1/4 wavetable
    for(x=0; x<N; x++) {                              //change to N/4 to implement 1/4 wavetable
        int value = 32767 * sin(2 * M_PI * x / N);
        printf("%d, ", value);
        if ((x % 8) == 7) printf("\n");
}
    printf("};\n");
    return(0);
}
