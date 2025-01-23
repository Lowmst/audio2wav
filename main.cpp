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

	// 初始化格式上下文
	AVFormatContext* fmt_ctx = avformat_alloc_context();

	wchar_t* filename = argv[1];
	int utf8_size = WideCharToMultiByte(CP_UTF8, 0, filename, -1, NULL, 0, NULL, NULL);
	char* utf8_filename = (char*)malloc(utf8_size);
	WideCharToMultiByte(CP_UTF8, 0, filename, -1, utf8_filename, utf8_size, NULL, NULL);


	// 打开文件并读取流信息
	avformat_open_input(&fmt_ctx, utf8_filename, NULL, NULL);
	avformat_find_stream_info(fmt_ctx, NULL);

	std::println("容器格式: {}", fmt_ctx->iformat->long_name);
	std::println("轨道数: {}", fmt_ctx->nb_streams);

	int audio_stream_index = 0; // 查找音频轨道索引
	for (unsigned int i = 0; i < fmt_ctx->nb_streams; i++)
	{
		std::println("第 {} 轨媒体类型: {} , 编码格式: {}",
			i, av_get_media_type_string(fmt_ctx->streams[i]->codecpar->codec_type), avcodec_get_name(fmt_ctx->streams[i]->codecpar->codec_id));
		if (fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
		{
			audio_stream_index = i;
		}
	}

	// 获取音频轨道的编码参数
	AVCodecParameters* codecpar = fmt_ctx->streams[audio_stream_index]->codecpar;
	std::println("音频轨时间基 : {}/{} s", fmt_ctx->streams[audio_stream_index]->time_base.num, fmt_ctx->streams[audio_stream_index]->time_base.den);
	std::println("音频轨时间基时长 : {}", fmt_ctx->streams[audio_stream_index]->duration);

	std::println("音频轨时长 : {} s", av_rescale_q(fmt_ctx->streams[audio_stream_index]->duration, fmt_ctx->streams[audio_stream_index]->time_base, { 1, 1 }));
	std::println("音频轨采样率 : {} Hz", codecpar->sample_rate);

	int bits_per_sample = codecpar->bits_per_raw_sample ? codecpar->bits_per_raw_sample : codecpar->bits_per_coded_sample;
	std::println("音频轨位深 : {} bit", bits_per_sample);
	std::println("音频轨声道数 : {}", codecpar->ch_layout.nb_channels);

	// 初始化数据包
	AVPacket* pkt = av_packet_alloc();

	// 查找解码器并初始化解码器上下文
	const AVCodec* codec = avcodec_find_decoder(fmt_ctx->streams[audio_stream_index]->codecpar->codec_id);
	AVCodecContext* avc_ctx = avcodec_alloc_context3(codec);

	// 将解码参数应用于解码器
	avcodec_parameters_to_context(avc_ctx, codecpar);
	avcodec_open2(avc_ctx, codec, NULL);

	// 初始化帧
	AVFrame* frame = av_frame_alloc();

	std::filesystem::path input_path(filename);
	std::string output_filename = input_path.filename().stem().string() + ".wav";
	std::println("PCM 写入 {}", output_filename);
	wav_output wav(output_filename.c_str(), codecpar->sample_rate, bits_per_sample);

	auto codec_start = std::chrono::high_resolution_clock::now();
	while (av_read_frame(fmt_ctx, pkt) == 0) // 循环读数据包
	{
		if (pkt->stream_index == audio_stream_index)
		{
			avcodec_send_packet(avc_ctx, pkt);

			while (avcodec_receive_frame(avc_ctx, frame) == 0) // 发送单个数据包后循环取解码后数据
			{
				wav.write_data(frame->data, frame->format, frame->nb_samples);
			}
		}
	}
	wav.write_head();

	// 最后发一次空包 // 音频流大概率不需要
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