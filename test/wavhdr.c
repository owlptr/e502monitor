#include "../src/wavfile.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

int main(char* argv, int argc)
{
    wav_hdr wavhdr1 = create_wav_hdr(1, 44100, 16, 60);

    int16_t *values;
    


    values = (int16_t*)malloc(sizeof(int16_t) * wavhdr1.sample_rate * 60);

    print_hdr_info(&wavhdr1);

    for(int i = 0; i < wavhdr1.sample_rate * 60; i++)
    {
        values[i] = -1000;
    }
    

    FILE *wavfile;

    wavfile = fopen("test.wav", "wb");

    fwrite(&wavhdr1, sizeof(wavhdr1), 1, wavfile);
    fwrite(values, sizeof(int16_t), wavhdr1.sample_rate * 60, wavfile);

    fclose(wavfile);
    free(values);

    return 0;
}