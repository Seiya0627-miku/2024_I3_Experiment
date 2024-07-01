/*
 gcc -o webcam_client webcam_client.c -lavcodec -lavformat -lavutil -lswscale -lSDL2
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

int main(int argc, char** argv) {
	int s = socket(PF_INET, SOCK_STREAM, 0);
	if (s == -1) {
		die("socket");
	}

	struct sockaddr_in addr;
	addr.sin_family = AF_INET; // declare the protocol of the following address (IPv4)
	// addr.sin_addr.s_addr = inet_addr("192.169.1.100");
	if (inet_aton(argv[1], &addr.sin_addr) == 0) die("inet_aton"); // set target ip address
	addr.sin_port = htons(atoi(argv[2])); // set target port
	int ret = connect(s, (struct sockaddr *)&addr, sizeof(addr)); // connect
	if (ret == -1) die("connect");

    // SDL initialization
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
        fprintf(stderr, "Could not initialize SDL - %s\n", SDL_GetError());
        return -1;
    }

    SDL_Window *window = SDL_CreateWindow("EEIC Client Webcam", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WIDTH * SCALE, HEIGHT * SCALE, SDL_WINDOW_SHOWN);
    if (!window) {
        fprintf(stderr, "SDL: could not create window - exiting:%s\n", SDL_GetError());
        return -1;
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, 0);
    if (!renderer) {
        fprintf(stderr, "SDL: could not create renderer - exiting:%s\n", SDL_GetError());
        return -1;
    }

    SDL_Texture *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_YV12, SDL_TEXTUREACCESS_STREAMING, WIDTH, HEIGHT);
    if (!texture) {
        fprintf(stderr, "SDL: could not create texture - exiting:%s\n", SDL_GetError());
        return -1;
    }

    SDL_Rect rect;
    rect.x = 0;
    rect.y = 0;
    rect.w = WIDTH * SCALE;
    rect.h = HEIGHT * SCALE;

    int frame_size = WIDTH * HEIGHT * 3 / 2; // YUV420P
    uint8_t *frame_data = (uint8_t *)malloc(frame_size);

    while (1) {
        ssize_t total_received = 0;
        while (total_received < frame_size) {
            ssize_t bytes_received = read(s, frame_data + total_received, frame_size - total_received);
            if (bytes_received < 0) {
                die("read failed");
            }
            total_received += bytes_received;
        }

        SDL_UpdateYUVTexture(texture, &rect, frame_data, WIDTH, frame_data + WIDTH * HEIGHT, WIDTH / 2, frame_data + WIDTH * HEIGHT * 5 / 4, WIDTH / 2);
        SDL_RenderClear(renderer);
		SDL_RenderCopyEx(renderer, texture, NULL, &rect, 0, NULL, SDL_FLIP_HORIZONTAL);
        SDL_RenderPresent(renderer);

        SDL_Event event;
        SDL_PollEvent(&event);
        if (event.type == SDL_QUIT) {
            break;
        }
    }

    free(frame_data);
    close(s);

    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
