#include "stdafx.h"
#include "ScreenRecorder.h"

using namespace std;

ScreenRecorder::ScreenRecorder()
{
	av_register_all();
	avcodec_register_all();
	avdevice_register_all();

	avformat_network_init();
}


ScreenRecorder::~ScreenRecorder()
{
	avformat_close_input(&pAVFormatContext);
	if (!pAVFormatContext)
	{
		AfxMessageBox(_T("file closed sucessfully"));
		//cout << "\nfile closed sucessfully";
	}
	else
	{
		AfxMessageBox(_T("unable to close the file"));
		//cout << "\nunable to close the file";
		exit(1);
	}

	avformat_free_context(pAVFormatContext);
	if (!pAVFormatContext)
	{
		AfxMessageBox(_T("avformat free successfully"));
		//cout << "\navformat free successfully";
	}
	else
	{
		AfxMessageBox(_T("unable to free avformat context"));
		//cout << "\nunable to free avformat context";
		exit(1);
	}
}


int ScreenRecorder::openCamera()
{
	// Open RTSP IP Camere
	value = 0;
	options = NULL;
	pAVFormatContext = NULL;

	pAVFormatContext = avformat_alloc_context();//Allocate an AVFormatContext.

	// 유니코드 to 멀티바이트
	CString m_strURL = _T("rtsp://192.168.1.100:554/profile1/media.smp");
	int iLength = m_strURL.GetLength();
	int iBytes = WideCharToMultiByte(CP_ACP, 0, m_strURL, iLength, NULL, 0, NULL, NULL);
	char* strFilePath = new char[iBytes + 1];
	memset(strFilePath, 0, iLength + 1);
	WideCharToMultiByte(CP_OEMCP, 0, m_strURL, iLength, strFilePath, iBytes, NULL, NULL);

	/* set option */
	value = av_dict_set(&options, "rtsp_transport", "tcp", 0);
	value = av_dict_set(&options, "stimeout", "1000*1000", 0);
	value = av_dict_set(&options, "framerate", "30", 0);
	if (value < 0)
	{
	
		AfxMessageBox(_T("error in setting dictionary value"));
		return 0;
	}
	value = av_dict_set(&options, "preset", "medium", 0);
	if (value < 0)
	{
		AfxMessageBox(_T("error in setting preset values"));
		return 0;
	}

	// RTSP Stream Open
	value = avformat_open_input(&pAVFormatContext, strFilePath, NULL, &options);
	if(value != 0)
	{
		AfxMessageBox(_T("RTSP open Error"));
		return 0;
	}

	value = avformat_find_stream_info(pAVFormatContext,NULL);
	if (value < 0)
	{
		AfxMessageBox(_T("unable to find the stream information"));
		return 0;
	}

	VideoStreamIndx = -1;

	/* find the first video stream index . Also there is an API available to do the below operations */
	for (int i = 0; i < pAVFormatContext->nb_streams; i++) // find video stream posistion/index.
	{
		if (pAVFormatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			VideoStreamIndx = i;
			break;
		}
	}

	if (VideoStreamIndx == -1)
	{
		AfxMessageBox(_T("unable to find the video stream index. (-1)"));
		return 0;
	}

	// assign pAVFormatContext to VideoStreamIndx
	pAVCodecContext = pAVFormatContext->streams[VideoStreamIndx]->codec;

	pAVCodec = avcodec_find_decoder(pAVCodecContext->codec_id);
	if (pAVCodec == NULL)
	{
		AfxMessageBox(_T("unable to find the decoder"));
		return 0;
	}

	value = avcodec_open2(pAVCodecContext, pAVCodec, NULL);//Initialize the AVCodecContext to use the given AVCodec.
	if (value < 0)
	{
		AfxMessageBox(_T("unable to open the av codec"));
		return 0;
	}
}

/* initialize the video output file and its properties  */
int ScreenRecorder::init_outputfile()
{
	outAVFormatContext = NULL;
	value = 0;
	output_file = "output.mp4";

	avformat_alloc_output_context2(&outAVFormatContext, NULL, NULL, output_file);
	if (!outAVFormatContext)
	{
		AfxMessageBox(_T("error in allocating av format output context"));
		return 0;
	}

	/* Returns the output format in the list of registered output formats which best matches the provided parameters, or returns NULL if there is no match. */
	output_format = av_guess_format(NULL, output_file, NULL);
	if (!output_format)
	{
		AfxMessageBox(_T("error in guessing the video format. try with correct format"));
		return 0;
	}

	video_st = avformat_new_stream(outAVFormatContext, NULL);
	if (!video_st)
	{
		AfxMessageBox(_T("error in creating a av format new stream"));
		return 0;
	}

	outAVCodecContext = avcodec_alloc_context3(outAVCodec);
	if (!outAVCodecContext)
	{
		AfxMessageBox(_T("error in allocating the codec contexts"));
		return 0;
	}

	//pAVCodecContext->codec_id

	/* set property of the video file */
	outAVCodecContext = video_st->codec;
	outAVCodecContext->codec_id = AV_CODEC_ID_MPEG4;// AV_CODEC_ID_MPEG4; // AV_CODEC_ID_H264 // AV_CODEC_ID_MPEG1VIDEO
	outAVCodecContext->codec_type = AVMEDIA_TYPE_VIDEO;
	outAVCodecContext->pix_fmt = AV_PIX_FMT_YUV420P;
	outAVCodecContext->bit_rate = 400000; // 2500000
	outAVCodecContext->width = 1920;
	outAVCodecContext->height = 1080;
	outAVCodecContext->max_b_frames = 2;	
	outAVCodecContext->time_base.num = 1;
	outAVCodecContext->time_base.den = 5;		// 테스트용 IPCamera의 경우 5프레임 설정을 이후에 변경하자

	if (codec_id == AV_CODEC_ID_H264)
	{
		av_opt_set(outAVCodecContext->priv_data, "preset", "slow", 0);
	}

	outAVCodec = avcodec_find_encoder(AV_CODEC_ID_MPEG4);
	if (!outAVCodec)
	{
		AfxMessageBox(_T("error in finding the av codecs. try again with correct codec"));
		return 0;
	}

	/* Some container formats (like MP4) require global headers to be present
	   Mark the encoder so that it behaves accordingly. */

	if (outAVFormatContext->oformat->flags & AVFMT_GLOBALHEADER)
	{
		outAVCodecContext->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
	}

	value = avcodec_open2(outAVCodecContext, outAVCodec, NULL);
	if (value < 0)
	{
		AfxMessageBox(_T("error in opening the avcodec"));
		return 0;
	}

	/* create empty video file */
	if (!(outAVFormatContext->flags & AVFMT_NOFILE))
	{
		if (avio_open2(&outAVFormatContext->pb, output_file, AVIO_FLAG_WRITE, NULL, NULL) < 0)
		{
			AfxMessageBox(_T("error in creating the video file"));
			return 0;
		}
	}

	if (!outAVFormatContext->nb_streams)
	{
		AfxMessageBox(_T("output file dose not contain any stream"));
		return 0;
	}

	/* imp: mp4 container or some advanced container file required header information*/
	value = avformat_write_header(outAVFormatContext, &options);
	if (value < 0)
	{
		AfxMessageBox(_T("error in writing the header context"));
		return 0;
	}
}




int ScreenRecorder::CaptureVideoFrames()
{
	int flag;
	int frameFinished;//when you decode a single packet, you still don't have information enough to have a frame [depending on the type of codec, some of them //you do], when you decode a GROUP of packets that represents a frame, then you have a picture! that's why frameFinished will let //you know you decoded enough to have a frame.

	int frame_index = 0;
	value = 0;

	pAVPacket = (AVPacket *)av_malloc(sizeof(AVPacket));
	av_init_packet(pAVPacket);
	

	pAVFrame = av_frame_alloc();
	if (!pAVFrame)
	{
		AfxMessageBox(_T("unable to release the avframe resources"));
		return 0;
	}

	outFrame = av_frame_alloc();//Allocate an AVFrame and set its fields to default values.
	if (!outFrame)
	{
		AfxMessageBox(_T("unable to release the avframe resources for outframe"));
		return 0;
	}

	int video_outbuf_size;
	int nbytes = av_image_get_buffer_size(outAVCodecContext->pix_fmt, outAVCodecContext->width, outAVCodecContext->height, 32);
	uint8_t *video_outbuf = (uint8_t*)av_malloc(nbytes);
	if (video_outbuf == NULL)
	{
		AfxMessageBox(_T("unable to allocate memory"));
		return 0;
	}

	// Setup the data pointers and linesizes based on the specified image parameters and the provided array.
	value = av_image_fill_arrays(outFrame->data, outFrame->linesize, video_outbuf, AV_PIX_FMT_YUV420P, outAVCodecContext->width, outAVCodecContext->height, 1); // returns : the size in bytes required for src
	if (value < 0)
	{
		AfxMessageBox(_T("error in filling image array"));
	}

	SwsContext* swsCtx_;

	// Allocate and return swsContext.
	// a pointer to an allocated context, or NULL in case of error
	// Deprecated : Use sws_getCachedContext() instead.
	swsCtx_ = sws_getContext(pAVCodecContext->width,
		pAVCodecContext->height,
		pAVCodecContext->pix_fmt,
		outAVCodecContext->width,
		outAVCodecContext->height,
		outAVCodecContext->pix_fmt,
		SWS_BICUBIC, NULL, NULL, NULL);


	int ii = 0;
	int no_frames = 100;	// 100 = 20s
	//cout << "\nenter No. of frames to capture : ";
	//cin >> no_frames;

	AVPacket outPacket;
	int j = 0;

	int got_picture;

	while (av_read_frame(pAVFormatContext, pAVPacket) >= 0)
	{
		if (ii++ == no_frames)break;
		if (pAVPacket->stream_index == VideoStreamIndx)
		{
			value = avcodec_decode_video2(pAVCodecContext, pAVFrame, &frameFinished, pAVPacket);
			if (value < 0)
			{
				//cout << "unable to decode video";
				AfxMessageBox(_T("unable to decode video"));
			}

			if (frameFinished)// Frame successfully decoded :)
			{
				sws_scale(swsCtx_, pAVFrame->data, pAVFrame->linesize, 0, pAVCodecContext->height, outFrame->data, outFrame->linesize);
				av_init_packet(&outPacket);
				outPacket.data = NULL;    // packet data will be allocated by the encoder
				outPacket.size = 0;

				avcodec_encode_video2(outAVCodecContext, &outPacket, outFrame, &got_picture);

				
				if (got_picture)
				{
					if (outPacket.pts != AV_NOPTS_VALUE)
						outPacket.pts = av_rescale_q(outPacket.pts, video_st->codec->time_base, video_st->time_base);
					if (outPacket.dts != AV_NOPTS_VALUE)
						outPacket.dts = av_rescale_q(outPacket.dts, video_st->codec->time_base, video_st->time_base);

					//printf("Write frame %3d (size= %2d)\n", j++, outPacket.size / 1000);
					if (av_write_frame(outAVFormatContext, &outPacket) != 0)
					{
						AfxMessageBox(_T("error in writing video frame"));
					}

					av_packet_unref(&outPacket);
				} // got_picture
				av_packet_unref(&outPacket);
			} // frameFinished

			// av_free_packet 없으면 메모리릭 발생 주의!
			av_free_packet(pAVPacket);
		}
	}// End of while-loop

	value = av_write_trailer(outAVFormatContext);
	if (value < 0)
	{
		AfxMessageBox(_T("error in writing av trailer"));
		return 0;
	}

	//THIS WAS ADDED LATER
	av_free(video_outbuf);
}