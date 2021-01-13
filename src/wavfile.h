/*
    This file part of e502monitor source code.
    Licensed under GPLv3.

    "wavfile.h" contains defines of wavfile-structure and function for it.

    Author: Gapeev Maksim
    Email: gm16493@gmail.com
*/
#include <stdint.h>
#include <stdio.h>

typedef struct{
    char chunk_id[4];
    int32_t chunk_size;
    char format[4];
    char subchunk1_id[4];
    int32_t subchunk1_size;
    int16_t audio_format;
    int16_t num_channels;
    int32_t sample_rate;
    int32_t byte_rate;
    int16_t block_align;
    int16_t bps;
    char subchunk2_id[4];
    int32_t subchunk2_size;
} wav_hdr;

wav_hdr create_wav_hdr(int16_t num_channels,
                       int32_t sample_rate,
                       int16_t bps,
                       int32_t size)
{
    wav_hdr whdr;
    
    whdr.chunk_id[0] = 'R';
    whdr.chunk_id[1] = 'I';
    whdr.chunk_id[2] = 'F';
    whdr.chunk_id[3] = 'F';

    whdr.format[0] = 'W';
    whdr.format[1] = 'A';
    whdr.format[2] = 'V';
    whdr.format[3] = 'E';

    whdr.subchunk1_id[0] = 'f';
    whdr.subchunk1_id[1] = 'm';
    whdr.subchunk1_id[2] = 't';
    whdr.subchunk1_id[3] = ' ';

    whdr.subchunk1_size = 16; // for PCM
    whdr.audio_format = 1;

    whdr.subchunk2_id[0] = 'd';
    whdr.subchunk2_id[1] = 'a';
    whdr.subchunk2_id[2] = 't';
    whdr.subchunk2_id[3] = 'a';

    whdr.num_channels = num_channels;
    whdr.sample_rate = sample_rate;
    whdr.bps = bps;

    whdr.byte_rate = whdr.sample_rate * whdr.num_channels * whdr.bps / 8;
    whdr.block_align = whdr.num_channels * whdr.bps / 8;

    whdr.subchunk2_size = size * num_channels * sample_rate * bps;

    whdr.chunk_size = sizeof(wav_hdr) - 8 + whdr.subchunk2_size;

    return whdr;
}

void print_hdr_info(wav_hdr *hdr)
{
    printf("Chunk Id:\t%s:\n", hdr->chunk_id);
    printf("Chunk size:\t%i\n", hdr->chunk_size);
    printf("Format:\t%s\n", hdr->format);
    printf("Subchink 1 Id:\t%s\n", hdr->subchunk1_id);
    printf("Subchunk 1 size:\t%i\n", hdr->subchunk1_size);
    printf("Audio format:\t%i\n", hdr->audio_format);
    printf("Number of channels:\t%i\n", hdr->num_channels);
    printf("Sample rate:\t%i\n", hdr->sample_rate);
    printf("Bytes rate:\t%i\n", hdr->byte_rate);
    printf("Block align:\t%i\n", hdr->block_align);
    printf("Bites per sample:\t%i\n", hdr->bps);
    printf("Subchink 2 Id:\t%s\n", hdr->subchunk2_id);
    printf("Subchunk 2 size:\t%i\n", hdr->subchunk2_size);
}