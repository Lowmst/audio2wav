#include <fstream>
#include "ffmpeg.h"
#include "wav.h"

wav_output::wav_output(const char* filename, int sample_rate, int bits_per_sample)
{
	this->file.open(filename, std::ios::binary);
	this->file.seekp(44, std::ios::beg);

	this->sample_rate = sample_rate;
	this->bits_per_sample = bits_per_sample;
	this->bytes_per_sample = (int)ceil((float)bits_per_sample / 8);

	this->head = new wav_head{
		{'R','I','F','F'},
		36,
		{'W','A','V','E'},
		{'f','m','t',' '},
		16,
		1,
		2,
		(uint32_t)sample_rate,
		(uint32_t)(2 * sample_rate * bits_per_sample / 8),
		(uint16_t)(2 * bits_per_sample / 8),
		(uint16_t)bits_per_sample,
		{'d','a','t','a'},
		0
	};
}

void wav_output::write_sample(int64_t sample_left, int64_t sample_right, int format)
{

	this->nb_samples += 2;

	int bytes_per_sample = av_get_bytes_per_sample(AVSampleFormat(format));
	sample_left = sample_left >> ((bytes_per_sample - this->bytes_per_sample) * 8);
	sample_right = sample_right >> ((bytes_per_sample - this->bytes_per_sample) * 8);

	char* bytes = new char[2 * this->bytes_per_sample];
	for (int i = 0; i < this->bytes_per_sample; i++)
	{
		bytes[i] = (char)((sample_left >> (8 * i)) & 0xFF);
	}
	for (int i = 0; i < this->bytes_per_sample; i++)
	{
		bytes[i + this->bytes_per_sample] = (char)((sample_right >> (8 * i)) & 0xFF);
	}

	this->file.write(bytes, 2 * this->bytes_per_sample);
	delete[] bytes;
}

void wav_output::write_head()
{
	int nb_bytes = this->nb_samples * this->bytes_per_sample;
	this->head->Size += nb_bytes;
	this->head->dataSize += nb_bytes;

	this->file.seekp(0, std::ios::beg);
	file.write((char*)this->head, sizeof(*head));
	delete head;
}

void wav_output::write_data(uint8_t** data, int format, int frame_nb_samples)
{
	if (av_sample_fmt_is_planar(AVSampleFormat(format)))
	{
		switch (format)
		{
		case AV_SAMPLE_FMT_U8P:
			write_planar<uint8_t>(data, format, frame_nb_samples);
			break;
		case AV_SAMPLE_FMT_S16P:
			write_planar<int16_t>(data, format, frame_nb_samples);
			break;
		case AV_SAMPLE_FMT_S32P:
			write_planar<int32_t>(data, format, frame_nb_samples);
			break;
		case AV_SAMPLE_FMT_S64P:
			write_planar<int64_t>(data, format, frame_nb_samples);
			break;
		}
	}
	else
	{
		switch (format)
		{
		case AV_SAMPLE_FMT_U8:
			write_packed<uint8_t>(data, format, frame_nb_samples);
			break;
		case AV_SAMPLE_FMT_S16:
			write_packed<int16_t>(data, format, frame_nb_samples);
			break;
		case AV_SAMPLE_FMT_S32:
			write_packed<int32_t>(data, format, frame_nb_samples);
			break;
		case AV_SAMPLE_FMT_S64:
			write_packed<int64_t>(data, format, frame_nb_samples);
			break;
		}
	}
}

template <typename T>
void wav_output::write_planar(uint8_t** data, int format, int frame_nb_samples)
{
	this->nb_samples += 2 * frame_nb_samples;
	int bytes_per_data_sample = av_get_bytes_per_sample(AVSampleFormat(format));
	
	T* samples = new T[2 * frame_nb_samples];
	for (int i = 0; i < frame_nb_samples; i++)
	{
		samples[2 * i] = ((T*)data[0])[i];
		samples[2 * i + 1] = ((T*)data[1])[i];
	}

	if (this->bytes_per_sample == bytes_per_data_sample)
	{
		this->file.write((char*)samples, 2 * sizeof(T) * frame_nb_samples);
	}
	else
	{
		char* bytes = new char[2 * frame_nb_samples * this->bytes_per_sample];
		int shift = (bytes_per_data_sample - this->bytes_per_sample) * 8;

		for (int i = 0; i < 2 * frame_nb_samples; i++)
		{
			for (int j = 0; j < this->bytes_per_sample; j++)
			{
				bytes[this->bytes_per_sample * i + j] = (char)((samples[i] >> (8 * j + shift)) & 0xFF);
			}
		}

		this->file.write(bytes, 2 * frame_nb_samples * this->bytes_per_sample);
		delete[] bytes;
	}
	delete[] samples;
}

template <typename T>
void wav_output::write_packed(uint8_t** data, int format, int frame_nb_samples)
{
	this->nb_samples += 2 * frame_nb_samples;
	int bytes_per_data_sample = av_get_bytes_per_sample(AVSampleFormat(format));
	

	if (this->bytes_per_sample == bytes_per_data_sample)
	{
		this->file.write((char*)data[0], 2 * sizeof(T) * frame_nb_samples);
	}
	else
	{
		char* bytes = new char[2 * frame_nb_samples * this->bytes_per_sample];
		int shift = (bytes_per_data_sample - this->bytes_per_sample) * 8;

		for (int i = 0; i < 2 * frame_nb_samples; i++)
		{
			for (int j = 0; j < this->bytes_per_sample; j++)
			{
				bytes[this->bytes_per_sample * i + j] = (char)((((T*)data[0])[i] >> (8 * j + shift)) & 0xFF);
			}
		}

		this->file.write(bytes, 2 * frame_nb_samples * this->bytes_per_sample);
		delete[] bytes;
	}
}