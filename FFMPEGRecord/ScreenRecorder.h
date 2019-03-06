#pragma once

// for deprecated로 선언되었습니다.
#pragma warning(disable:4996)

//FFMPEG LIBRARIES
extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavcodec/avfft.h"

#include "libavdevice/avdevice.h"

#include "libavfilter/avfilter.h"
//#include "libavfilter/avfiltergraph.h"
#include "libavfilter/buffersink.h"
#include "libavfilter/buffersrc.h"

#include "libavformat/avformat.h"
#include "libavformat/avio.h"

// libav resample

#include "libavutil/opt.h"
#include "libavutil/common.h"
#include "libavutil/channel_layout.h"
#include "libavutil/imgutils.h"
#include "libavutil/mathematics.h"
#include "libavutil/samplefmt.h"
#include "libavutil/time.h"
#include "libavutil/opt.h"
#include "libavutil/pixdesc.h"
#include "libavutil/file.h"

// lib swresample

#include "libswscale/swscale.h"
}


#pragma comment(lib, "avcodec.lib")
#pragma comment(lib, "avformat.lib")
#pragma comment(lib, "avutil.lib")
#pragma comment(lib, "swscale.lib")
#pragma comment(lib, "avdevice.lib")
#pragma comment(lib, "avfilter.lib")
#pragma comment(lib, "swresample.lib")

class ScreenRecorder
{
public:
	ScreenRecorder();
	~ScreenRecorder();

	/* function to initiate communication with display library */
	int openCamera();
	int init_outputfile();
	int CaptureVideoFrames();

private:
	AVInputFormat *pAVInputFormat;
	AVOutputFormat *output_format;

	AVCodecContext *pAVCodecContext;

	AVFormatContext *pAVFormatContext;

	AVFrame *pAVFrame;
	AVFrame *outFrame;

	AVCodec *pAVCodec;
	AVCodec *outAVCodec;

	AVPacket *pAVPacket;

	AVDictionary *options;

	AVOutputFormat *outAVOutputFormat;
	AVFormatContext *outAVFormatContext;
	AVCodecContext *outAVCodecContext;

	AVStream *video_st;
	AVFrame *outAVFrame;

	const char *dev_name;
	const char *output_file;

	double video_pts;

	int out_size;
	int codec_id;
	int value;
	int VideoStreamIndx;
};


