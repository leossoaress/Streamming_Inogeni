#include <iostream>
#include <string>

extern "C" {
#include <libavdevice/avdevice.h>
#include <libavfilter/avfilter.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
}

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

using namespace std;
using namespace cv;


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
    avdevice_register_all();
    avformat_network_init();

    ff_input_t input;
    //ff_output_t output;

    //int videoId = -1;
    int ret = 0;

    input.pInputFmt = av_find_input_format("v4l2");
    if(!input.pInputFmt) Erro("Input format could not be found");

    input.pFmtCtx = avformat_alloc_context();
    if(!input.pFmtCtx) Erro("Could not alloc input context");

    ret = avformat_open_input(&input.pFmtCtx, "/dev/video0", input.pInputFmt, nullptr);
    if(ret < 0) Erro("Could not open input");

    AVCodecParameters *pCodecAux = nullptr;

    for(unsigned int i = 0; i < input.pFmtCtx->nb_streams; ++i) {
        pCodecAux = input.pFmtCtx->streams[i]->codecpar;
        if(pCodecAux->codec_type == AVMEDIA_TYPE_VIDEO) {
            input.pCodec = avcodec_find_decoder(pCodecAux->codec_id);
            //videoId = i;
            break;
        }
    }

    input.pCodecCtx = avcodec_alloc_context3(input.pCodec);
    if(!input.pCodecCtx) Erro("Could not alloc input codec context");

    ret = avcodec_parameters_to_context(input.pCodecCtx, pCodecAux);
    if(ret < 0) Erro("Could not copy parameters to context");

    ret = avcodec_open2(input.pCodecCtx, input.pCodec, nullptr);
    if(ret < 0) Erro("Could not open codec");

    return 0;
}