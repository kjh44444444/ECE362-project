//This file is only used to generate and store the midiArray array
//to be referenced in other files

#include <stdio.h>
#include <math.h>

void genMidiArray(void){
    int a = 440;
    int size = 127;
    printf("const float midiArray[%d] = {\n", size);
    for(int i = 0; i < size; i++){
        float value = 13.75 * pow(2.0,((float)i - 9) / 12);
        printf("%.3f, ", value);
        if ((i % 8) == 7) printf("\n");
    }
    printf("};\n");
}
