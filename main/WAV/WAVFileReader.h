#ifndef _WAVFILEREADER_H
#define _WAVFILEREADER_H


#include <stdio.h>
#include "WAVFile.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

typedef struct WAVFileReader
{
    wav_header_t m_wav_header;
    FILE *m_fp;
}WAVFILEREADER;
    
    void WAVFileReader(WAVFILEREADER * reader, FILE *fp);
    int sample_rate();
    int read_fr(WAVFILEREADER * reader, int16_t *samples, int count);
    void deleteReader(WAVFILEREADER * reader);
#endif