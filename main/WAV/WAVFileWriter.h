
#include <stdio.h>
#include "WAVFile.h"

#ifndef _WAVFILEWRITER_H
#define _WAVFILEWRITER_H

typedef struct WAVFileWriter
{
    int m_file_size;
    FILE *m_fp;
    wav_header_t  m_header;
} WAVFILEWRITER;

    void WAVFileWriter_init(WAVFILEWRITER * writer, FILE *fp, int sample_rate);
    void start();
    void write_wr(WAVFILEWRITER * writer, int16_t *samples, int count);
    void finish(WAVFILEWRITER * writer);

#endif