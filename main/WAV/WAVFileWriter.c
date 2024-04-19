#include "WAVFileWriter.h"
#include "esp_log.h"



static const char *TAG = "WAV";

void WAVFileWriter_init(WAVFILEWRITER * writer, FILE *fp, int sample_rate) 
{
     writer->m_fp = fp;
     writer->m_header.sample_rate = sample_rate;
     fwrite(&writer->m_header, sizeof(wav_header_t), 1, writer->m_fp);
     writer->m_file_size = sizeof(wav_header_t);
}

void write_wr(WAVFILEWRITER * writer ,int16_t *samples, int count) 
{
    fwrite(samples, sizeof(int16_t), count, writer->m_fp);
     writer->m_file_size += sizeof(int16_t) * count;
}

void finish(WAVFILEWRITER * writer)
{
  ESP_LOGI(TAG, "Finishing wav file size: %d", writer->m_file_size);
  writer->m_header.data_bytes = writer->m_file_size - sizeof(wav_header_t);
  writer->m_header.wav_size = writer->m_file_size - 8;
  fseek(writer->m_fp, 0, SEEK_SET);
  fwrite(&writer->m_header, sizeof(wav_header_t), 1, writer->m_fp);
}

