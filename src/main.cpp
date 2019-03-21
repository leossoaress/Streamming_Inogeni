#include <iostream>
#include <string>

extern "C" {
#include <libavdevice/avdevice.h>
#include <libavfilter/avfilter.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
}

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

using namespace std;
using namespace cv;

#define AV_OUTPUT_FORMAT "mpegts"
#define AV_OUTPUT_CODEC  "libx264"
#define AV_OUTPUT_BITRATE 6000000

#define OUTPUT_WIDTH 640
#define OUTPUT_HEIGHT 480

typedef struct {
    AVFormatContext *pFmtCtx    = nullptr;
    AVCodecContext  *pCodecCtx  = nullptr;
    AVCodec         *pCodec     = nullptr;
    AVStream        *pStream    = nullptr;
    AVFrame         *pFrame     = nullptr;
    AVPacket        *pPacket    = nullptr;
} ff_output_t;

typedef struct {
    AVFormatContext *pFmtCtx    = nullptr;
    AVCodecContext  *pCodecCtx  = nullptr;
    AVInputFormat   *pInputFmt  = nullptr;
    AVCodec         *pCodec     = nullptr;
    AVStream        *pStream    = nullptr;
    AVFrame         *pFrame     = nullptr;
    AVPacket        *pPacket    = nullptr;
} ff_input_t;

void Erro(string msg) {
    clog << "Error: " << msg << endl;
    exit(0);
}

int main(int argc, char **argv) {

    av_register_all();
    avcodec_register_all();
    avdevice_register_all();
    avformat_network_init();

    ff_input_t input;
    ff_output_t output;

    const char outputName[] = "udp://127.0.0.1:6000";
    int videoId = -1;
    int ret = 0;


    input.pInputFmt = av_find_input_format("v4l2");
    if(!input.pInputFmt) Erro("Input format could not be found");

    AVDictionary *inputOpts = nullptr;
    av_dict_set(&inputOpts, "video_size", "uhd2160", 0);
    av_dict_set(&inputOpts, "framerate", "30", 0);
    av_dict_set(&inputOpts, "preset", "fast", 0);

    ret = avformat_open_input(&input.pFmtCtx, "/dev/video0", input.pInputFmt, &inputOpts);
    if(ret < 0) Erro("Could not open input");

    ret = avformat_find_stream_info(input.pFmtCtx, nullptr);
    if(ret < 0) Erro("Could not find stream info");

    av_dump_format(input.pFmtCtx, 0, "/dev/video0", 0);
    cout << endl;

    AVCodecParameters *pCodecAux = nullptr;

    for(unsigned int i = 0; i < input.pFmtCtx->nb_streams; ++i) {
        pCodecAux = input.pFmtCtx->streams[i]->codecpar;
        if(pCodecAux->codec_type == AVMEDIA_TYPE_VIDEO) {
            input.pCodec = avcodec_find_decoder(pCodecAux->codec_id);
            videoId = i;
            break;
        }
    }

    input.pCodecCtx = avcodec_alloc_context3(input.pCodec);
    if(!input.pCodecCtx) Erro("Could not alloc input codec context");

    ret = avcodec_parameters_to_context(input.pCodecCtx, pCodecAux);
    if(ret < 0) Erro("Could not copy parameters to context");

    const int INPUT_WIDTH = input.pCodecCtx->width;
    const int INPUT_HEIGHT = input.pCodecCtx->height;

    std::cout << input.pCodecCtx->pix_fmt << endl;
    cout << AV_PIX_FMT_YUV420P << endl;

    ret = avcodec_open2(input.pCodecCtx, input.pCodec, nullptr);
    if(ret < 0) Erro("Could not open codec");

    input.pFrame = av_frame_alloc();
    input.pPacket = av_packet_alloc();
    if(!input.pFrame || !input.pPacket) Erro("Could not alloc frame/packet");

    ret = avformat_alloc_output_context2(&output.pFmtCtx, nullptr, AV_OUTPUT_FORMAT, outputName);
    if(ret < 0) Erro("Could not create output context");

    //Find encoder
    output.pCodec = avcodec_find_encoder_by_name(AV_OUTPUT_CODEC);
    if(!output.pCodec) Erro("Codec not found");

    //Create and add new stream
    output.pStream = avformat_new_stream(output.pFmtCtx, output.pCodec);
    if(!output.pStream) Erro("Could not create the output stream");

    //Initialize IO context
    if(!(output.pFmtCtx->flags & AVFMT_NOFILE)) {
        ret = avio_open2(&output.pFmtCtx->pb, outputName, AVIO_FLAG_WRITE, nullptr, nullptr);
        if (ret < 0) Erro("Could not initialize IO context");
    }

    //Alloc codec context
    output.pCodecCtx = avcodec_alloc_context3(output.pCodec);
    if(!output.pCodecCtx) Erro("Could not alloc codec context");

    output.pCodecCtx->codec_tag = 0;
    output.pCodecCtx->codec_type = AVMEDIA_TYPE_VIDEO;
    output.pCodecCtx->pix_fmt = AV_PIX_FMT_YUV420P;
    output.pCodecCtx->codec_id = AV_CODEC_ID_H264;
    output.pCodecCtx->bit_rate = AV_OUTPUT_BITRATE;
    output.pCodecCtx->width = OUTPUT_WIDTH;
    output.pCodecCtx->height = OUTPUT_HEIGHT;
    output.pCodecCtx->time_base.den = 20;
    output.pCodecCtx->time_base.num = 1;
    output.pCodecCtx->thread_count = 16;
    output.pCodecCtx->flags |= CODEC_FLAG_LOOP_FILTER;
    output.pCodecCtx->qmin = 10;
    output.pCodecCtx->qmax = 51;
    output.pCodecCtx->qcompress = 0.6;
    output.pCodecCtx->max_qdiff = 4;
    output.pCodecCtx->i_quant_factor = 0.71;
    output.pCodecCtx->me_range = 16;
    output.pCodecCtx->me_cmp = FF_CMP_CHROMA;
    output.pCodecCtx->ildct_cmp = FF_CMP_CHROMA;
    output.pCodecCtx->gop_size = 250;

    output.pCodecCtx->keyint_min = 25;
    output.pCodecCtx->trellis = 0;
    output.pCodecCtx->me_subpel_quality = 0;
    output.pCodecCtx->max_b_frames = 0;
    output.pCodecCtx->refs = 1;

    if(output.pFmtCtx->flags & AVFMT_GLOBALHEADER) {
        output.pCodecCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }

    //Initialize codec stream
    ret = avcodec_parameters_from_context(output.pStream->codecpar, output.pCodecCtx);
    if(ret < 0) Erro("Could not initialize stream codec");

    //Set open codec configuration
    AVDictionary *opts = nullptr;
    av_dict_set(&opts, "preset", "ultrafast", 0);
    av_dict_set(&opts, "crf", "22", 0);
    av_dict_set(&opts, "tune", "zerolatency", 0);
    av_dict_set(&opts, "profile", "high", 0);

    av_opt_set(output.pCodecCtx->priv_data, "preset", "ultrafast", 0);
    av_opt_set(output.pCodecCtx->priv_data, "crf", "22", 0);

    //Open video encoder
    ret = avcodec_open2(output.pCodecCtx, output.pCodec, &opts);
    if(ret < 0) Erro("Could nt open output codec");

    output.pStream->codecpar->extradata = output.pCodecCtx->extradata;
    output.pStream->codecpar->extradata_size = output.pCodecCtx->extradata_size;

    //Print some information about output
    av_dump_format(output.pFmtCtx, 0, outputName, 1);

    //Initialize converter
    SwsContext *swsctx = sws_alloc_context();
    swsctx = sws_getContext(INPUT_WIDTH, INPUT_HEIGHT, input.pCodecCtx->pix_fmt,
                            OUTPUT_WIDTH, OUTPUT_HEIGHT, AV_PIX_FMT_YUV420P,
                            SWS_BICUBIC,nullptr, nullptr, nullptr);

    //Alloc and initialize frame buffer
    output.pFrame = av_frame_alloc();
    if(!output.pFrame) Erro("Could not alloc frame");

    std::vector<uint8_t> framebuf(av_image_get_buffer_size(output.pCodecCtx->pix_fmt, OUTPUT_WIDTH, OUTPUT_HEIGHT, 1));
    av_image_fill_arrays(output.pFrame->data, output.pFrame->linesize, framebuf.data(), output.pCodecCtx->pix_fmt, OUTPUT_WIDTH, OUTPUT_HEIGHT, 1);

    //Write header
    ret = avformat_write_header(output.pFmtCtx, &opts);
    if(ret < 0) Erro("Fail to write outstream header");

    while(av_read_frame(input.pFmtCtx, input.pPacket) >= 0) {

        if(input.pPacket->size == 0) continue;
        if(input.pPacket->stream_index == videoId) {
            ret = avcodec_send_packet(input.pCodecCtx, input.pPacket);
            if(ret < 0) Erro("Could not send packet to decoder");

            ret = avcodec_receive_frame(input.pCodecCtx, input.pFrame);
            if(ret < 0) Erro("Could not receive frame from decoder");

            //Convert YUV422 to YUV420P
            sws_scale(swsctx, input.pFrame->data, input.pFrame->linesize, 0, OUTPUT_HEIGHT, output.pFrame->data, output.pFrame->linesize);

            //Set frame config
            output.pFrame->width = OUTPUT_WIDTH;
            output.pFrame->height = OUTPUT_HEIGHT;
            output.pFrame->format = AV_PIX_FMT_YUV420P;
            output.pFrame->pts += av_rescale_q(1, output.pCodecCtx->time_base, output.pStream->time_base);

            //Alloc and initialize packet
            output.pPacket = av_packet_alloc();
            av_init_packet(output.pPacket);

            //Send frame to encoder
            ret = avcodec_send_frame(output.pCodecCtx, output.pFrame);
            if(ret < 0) Erro("Error sending frame to codec context");

            //Receive packet from encoder
            ret = avcodec_receive_packet(output.pCodecCtx, output.pPacket);
            if(ret < 0) Erro("Error receiving packet from codec context");

            //Write frame
            av_interleaved_write_frame(output.pFmtCtx, output.pPacket);
            av_packet_unref(output.pPacket);
            av_packet_unref(input.pPacket);
        }
    }

    return 0;
}

