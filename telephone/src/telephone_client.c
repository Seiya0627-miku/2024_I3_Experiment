// serv_send_sound.c
// usage:
// ./serv_send <port>

#include<sys/types.h>
#include<sys/stat.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<netinet/in.h>
#include<netinet/ip.h>
#include<netinet/tcp.h>
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<fcntl.h>
#include"bandpass.h"
#include"voice_changer.h"

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
	
	int N = 8092;
	
	// Record
	FILE *fp = popen("rec -t raw -b 16 -c 1 -e s -r 44100 -", "r");
	short buf[N];
	
	// Play
	FILE *fp2 = popen("play -t raw -b 16 -c 1 -e s -r 44100 -", "w");
	short buf2[N];
	
	while (1) {
		int n = fread(buf, sizeof(short), N, fp);
		if (n == 0) continue;
		voice_changer(N, buf, 1000);
		bandpass(N, buf, 100, 4000);
		send(s, buf, n*sizeof(short), 0);
		
		n = recv(s, buf2, N*sizeof(short) , 0);
		if (n == 0) continue;
		fwrite(buf2, sizeof(short), n/sizeof(short), fp2);
	}
	close(s);
	pclose(fp);
	pclose(fp2);
	
	return 0;
}
