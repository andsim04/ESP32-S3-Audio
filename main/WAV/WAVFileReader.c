#include "WAVFileReader.h"
#include "esp_log.h"


static const char *TAG = "WAV";

void WAVFileReader(WAVFILEREADER * reader, FILE *fp) 
{
    reader->m_fp = fp;
    // read the WAV header
    fread((void *)&reader->m_wav_header, sizeof(wav_header_t), 1, reader->m_fp);
    // sanity check the bit depth
    if (reader->m_wav_header.bit_depth != 16)
    {
        ESP_LOGE(TAG, "ERROR: bit depth %d is not supported\n", reader->m_wav_header.bit_depth);
    }
    if (reader->m_wav_header.num_channels != 1)
    {
        ESP_LOGE(TAG, "ERROR: channels %d is not supported\n", reader->m_wav_header.num_channels);
    }
    ESP_LOGI(TAG, "fmt_chunk_size=%d, audio_format=%d, num_channels=%d, sample_rate=%d, sample_alignment=%d, bit_depth=%d, data_bytes=%d\n",
             reader->m_wav_header.fmt_chunk_size, reader->m_wav_header.audio_format, reader->m_wav_header.num_channels, 
             reader->m_wav_header.sample_rate, reader->m_wav_header.sample_alignment, reader->m_wav_header.bit_depth,reader-> m_wav_header.data_bytes);

}


int sample_rate(WAVFILEREADER * reader) 
{
    return reader->m_wav_header.sample_rate;
}
int read_fr(WAVFILEREADER * reader , int16_t *samples, int count) 
{
    size_t read = fread(samples, sizeof(int16_t), count, reader->m_fp);
    return read;
}