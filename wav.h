#pragma once
#include <fstream>
#include "ffmpeg.h"

typedef struct WAV_HEAD
{
	//Chunk "RIFF"
	uint8_t ID[4]; //"RIFF"
	uint32_t Size;
	uint8_t FourCC[4]; //"WAVE"

	//SubChunk "fmt"
	uint8_t fmtID[4]; //"fmt "
	uint32_t fmtSize; //16
	//"fmt" Data
	uint16_t encodeMode; //1 for PCM, ...
	uint16_t numChannel; //1 for Mono, 2 for Stereo
	uint32_t samplingRate; //usually 44100 or higher for lossless
	uint32_t byteRate; //numChannel * samplingRate * bitDepth / 8
	uint16_t blockAlign; //numChannel * bitDepth / 8
	uint16_t bitDepth; //usually 16 for lossless

	//SubChunk "data"
	uint8_t dataID[4]; //"data"
	uint32_t dataSize;
	//"data" Data is the kind chosen in "encodeMode"
}wav_head;


class wav_output
{
public:
	wav_output(const char* filename, int sample_rate, int bits_per_sample);

	void write_sample(int64_t sample_left, int64_t sample_right, int format);
	void write_data(uint8_t** data, int format, int frame_nb_samples);
	void write_head();

private:
	template <typename T>
	void write_planar(uint8_t** data, int format, int frame_nb_samples);

	template <typename T>
	void write_packed(uint8_t** data, int format, int frame_nb_samples);

	std::ofstream file;
	wav_head* head;
	int nb_samples = 0; // all channels (e.g. in stereo, please multiply 2. but notice that there is only stereo support)

	int sample_rate;
	int bits_per_sample;
	int bytes_per_sample;
};