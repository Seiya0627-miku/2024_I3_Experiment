/*

 gcc -o webcam_server webcam_server.c -lavcodec -lavformat -lavutil -lswscale -lavdevice -lSDL2

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
#include <libswscale/swscale.h>
#include <libavdevice/avdevice.h>
#include <SDL2/SDL.h>

#include<sys/types.h>
#include<sys/stat.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<netinet/in.h>
#include<netinet/ip.h>
#include<netinet/tcp.h>

#define WIDTH 640
#define HEIGHT 480
#define SCALE 1

void die(char *s) {
	perror(s);
	exit(1);
}

int main(int argc, char **argv) {
    // Set camera variables
    AVFormatContext *pFormatCtx = NULL;
    int videoStream, i;
    AVCodecContext *pCodecCtx = NULL;
    AVCodec *pCodec = NULL;
    AVFrame *pFrame = NULL;
    AVPacket packet;
    struct SwsContext *sws_ctx = NULL;
    SDL_Window *window = NULL;
    SDL_Renderer *renderer = NULL;
    SDL_Texture *texture = NULL;
    SDL_Rect rect;
    int ret;

    avdevice_register_all();

    // Open video device
    AVInputFormat *inputFormat = av_find_input_format("video4linux2");
    if (avformat_open_input(&pFormatCtx, "/dev/video0", inputFormat, NULL) != 0)	die("avformat_open_input");

    // Retrieve stream information
    if (avformat_find_stream_info(pFormatCtx, NULL) < 0) die("avformat_find_stream_info");

    // Find the first video stream
    videoStream = -1;
    for (i = 0; i < pFormatCtx->nb_streams; i++) {
        if (pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoStream = i;
            break;
        }
    }
    if (videoStream == -1) die("Could not find a video stream\n");

    // Get a pointer to the codec context for the video stream
    pCodecCtx = avcodec_alloc_context3(NULL);
    if (!pCodecCtx) die("avcodec_alloc_context3");
    avcodec_parameters_to_context(pCodecCtx, pFormatCtx->streams[videoStream]->codecpar);

    // Find decoder
    pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
    if (pCodec == NULL) die("avcodec_find_decoder");

    // Open codec
    if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0) die("avcodec_open2");

    // Allocate video frame
    pFrame = av_frame_alloc();
    if (!pFrame) die("av_frame_alloc");

    // SDL initialization
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
        fprintf(stderr, "Could not initialize SDL - %s\n", SDL_GetError());
        return -1;
    }

    // Create window
    window = SDL_CreateWindow("EEIC Server Webcam", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WIDTH * SCALE, HEIGHT * SCALE, SDL_WINDOW_SHOWN);
    if (!window) {
        fprintf(stderr, "SDL: could not create window - exiting:%s\n", SDL_GetError());
        return -1;
    }

    // Create renderer
    renderer = SDL_CreateRenderer(window, -1, 0);
    if (!renderer) {
        fprintf(stderr, "SDL: could not create renderer - exiting:%s\n", SDL_GetError());
        return -1;
    }

    // Create texture
    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_YV12, SDL_TEXTUREACCESS_STREAMING, WIDTH, HEIGHT);
    if (!texture) {
        fprintf(stderr, "SDL: could not create texture - exiting:%s\n", SDL_GetError());
        return -1;
    }

    // Set up rect for texture rendering
    rect.x = 0;
    rect.y = 0;
    rect.w = WIDTH * SCALE;
    rect.h = HEIGHT * SCALE;

    // Initialize SWS context for software scaling
    sws_ctx = sws_getContext(WIDTH, HEIGHT, pCodecCtx->pix_fmt, WIDTH, HEIGHT, AV_PIX_FMT_YUV420P, SWS_BILINEAR, NULL, NULL, NULL);
    
    // Sending video
	int ss = socket(PF_INET, SOCK_STREAM, 0);

	struct sockaddr_in addr;
	addr.sin_family = PF_INET;
	addr.sin_port = htons(atoi(argv[1]));
	addr.sin_addr.s_addr = INADDR_ANY;
	bind(ss, (struct sockaddr *)&addr, sizeof(addr));
	
	listen(ss, 10);
	
	struct sockaddr_in client_addr;
	socklen_t len = sizeof(struct sockaddr_in);
	int s = accept(ss, (struct sockaddr *)&client_addr, &len);
	if (s == -1) die("accept");
	close(ss); 
    
    int F = 0;
    // Read frames and display
    while (av_read_frame(pFormatCtx, &packet) >= 0) {
        // Is this a packet from the video stream?
        if (packet.stream_index == videoStream) {
            // Decode video frame
            ret = avcodec_send_packet(pCodecCtx, &packet); // Send packet for decoding
            if (ret < 0) {
                fprintf(stderr, "Error sending packet for decoding\n");
                continue;
            }

            while (ret >= 0) {
                ret = avcodec_receive_frame(pCodecCtx, pFrame); // Decode
                if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                    break;
                } else if (ret < 0) {
                    fprintf(stderr, "Error during decoding\n");
                    break;
                }

            	
            	// Send raw frame out
            	int frame_size = pFrame->linesize[0] * HEIGHT + pFrame->linesize[1] * (HEIGHT / 2) + pFrame->linesize[2] * (HEIGHT / 2);
            	write(s, pFrame->data[0], pFrame->linesize[0] * HEIGHT);
            	write(s, pFrame->data[1], pFrame->linesize[1] * (HEIGHT * 2));
            	write(s, pFrame->data[2], pFrame->linesize[2] * (HEIGHT * 2));                
            	if (F == 0) {
            		printf("0: %d\n",pFrame->linesize[0]);
            		printf("1: %d\n",pFrame->linesize[1]);
            		printf("2: %d\n",pFrame->linesize[2]);
            		F = 1;
            	}
            	
            	// Show self camera
                // Allocate an AVFrame structure for YUV conversion
                AVFrame *pFrameYUV = av_frame_alloc();
                if (!pFrameYUV) {
                    fprintf(stderr, "Could not allocate frame\n");
                    break;
                }
                av_image_alloc(pFrameYUV->data, pFrameYUV->linesize, WIDTH*SCALE, HEIGHT*SCALE, AV_PIX_FMT_YUV420P, 32);

                // Convert the image from its native format to YUV
                sws_scale(sws_ctx, (uint8_t const * const *)pFrame->data, pFrame->linesize, 0, HEIGHT, pFrameYUV->data, pFrameYUV->linesize);

                // Update SDL texture with YUV data
                SDL_UpdateYUVTexture(texture, &rect, pFrameYUV->data[0], pFrameYUV->linesize[0], pFrameYUV->data[1], pFrameYUV->linesize[1], pFrameYUV->data[2], pFrameYUV->linesize[2]);

                // Render texture to screen
                SDL_RenderClear(renderer);
                SDL_RenderCopyEx(renderer, texture, NULL, &rect, 0, NULL, SDL_FLIP_HORIZONTAL);  // Apply horizontal flip
                SDL_RenderPresent(renderer);

                // Free the YUV frame
                av_freep(&pFrameYUV->data[0]);
                av_frame_free(&pFrameYUV);
            }
        }
        av_packet_unref(&packet);

        // Handle SDL events
        SDL_Event event;
        SDL_PollEvent(&event);
        if (event.type == SDL_QUIT) {
            break;
        }
    }

    // Free YUV frame
    av_frame_free(&pFrame);

    // Close codecs and socket
    close(s);
    avcodec_close(pCodecCtx);
    avcodec_free_context(&pCodecCtx);

    // Close video file
    avformat_close_input(&pFormatCtx);

    // Destroy SDL objects
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}

