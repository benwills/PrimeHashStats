#include <inttypes.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <math.h>
#include <limits.h>
#include <immintrin.h>
#include <string.h>
#include <stdarg.h>

#include "PrimeStats.h"
#include "PrimeStats.Util.h"

//
// keys are mapped to a file. that file is a constant string of keys,
// not separated by a newline.
//
// based on tests, that may or may not be faster. (appears only slightly faster)
//
// however, it does significantly reduce memory usage since the data
// is memory mapped.
//


//==============================================================================
static char* file_out_data   = NULL;
static char* key_files_dir   = NULL;
static char* prime_files_dir = NULL;


//------------------------------------------------------------------------------
typedef struct PrimeStats_KeyFileKeys_st {
	      char     filePath[1024];
	      off_t    fileSize;
	const void*    keys;
	      uint64_t keyCnt;
	      int      keyLen;
} PrimeStats_KeyFileKeys_st;

typedef struct PrimeStats_KeyFiles_st {
	PrimeStats_KeyFileKeys_st keyFile[8];
	int                       keyLenMax;
	int                       keyFilesCnt;
} PrimeStats_KeyFiles_st;

const char* keyFileNames[] = {
	"toks.len.sequential.1.64.txt",
	"toks.len.sequential.2.3929.txt",
	"toks.len.sequential.3.201228.txt",
	"toks.len.sequential.4.2429953.txt",
	"toks.len.sequential.5.12629812.txt",
	"toks.len.sequential.6.18510142.txt",
	"toks.len.sequential.7.15311088.txt",
	"toks.len.sequential.8.18679078.txt",
};

//------------------------------------------------------------------------------
bool
keyFileInit(PrimeStats_KeyFiles_st* keyFiles, const char* dirName, const char* fileName)
{
	int      keyLen = 0;
	uint64_t keyCnt = 0;
	sscanf(fileName, "toks.len.sequential.%d.%"PRIu64".txt", &keyLen, &keyCnt);

	int keyLenMax = sizeof(keyFiles->keyFile) / sizeof(keyFiles->keyFile[0]);
	if (keyLen > keyLenMax) { return false; }

	int keyFileIdx = keyLen - 1;
	sprintf(keyFiles->keyFile[keyFileIdx].filePath, "%s%s", dirName, fileName);

	keyFiles->keyFile[keyFileIdx].keyLen = keyLen;
	keyFiles->keyFile[keyFileIdx].keyCnt = keyCnt;

	keyFiles->keyFile[keyFileIdx].keys
		= mmapFileToPtr( keyFiles->keyFile[keyFileIdx].filePath,
		                &keyFiles->keyFile[keyFileIdx].fileSize);

	return true;
}

//------------------------------------------------------------------------------
void
printHelpAndExit()
{
	printf(
		"\n"
		"primegen: find and output prime numbers to a binary file\n"
		"options:\n"
		"\n\t" "-h: help"
		"\n\t" "-o: output file : file_out_data"
		"\n\t" "-k: keys dir    : key_files_dir"
		"\n\t" "-p: primes file : prime_files_dir"
		"\n\n"
		"eg:\n"
		"\n./PrimeStats.main"
		"\n\t-o \"./PrimeStats.8.data\" "
		"\n\t-k \"/media/src/o/libo/src/Hash/\" "
		"\n\t-p \"/media/src/c/primesieve/out.primes2.128.64/8/\""
		"\n\n"
	);
	exit(1);
}

void
cliOptsToCfg(int argc, char *argv[])
{
  int  opt;
  bool hasErr = false;

  while ((opt = getopt(argc, argv, ":h:o:k:p:")) != -1)
  {
    switch(opt)
    {
			case 'h':
		    printHelpAndExit();
		    break;
			case 'o':
		    file_out_data = optarg;
		    break;
			case 'k':
		    key_files_dir = optarg;
		    break;
			case 'p':
		    prime_files_dir = optarg;
		    break;
			default:
				hasErr = true;
		    break;
    }
  }

  // other unparsed options
  for(; optind < argc; optind++){
    hasErr = true;
  }

	if (file_out_data  == NULL) {
		hasErr = true;
	}
	if (key_files_dir  == NULL) {
		hasErr = true;
	}
	if (prime_files_dir == NULL) {
		hasErr = true;
	}

  if (hasErr) {
  	printf("invalid options given.\n");
    printHelpAndExit();
  }
}

void
printCfg()
{
	printf("using config:\n");
	printf("\tfile_out_data  : %s\n", file_out_data);
	printf("\tkey_files_dir  : %s\n", key_files_dir);
	printf("\tprime_files_dir : %s\n", prime_files_dir);
	printf("\n");
	fflush(stdout);
}


//==============================================================================
int
main(int argc, char *argv[])
{
	cliOptsToCfg(argc, argv);
	printCfg();

  DIR *dPrimes;
  struct dirent *dirPrimes;
  dPrimes = opendir(prime_files_dir);
  if (!dPrimes) {
  	printf("couldn't open prime files dir\n");
  	exit(1);
  }

  PrimeStats_KeyFiles_st keyFiles = {0};
  keyFiles.keyLenMax   = 8;
  keyFiles.keyFilesCnt = 8;
  for (int i = 0; i < keyFiles.keyFilesCnt; ++i) {
	  keyFileInit(&keyFiles, key_files_dir, keyFileNames[i]);
  }
  const int maxKeysPerFile = 10000;
  const uint64_t keysTotal
  	= (keyFiles.keyFile[0].keyCnt > maxKeysPerFile
  	   ? maxKeysPerFile : keyFiles.keyFile[0].keyCnt)
  	+ (keyFiles.keyFile[1].keyCnt > maxKeysPerFile
  	   ? maxKeysPerFile : keyFiles.keyFile[1].keyCnt)
  	+ (keyFiles.keyFile[2].keyCnt > maxKeysPerFile
  	   ? maxKeysPerFile : keyFiles.keyFile[2].keyCnt)
  	+ (keyFiles.keyFile[3].keyCnt > maxKeysPerFile
  	   ? maxKeysPerFile : keyFiles.keyFile[3].keyCnt)
  	+ (keyFiles.keyFile[4].keyCnt > maxKeysPerFile
  	   ? maxKeysPerFile : keyFiles.keyFile[4].keyCnt)
  	+ (keyFiles.keyFile[5].keyCnt > maxKeysPerFile
  	   ? maxKeysPerFile : keyFiles.keyFile[5].keyCnt)
  	+ (keyFiles.keyFile[6].keyCnt > maxKeysPerFile
  	   ? maxKeysPerFile : keyFiles.keyFile[6].keyCnt)
  	+ (keyFiles.keyFile[7].keyCnt > maxKeysPerFile
  	   ? maxKeysPerFile : keyFiles.keyFile[7].keyCnt);

  //--------------------------------------------------------------------
        int statsCnt      = 0;
  const int statsBatchCnt = 4096;

  PrimeStats_st* statsArr = calloc(statsBatchCnt, sizeof(*statsArr));
  while ((dirPrimes = readdir(dPrimes)) != NULL)
  {
  	if (strlen(dirPrimes->d_name) < 10) { continue; }

  	char primesFilePath[1024] = {0};

  	sprintf(primesFilePath, "%s%s", prime_files_dir, dirPrimes->d_name);
    printf("\nfilename: %s\n", dirPrimes->d_name); fflush(stdout);

		uint64_t primesFileBytes = 0;
		const void* primesArr = mmapFileToPtr(primesFilePath, &primesFileBytes);

	  uint64_t* primeMap  = (uint64_t*)primesArr;
	  const int numPrimes = primesFileBytes / sizeof(*primeMap);

	  //------------------------------------------------------
	  struct timespec timeStart = timerStart();

	  int timerIterCnt = 0;
	  for (int iPrime = 0; iPrime < numPrimes; iPrime++)
	  {
	    //--------------------------------
	  	timerIterCnt++;
	  	const int statsArrIdx = statsCnt % statsBatchCnt;
	    PrimeStats_st* stats = &statsArr[statsArrIdx];
	    PrimeStats_Init(stats, primeMap[iPrime]);

	    //--------------------------------
	    for (int iKeyFile = 0; iKeyFile < keyFiles.keyFilesCnt; iKeyFile++) {
				PrimeStats_RunKeys(stats,
				                   keyFiles.keyFile[iKeyFile].keys,
				                   keyFiles.keyFile[iKeyFile].keyLen,
				                   keyFiles.keyFile[iKeyFile].keyCnt,
				                   maxKeysPerFile);
	    }

			PrimeStatsMeta_Calc(stats);

	    //--------------------------------
	    if (statsArrIdx == (statsBatchCnt - 1))
	    {
			  uint64_t timeDiff = timerEnd(timeStart);

			  printf("\ntimerIterCnt : %d", timerIterCnt);
			  printf("\ntime taken   : ");
			  printU64WithCommas(timeDiff);
			  uint64_t nsPerPrime = timeDiff / (uint64_t)timerIterCnt;
			  printf("\nns Per Prime : ");
			  printU64WithCommas(nsPerPrime);
			  printf("\n");

	    	fileAppendBytes(file_out_data, statsArr,
	    	                statsBatchCnt * sizeof(*statsArr));
			  printf("\n");

			  memset(statsArr, 0, statsBatchCnt * sizeof(*statsArr));

			  timerIterCnt = 0;
			  timeStart = timerStart();
	    }

	    statsCnt++;
	  }
	  //------------------------------------------------------
  }

  closedir(dPrimes);
	exit(1);
}
