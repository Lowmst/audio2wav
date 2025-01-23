#include <chrono>
#include <filesystem>
#include <print>
#include <vector>

#include "ffmpeg.h"
#include "wav.h"
#include <windows.h>

int main()
{
	int argc;
	wchar_t** argv = CommandLineToArgvW(GetCommandLineW(), &argc);

	if (argc < 2) {
		std::filesystem::path input_path(argv[0]);
		std::string output_filename = input_path.filename().stem().string() + ".exe";
		std::println("Usage: {} <input file>", output_filename);
		return 1;
	}

	//auto start = std::chrono::high_resolution_clock::now();

	// ��ʼ����ʽ������
	AVFormatContext* fmt_ctx = avformat_alloc_context();

	wchar_t* filename = argv[1];
	int utf8_size = WideCharToMultiByte(CP_UTF8, 0, filename, -1, NULL, 0, NULL, NULL);
	char* utf8_filename = (char*)malloc(utf8_size);
	WideCharToMultiByte(CP_UTF8, 0, filename, -1, utf8_filename, utf8_size, NULL, NULL);


	// ���ļ�����ȡ����Ϣ
	avformat_open_input(&fmt_ctx, utf8_filename, NULL, NULL);
	avformat_find_stream_info(fmt_ctx, NULL);

	std::println("������ʽ: {}", fmt_ctx->iformat->long_name);
	std::println("�����: {}", fmt_ctx->nb_streams);

	int audio_stream_index = 0; // ������Ƶ�������
	for (unsigned int i = 0; i < fmt_ctx->nb_streams; i++)
	{
		std::println("�� {} ��ý������: {} , �����ʽ: {}",
			i, av_get_media_type_string(fmt_ctx->streams[i]->codecpar->codec_type), avcodec_get_name(fmt_ctx->streams[i]->codecpar->codec_id));
		if (fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
		{
			audio_stream_index = i;
		}
	}

	// ��ȡ��Ƶ����ı������
	AVCodecParameters* codecpar = fmt_ctx->streams[audio_stream_index]->codecpar;
	std::println("��Ƶ��ʱ��� : {}/{} s", fmt_ctx->streams[audio_stream_index]->time_base.num, fmt_ctx->streams[audio_stream_index]->time_base.den);
	std::println("��Ƶ��ʱ���ʱ�� : {}", fmt_ctx->streams[audio_stream_index]->duration);

	std::println("��Ƶ��ʱ�� : {} s", av_rescale_q(fmt_ctx->streams[audio_stream_index]->duration, fmt_ctx->streams[audio_stream_index]->time_base, { 1, 1 }));
	std::println("��Ƶ������� : {} Hz", codecpar->sample_rate);

	int bits_per_sample = codecpar->bits_per_raw_sample ? codecpar->bits_per_raw_sample : codecpar->bits_per_coded_sample;
	std::println("��Ƶ��λ�� : {} bit", bits_per_sample);
	std::println("��Ƶ�������� : {}", codecpar->ch_layout.nb_channels);

	// ��ʼ�����ݰ�
	AVPacket* pkt = av_packet_alloc();

	// ���ҽ���������ʼ��������������
	const AVCodec* codec = avcodec_find_decoder(fmt_ctx->streams[audio_stream_index]->codecpar->codec_id);
	AVCodecContext* avc_ctx = avcodec_alloc_context3(codec);

	// ���������Ӧ���ڽ�����
	avcodec_parameters_to_context(avc_ctx, codecpar);
	avcodec_open2(avc_ctx, codec, NULL);

	// ��ʼ��֡
	AVFrame* frame = av_frame_alloc();

	std::filesystem::path input_path(filename);
	std::string output_filename = input_path.filename().stem().string() + ".wav";
	std::println("PCM д�� {}", output_filename);
	wav_output wav(output_filename.c_str(), codecpar->sample_rate, bits_per_sample);

	auto codec_start = std::chrono::high_resolution_clock::now();
	while (av_read_frame(fmt_ctx, pkt) == 0) // ѭ�������ݰ�
	{
		if (pkt->stream_index == audio_stream_index)
		{
			avcodec_send_packet(avc_ctx, pkt);

			while (avcodec_receive_frame(avc_ctx, frame) == 0) // ���͵������ݰ���ѭ��ȡ���������
			{
				wav.write_data(frame->data, frame->format, frame->nb_samples);
			}
		}
	}
	wav.write_head();

	// ���һ�οհ� // ��Ƶ������ʲ���Ҫ
	//int ret_send = avcodec_send_packet(avc_ctx, NULL);
	//if (ret_send == 0) {
	//	for (;;)
	//	{
	//		int ret_receive = avcodec_receive_frame(avc_ctx, frame);
	//		if (ret_receive == 0)
	//		{
	//			
	//		}
	//		else if (ret_receive == AVERROR_EOF)
	//		{
	//			break;
	//		}
	//	}
	//}

	//auto end = std::chrono::high_resolution_clock::now();
	//std::chrono::duration<double> duration = end - start;
	//std::println("time: {} s", duration.count());

	return 0;
}