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

#define  FILE_OUT_DATA "./PrimeStats.data"




//==============================================================================
int
main()
{
	off_t fileSize = 0;
	const PrimeStats_st* stats = mmapFileToPtr(FILE_OUT_DATA, &fileSize);
	const int statsCnt = fileSize / sizeof(*stats);

	printf("fileSize: %ld\n", fileSize);
	printf("statsCnt: %d\n", statsCnt);
	printf("\n\n");
	fflush(stdout);

	int outCnt = 100;
	int loopCnt = 0;
	for (int i = 0; i < statsCnt; i++)
	{
		loopCnt++;

		// for (int l = 2; l < 8; l++) {
		// 	if (   stats->meta.ava[l].pop.avg >= 27
		// 	    && stats->meta.ava[l].pop.avg <= 35) {
		// 		PrimeStatsMeta_PrintChart(&stats[i]);
		// 		if (--outCnt == 0) {
		// 			goto FIN;
		// 		}
		// 		break;
		// 	}
		// }

		for (int l = 6; l < 8; l++) {
			if (   stats->meta.ava[l].pop.avg >= 15
			    && stats->meta.ava[l].pop.avg <= 35) {
				PrimeStatsMeta_PrintChart(&stats[i]);
				if (--outCnt == 0) {
					goto FIN;
				}
				break;
			}
		}
	}

	FIN:;
	printf("loopCnt: %d\n", loopCnt);

	return 1;
}
