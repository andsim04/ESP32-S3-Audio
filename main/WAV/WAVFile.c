#include "WAVFile.h"

void init_header(wav_header_t * header) {
    header->audio_format = 1;
    header->bit_depth = 16;
    header->byte_rate = 32000;
    header->data_bytes = 0;
    header->fmt_chunk_size = 16;
    header->num_channels = 1;
    header->sample_alignment = 2;
    header->sample_rate = 44100;
    header->wav_size = 0;
    header->riff_header[0] = 'R';
    header->riff_header[1] = 'I';
    header->riff_header[2] = 'F';
    header->riff_header[3] = 'F';
    header->wave_header[0] = 'W';
    header->wave_header[1] = 'A';
    header->wave_header[2] = 'V';
    header->wave_header[3] = 'E';
    header->fmt_header[0] = 'f';
    header->fmt_header[1] = 'm';
    header->fmt_header[2] = 't';
    header->fmt_header[3] = ' ';
    header->data_header[0] = 'd';
    header->data_header[1] = 'a';
    header->data_header[2] = 't';
    header->data_header[3] = 'a';
}