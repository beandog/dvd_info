#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <inttypes.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavformat/avio.h>
#include <libavutil/audio_fifo.h>
#include <libavutil/avstring.h>
#include <libavutil/imgutils.h>
#include <libavutil/timestamp.h>
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>

int main(int argc, char **argv) {

	const char *input_filename = NULL;

	if(argc < 2) {
		printf("dvd_av_info filename\n");
		return 1;
	}

	input_filename = argv[1];

	int retval = 0;
	unsigned int ix = 0;
	unsigned int audio_ix = 0;
	unsigned int subtitle_ix = 0;
	unsigned int num_audio_streams = 0;
	unsigned int num_subtitle_streams = 0;

	av_register_all();
	AVFormatContext *input = NULL;
	retval = avformat_open_input(&input, input_filename, NULL, NULL);

	if(retval < 0) {
		printf("Could not open file %s\n", input_filename);
		return 1;
	}

	retval = avformat_find_stream_info(input, NULL);

	if(retval < 0) {
		printf("Could not find stream information for %s\n", input_filename);
		return 1;
	}

	AVStream *data_stream = NULL;
	AVCodec *data_codec = NULL;
	AVCodecContext *data_codec_context = NULL;

	AVStream *video_stream = NULL;
	AVCodec *video_codec = NULL;
	AVCodecContext *video_codec_context = NULL;

	AVStream *audio_streams[8];
	AVCodec *audio_codecs[8];
	AVCodecParserContext *audio_ctxs[8];
	AVCodecContext *audio_codec_contexts[8];

	AVStream *subtitle_streams[32];

	// See also libavcodec/codec_desc.c for point of reference
	for(ix = 0; ix < input->nb_streams; ix++) {

		switch(input->streams[ix]->codecpar->codec_type) {

			case AVMEDIA_TYPE_DATA:
			data_stream = input->streams[ix];
			break;

			case AVMEDIA_TYPE_VIDEO:
			video_stream = input->streams[ix];
			break;

			case AVMEDIA_TYPE_AUDIO:
			audio_streams[audio_ix] = input->streams[ix];
			audio_ix++;
			num_audio_streams++;
			break;

			case AVMEDIA_TYPE_SUBTITLE:
			subtitle_streams[subtitle_ix] = input->streams[ix];
			subtitle_ix++;
			num_subtitle_streams++;
			break;

			case AVMEDIA_TYPE_UNKNOWN:
			case AVMEDIA_TYPE_ATTACHMENT:
			case AVMEDIA_TYPE_NB:
			break;

		}

	}

	// If decoding from an arbitrary media file, the decoder would be found by
	// scanning the media file. With a DVD, the data stream is DVD nav packets,
	// the video is MPEG2, etc., so load manually
	// data_codec = avcodec_find_decoder(data_stream->codecpar->codec_id);

	data_codec = avcodec_find_decoder(AV_CODEC_ID_DVD_NAV);
	data_codec_context = avcodec_alloc_context3(data_codec);
	avcodec_parameters_to_context(data_codec_context, data_stream->codecpar);
	avcodec_open2(data_codec_context, data_codec, NULL);

	video_codec = avcodec_find_decoder(AV_CODEC_ID_MPEG2VIDEO);
	video_codec_context = avcodec_alloc_context3(video_codec);
	avcodec_parameters_to_context(video_codec_context, video_stream->codecpar);
	avcodec_open2(video_codec_context, video_codec, NULL);

	// Fields which are missing from AVCodecParameters need to be taken from the AVCodecContext (see libavformat/dump.c)
	video_codec_context->properties = video_stream->codec->properties;
	video_codec_context->codec = video_stream->codec->codec;
	video_codec_context->qmin = video_stream->codec->qmin;
	video_codec_context->qmax = video_stream->codec->qmax;
	video_codec_context->coded_width = video_stream->codec->coded_width;
	video_codec_context->coded_height = video_stream->codec->coded_height;

	AVRational display_aspect_ratio;
	av_reduce(&display_aspect_ratio.num, &display_aspect_ratio.den, video_codec_context->width * (int64_t)video_codec_context->sample_aspect_ratio.num, video_codec_context->height * (int64_t)video_codec_context->sample_aspect_ratio.den, 1024 * 1024);

	printf("[%s]\n", input_filename);
	printf("* num. streams: %u\n", input->nb_streams);
	printf("* bitrate: %"PRId64" kb/s\n", (int64_t)input->bit_rate / 1000);
	printf("\n");

	printf("[data]\n");
	printf("* data stream id: 0x%x\n", data_stream->id);
	printf("\n");

	printf("[video]\n");
	printf("* video stream id: 0x%x\n", video_stream->id);
	printf("* video codec name: %s\n", video_codec->name);
	printf("* video start_time: %lu\n", video_stream->start_time);
	printf("* video duration: %lu\n", video_stream->duration);
	// all three will be 0
	// printf("* video bitrate: %lu\n", video_codec_context->bit_rate);
	// printf("* video frame size: %i\n", video_codec_context->frame_size);
	// printf("* video bits per raw sample: %i\n", video_codec_context->bits_per_raw_sample);
	printf("* video color range: %s\n", av_color_range_name(video_codec_context->color_range));
	printf("* video width: %d\n", video_codec_context->width);
	printf("* video height: %d\n", video_codec_context->height);
	printf("* video aspect ratio: %d:%d\n", display_aspect_ratio.num, display_aspect_ratio.den); 
	if(video_codec_context->properties & FF_CODEC_PROPERTY_CLOSED_CAPTIONS)
	printf("* video closed captions: %s\n", (video_codec_context->properties & FF_CODEC_PROPERTY_CLOSED_CAPTIONS) ? "yes" : "no");
	
	// Note that libavformat uses the startcode as a reference for the audio codec
	// 0x80 - 0x87 is AV_CODEC_ID_AC3
	// 0x88 - 0x8f or 0x98 - 0x9f are AV_CODEC_ID_DTS
	// See libavformat/mpeg.c

	for(ix = 0; ix < num_audio_streams; ix++) {

		audio_ctxs[ix] = audio_streams[ix]->parser;
		audio_codecs[ix] = avcodec_find_decoder(audio_streams[ix]->codecpar->codec_id);
		audio_codec_contexts[ix] = avcodec_alloc_context3(audio_codecs[ix]);
		avcodec_parameters_to_context(audio_codec_contexts[ix], audio_streams[ix]->codecpar);
		avcodec_open2(audio_codec_contexts[ix], audio_codecs[ix], NULL);

		printf("\n");
		printf("[audio stream %i of %i]\n", ix + 1, num_audio_streams);
		printf("* audio stream id: 0x%x\n", audio_streams[ix]->id);
		printf("* audio codec name: %s\n", audio_codecs[ix]->name);
		printf("* audio start_time: %lu\n", audio_streams[ix]->start_time);
		printf("* audio duration: %lu\n", audio_streams[ix]->duration);
		printf("* audio channels: %i\n", audio_codec_contexts[ix]->channels);
		printf("* audio bitrate: %lu\n", audio_codec_contexts[ix]->bit_rate);
		printf("* audio frame size: %i\n", audio_codec_contexts[ix]->frame_size);
	
	}

	printf("\n");
	printf("[av_dump_format]\n");
	av_dump_format(input, 0, input_filename, 0);

	return 0;

}
