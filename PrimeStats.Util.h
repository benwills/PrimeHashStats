#ifndef _PrimeStats_Util_h_
#define _PrimeStats_Util_h_

#include <sys/stat.h>
#include <sys/mman.h>
#include <stddef.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
#include <time.h>


//------------------------------------------------------------------------------
const void*
mmapFileToPtr(const char* filename, off_t* size)
{
  int fd;
  struct stat s;
  int status;
  const void* mapped;

  fd = open(filename, O_RDONLY);
  if (fd < 0) {
  	printf("open failed: %s\n", strerror(errno));
  	exit(1);
  }

  status = fstat(fd, &s);
  if (status < 0) {
  	printf("fstat failed: %s\n", strerror(errno));
  	exit(1);
  }
  *size = s.st_size;

  mapped = mmap(0, *size, PROT_READ, MAP_PRIVATE, fd, 0);
  if (mapped == MAP_FAILED) {
  	printf("mmap failed: %s\n", strerror(errno));
  	exit(1);
  }

	madvise((void*)mapped, *size, MADV_SEQUENTIAL);

  return mapped;
}

//------------------------------------------------------------------------------
struct timespec timerStart() {
  struct timespec start_time;
  clock_gettime(CLOCK_MONOTONIC, &start_time);
  return start_time;
}

uint64_t timerEnd(struct timespec start_time) {
  struct timespec end_time;
  clock_gettime(CLOCK_MONOTONIC, &end_time);
  uint64_t diffInNanos = (end_time.tv_sec - start_time.tv_sec)
                   * (uint64_t)1e9
                   + (end_time.tv_nsec - start_time.tv_nsec);
  return diffInNanos;
}

//------------------------------------------------------------------------------
void printU64WithCommas(uint64_t ns)
{
	char str[32] = {0};
	char* pos = str + sizeof(str) - 2; // leave a null byte
	int digits = 0;
	while (ns) {
		digits++;
		*pos = (ns%10)+'0';
		pos--;
		ns /= 10;
		if (0 == (digits % 3)) {
			*pos = ',';
			pos--;
		}
	};
	++pos;
	if (*pos == ',') {
		++pos;
	}
	printf("%s", pos);
}


//------------------------------------------------------------------------------
void
fileAppendBytes(char* filename, void* buf, int bufLen)
{
  FILE* fp;
  fp = fopen(filename, "a+");
  if (fp == NULL) {
    fprintf(stderr, "\nError opened file\n");
    exit(1);
  }
  size_t nElemsWritten = fwrite(buf, bufLen, 1, fp);
  if (nElemsWritten == 1) {
    printf("fileAppendBytes(): OK\n");
	  fflush(stdout);
	}
  else {
    printf("fileAppendBytes(): FATAL: wrote [%d] bytes\n", (int)nElemsWritten);
    exit(1);
  }
  fclose(fp);
}


#endif // _PrimeStats_Util_h_
