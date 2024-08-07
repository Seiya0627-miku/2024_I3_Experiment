#include <assert.h>
#include <complex.h>
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void _die(char * s) {
  perror(s); 
  exit(1);
}

/* 標本(整数)を複素数へ変換 */
void sample_to_complex(short * s, 
		       complex double * X, 
		       long n) {
  long i;
  for (i = 0; i < n; i++) X[i] = s[i];
}

/* 複素数を標本(整数)へ変換. 虚数部分は無視 */
void complex_to_sample(complex double * X, 
		       short * s, 
		       long n) {
  long i;
  for (i = 0; i < n; i++) {
    s[i] = creal(X[i]);
  }
}

/* 高速(逆)フーリエ変換;
   w は1のn乗根.
   フーリエ変換の場合   偏角 -2 pi / n
   逆フーリエ変換の場合 偏角  2 pi / n
   xが入力でyが出力.
   xも破壊される
 */
void fft_r(complex double * x, 
	   complex double * y, 
	   long n, 
	   complex double w) {
  if (n == 1) { y[0] = x[0]; }
  else {
    complex double W = 1.0; 
    long i;
    for (i = 0; i < n/2; i++) {
      y[i]     =     (x[i] + x[i+n/2]); /* 偶数行 */
      y[i+n/2] = W * (x[i] - x[i+n/2]); /* 奇数行 */
      W *= w;
    }
    fft_r(y,     x,     n/2, w * w);
    fft_r(y+n/2, x+n/2, n/2, w * w);
    for (i = 0; i < n/2; i++) {
      y[2*i]   = x[i];
      y[2*i+1] = x[i+n/2];
    }
  }
}

void fft(complex double * x, 
	 complex double * y, 
	 long n) {
  long i;
  double arg = 2.0 * M_PI / n;
  complex double w = cos(arg) - 1.0j * sin(arg);
  fft_r(x, y, n, w);
  for (i = 0; i < n; i++) y[i] /= n;
}

void ifft(complex double * y, 
	  complex double * x, 
	  long n) {
  double arg = 2.0 * M_PI / n;
  complex double w = cos(arg) + 1.0j * sin(arg);
  fft_r(y, x, n, w);
}

int pow2check(long N) {
  long n = N;
  while (n > 1) {
    if (n % 2) return 0;
    n = n / 2;
  }
  return 1;
}

void print_complex(FILE * wp, 
		   complex double * Y, long n) {
  long i;
  for (i = 0; i < n; i++) {
    fprintf(wp, "%ld %f %f %f %f\n", 
	    i, 
	    creal(Y[i]), cimag(Y[i]),
	    cabs(Y[i]), atan2(cimag(Y[i]), creal(Y[i])));
  }
}


void voice_changer(long n, short *buf, int offset_freq) {
	if (!pow2check(n)) {
    	fprintf(stderr, "error : n (%ld) not a power of two\n", n);
    	exit(1);
  	}
  	FILE * wp = fopen("fft.dat", "wb");
  	if (wp == NULL) _die("fopen");
  	complex double * X = calloc(sizeof(complex double), n);
  	complex double * Y = calloc(sizeof(complex double), n);
  
  	int offset = (double) offset_freq / 44100 * (n/2);
    /* 標準入力からn個標本を読む */
    /* 複素数の配列に変換 */
    sample_to_complex(buf, X, n);
    /* FFT -> Y */
    fft(X, Y, n);
    
    print_complex(wp, Y, n);
    fprintf(wp, "----------------\n");

    if (offset >= 0) {
    	for (int i=n/2-1; i>0; --i) {
    	    int new = i - offset;
	    	if (new >= 0 && new < n/2) {
	        	Y[i] = Y[new];
	        	Y[n-i] = Y[n-new];
	    	}
	    	else {
	        	Y[i] = 0;
	        	Y[n-i] = 0;
	    	}
		}
    }
    else {
    	for (int i=0; i<n/2; ++i) {
    	    int new = i - offset;
	    	if (new >= 0 && new < n/2) {
	        	Y[i] = Y[new];
	        	Y[n-i] = Y[n-new];
	    	}
	    	else {
	        	Y[i] = 0;
	        	Y[n-i] = 0;
	    	}
		}
    }

    /* IFFT -> Z */
    ifft(Y, X, n);
    /* 標本の配列に変換 */
    complex_to_sample(X, buf, n);
    /* 標準出力へ出力 */
  	fclose(wp);
}
