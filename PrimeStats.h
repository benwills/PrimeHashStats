#ifndef _PrimeStats_h_
#define _PrimeStats_h_

#include <inttypes.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#define PS_KEYLEN_MAX 8


#define DBG_FFL {printf("DBG: File:[%s] Func:[%s] Line:[%d]\n",\
                 __FILE__, __FUNCTION__, __LINE__);fflush(stdout);}


static const int _keyLenMin = 1;
static const int _keyLenMax = PS_KEYLEN_MAX;




//------------------------------------------------------------------------------
typedef struct PrimeStats_BitsCnt_st {
	uint32_t bit[64]; // bit-set accumulator
	uint32_t pop[64]; // popcnt
	uint32_t valCnt;
} PrimeStats_BitsCnt_st;

typedef struct PrimeStats_BitsCntMeta_st {
	uint32_t cnt; // from valCnt
	struct {
		uint32_t min;
		uint32_t max;
		uint32_t gap; // max - min
		uint32_t sum;
		uint32_t avg;
	} bit;
	struct {
		uint32_t min;
		uint32_t max;
		uint32_t gap; // max - min
		uint32_t sum;
		uint32_t avg;
	} pop;
} PrimeStats_BitsCntMeta_st;

typedef struct PrimeStats_Data_st {
	PrimeStats_BitsCnt_st bits[PS_KEYLEN_MAX]; // hash bits accumulator
	PrimeStats_BitsCnt_st ava [PS_KEYLEN_MAX]; // avalanching bits stats
} PrimeStats_Data_st;

typedef struct PrimeStats_Meta_st {
  PrimeStats_BitsCntMeta_st bits[PS_KEYLEN_MAX];
  PrimeStats_BitsCntMeta_st ava [PS_KEYLEN_MAX];
} PrimeStats_Meta_st;

typedef struct PrimeStats_st {
	uint64_t           prime;
	PrimeStats_Data_st data;
	PrimeStats_Meta_st meta;
} PrimeStats_st;


//------------------------------------------------------------------------------
uint64_t
_keyTo64b(const uint8_t* key, const int len) {
  uint64_t k = 0;
  for (int i = len; i; i--) {
    k <<= 8;
    k |= key[i - 1];
  }
	return k;
}

uint64_t
_key64bHash(const uint64_t prime, const uint64_t key) {
	return key * prime;
}

//------------------------------------------------------------------------------
void
_bitsCntTest(PrimeStats_BitsCnt_st* bits, uint64_t val) {
	bits->valCnt++;

	const int popCnt = __builtin_popcountll(val);
	bits->pop[popCnt]++;

	uint64_t mask = 1;
	for (int i = 0; i < 64; i++) {
		bits->bit[i] += val & mask;
		val >>= 1;
	}
}

//------------------------------------------------------------------------------
int 
_avaTestPair(const uint64_t h1, const uint64_t h2) {
	return h1 ^ h2;
}

void 
_avaUpdate(PrimeStats_BitsCnt_st* ava, const uint64_t h1, const uint64_t h2) {
	const int bitDiff = _avaTestPair(h1, h2);
	_bitsCntTest(ava, bitDiff);
}

void
_avaTest(PrimeStats_BitsCnt_st* ava,
         const uint64_t hashIni,   const uint64_t prime,
         const uint64_t key64bIni, const int keyLen)
{
	uint64_t mask = 1;
	const int keyBits = keyLen * 8; // 8 = bits per byte
	for (int i = 0; i < keyBits; i++)
	{
		const uint64_t key64bNew = key64bIni ^ mask;
		// if (0 == key64bNew) {
		// 	// should only happen on certain single-byte keys
		// 	continue;
		// }

		const uint64_t hashNew = _key64bHash(prime, key64bNew);
		_avaUpdate(ava, hashIni, hashNew);
		mask <<= 1;
	}
}

//------------------------------------------------------------------------------
void
_statsAddKey(PrimeStats_st* stats, const uint8_t* key, const int keyLen)
{
	const uint64_t key64b = _keyTo64b  (key,          keyLen);
	const uint64_t hash   = _key64bHash(stats->prime, key64b);
	const int      idx    = keyLen - 1;
	_bitsCntTest(&stats->data.bits[idx], hash);
	_avaTest    (&stats->data.ava [idx],  hash, stats->prime, key64b, keyLen);
}




//------------------------------------------------------------------------------
void
BitsCntMeta_Calc(const PrimeStats_BitsCnt_st*     bits,
                       PrimeStats_BitsCntMeta_st* meta)
{
	meta->cnt = bits->valCnt;
	//------------
	meta->bit.min = UINT32_MAX;
	meta->bit.max = 0;
	meta->bit.gap = 0;
	meta->bit.sum = 0;
	meta->bit.avg = 0;
	for (int i = 0; i < 64; i++) {
		if (bits->bit[i] < meta->bit.min) {
			meta->bit.min = bits->bit[i];
		} else if (bits->bit[i] > meta->bit.max) {
			meta->bit.max = bits->bit[i];
		}
		meta->bit.sum += bits->bit[i];
	}
	meta->bit.avg = meta->bit.sum / meta->cnt;
	meta->bit.gap = meta->bit.max - meta->bit.min;
	//------------
	meta->pop.min = UINT32_MAX;
	meta->pop.max = 0;
	meta->pop.gap = 0;
	meta->pop.sum = 0;
	meta->pop.avg = 0;
	for (int i = 0; i < 64; i++) {
		if (bits->pop[i] > 0) {
			if (i < meta->pop.min) {
				meta->pop.min = i;
			} else if (i > meta->pop.max) {
				meta->pop.max = i;
			}
		}
		meta->pop.sum += (bits->pop[i] * (i+1)); // note the multiply here
	}
	meta->pop.avg = meta->pop.sum / meta->cnt;
	meta->pop.gap = meta->pop.max - meta->pop.min;
}

//------------------------------------------------------------------------------
void
PrimeStatsMeta_Calc(PrimeStats_st* stats)
{
	for (int i = _keyLenMin; i <= _keyLenMax; i++) {
		const int idx = i - 1;
		BitsCntMeta_Calc(&stats->data.bits[idx], &stats->meta.bits[idx]);
		BitsCntMeta_Calc(&stats->data.ava [idx], &stats->meta.ava [idx]);
	}
}

//------------------------------------------------------------------------------
void
PrimeStats_RunKeys(PrimeStats_st* stats,
                   const void*    keys,
                   const uint64_t keyLen,
                   const uint64_t keysCnt,
                   const uint64_t maxKeys)
{
	int iters = maxKeys;
	if (keysCnt < maxKeys) {
		iters = keysCnt;
	}
	const uint8_t* pos = keys;
	for (int i = 0; i < iters; ++i) {
		_statsAddKey(stats, pos, keyLen);
    pos += keyLen;
  }
}

//------------------------------------------------------------------------------
void
PrimeStats_Init(PrimeStats_st* stats, const uint64_t prime)
{
  memset(stats, 0, sizeof(*stats));
  stats->prime = prime;
}




//------------------------------------------------------------------------------
void
BitsCntMeta_PrintChart(const PrimeStats_BitsCntMeta_st* meta) {
	printf("\t\t\t.cnt : %"PRIu32"\n", meta->cnt);
	printf("\t\t\t.bit :\n");
	printf("\t\t\t\t.min : %"PRIu32"\n", meta->bit.min);
	printf("\t\t\t\t.max : %"PRIu32"\n", meta->bit.max);
	printf("\t\t\t\t.gap : %"PRIu32"\n", meta->bit.gap);
	printf("\t\t\t\t.sum : %"PRIu32"\n", meta->bit.sum);
	printf("\t\t\t\t.avg : %"PRIu32"\n", meta->bit.avg);
	printf("\t\t\t.pop :\n");
	printf("\t\t\t\t.min : %"PRIu32"\n", meta->pop.min);
	printf("\t\t\t\t.max : %"PRIu32"\n", meta->pop.max);
	printf("\t\t\t\t.gap : %"PRIu32"\n", meta->pop.gap);
	printf("\t\t\t\t.sum : %"PRIu32"\n", meta->pop.sum);
	printf("\t\t\t\t.avg : %"PRIu32"\n", meta->pop.avg);
}

void
PrimeStatsMeta_PrintChart(const PrimeStats_st* stats)
{
	printf("\n");
	printf("\n----------------------------------------"
	         "----------------------------------------");
	printf("\n");
	printf("%20"PRIu64"\n", stats->prime);

	printf("\n");
	printf(".bits.cnt       %8d%8d%8d%8d%8d%8d%8d%8d\n",
	       (int)stats->meta.bits[0].cnt, (int)stats->meta.bits[1].cnt,
	       (int)stats->meta.bits[2].cnt, (int)stats->meta.bits[3].cnt,
	       (int)stats->meta.bits[4].cnt, (int)stats->meta.bits[5].cnt,
	       (int)stats->meta.bits[6].cnt, (int)stats->meta.bits[7].cnt);
	printf(".bits.bit.min   %8d%8d%8d%8d%8d%8d%8d%8d\n",
	       (int)stats->meta.bits[0].bit.min, (int)stats->meta.bits[1].bit.min,
	       (int)stats->meta.bits[2].bit.min, (int)stats->meta.bits[3].bit.min,
	       (int)stats->meta.bits[4].bit.min, (int)stats->meta.bits[5].bit.min,
	       (int)stats->meta.bits[6].bit.min, (int)stats->meta.bits[7].bit.min);
	printf(".bits.bit.max   %8d%8d%8d%8d%8d%8d%8d%8d\n",
	       (int)stats->meta.bits[0].bit.max, (int)stats->meta.bits[1].bit.max,
	       (int)stats->meta.bits[2].bit.max, (int)stats->meta.bits[3].bit.max,
	       (int)stats->meta.bits[4].bit.max, (int)stats->meta.bits[5].bit.max,
	       (int)stats->meta.bits[6].bit.max, (int)stats->meta.bits[7].bit.max);
	printf(".bits.bit.sum   %8d%8d%8d%8d%8d%8d%8d%8d\n",
	       (int)stats->meta.bits[0].bit.sum, (int)stats->meta.bits[1].bit.sum,
	       (int)stats->meta.bits[2].bit.sum, (int)stats->meta.bits[3].bit.sum,
	       (int)stats->meta.bits[4].bit.sum, (int)stats->meta.bits[5].bit.sum,
	       (int)stats->meta.bits[6].bit.sum, (int)stats->meta.bits[7].bit.sum);
	printf(".bits.bit.gap   %8d%8d%8d%8d%8d%8d%8d%8d\n",
	       (int)stats->meta.bits[0].bit.gap, (int)stats->meta.bits[1].bit.gap,
	       (int)stats->meta.bits[2].bit.gap, (int)stats->meta.bits[3].bit.gap,
	       (int)stats->meta.bits[4].bit.gap, (int)stats->meta.bits[5].bit.gap,
	       (int)stats->meta.bits[6].bit.gap, (int)stats->meta.bits[7].bit.gap);
	printf(".bits.bit.avg   %8d%8d%8d%8d%8d%8d%8d%8d\n",
	       (int)stats->meta.bits[0].bit.avg, (int)stats->meta.bits[1].bit.avg,
	       (int)stats->meta.bits[2].bit.avg, (int)stats->meta.bits[3].bit.avg,
	       (int)stats->meta.bits[4].bit.avg, (int)stats->meta.bits[5].bit.avg,
	       (int)stats->meta.bits[6].bit.avg, (int)stats->meta.bits[7].bit.avg);

	printf("\n");
	printf(".bits.pop.min   %8d%8d%8d%8d%8d%8d%8d%8d\n",
	       (int)stats->meta.bits[0].pop.min, (int)stats->meta.bits[1].pop.min,
	       (int)stats->meta.bits[2].pop.min, (int)stats->meta.bits[3].pop.min,
	       (int)stats->meta.bits[4].pop.min, (int)stats->meta.bits[5].pop.min,
	       (int)stats->meta.bits[6].pop.min, (int)stats->meta.bits[7].pop.min);
	printf(".bits.pop.max   %8d%8d%8d%8d%8d%8d%8d%8d\n",
	       (int)stats->meta.bits[0].pop.max, (int)stats->meta.bits[1].pop.max,
	       (int)stats->meta.bits[2].pop.max, (int)stats->meta.bits[3].pop.max,
	       (int)stats->meta.bits[4].pop.max, (int)stats->meta.bits[5].pop.max,
	       (int)stats->meta.bits[6].pop.max, (int)stats->meta.bits[7].pop.max);
	printf(".bits.pop.sum   %8d%8d%8d%8d%8d%8d%8d%8d\n",
	       (int)stats->meta.bits[0].pop.sum, (int)stats->meta.bits[1].pop.sum,
	       (int)stats->meta.bits[2].pop.sum, (int)stats->meta.bits[3].pop.sum,
	       (int)stats->meta.bits[4].pop.sum, (int)stats->meta.bits[5].pop.sum,
	       (int)stats->meta.bits[6].pop.sum, (int)stats->meta.bits[7].pop.sum);
	printf(".bits.pop.gap   %8d%8d%8d%8d%8d%8d%8d%8d\n",
	       (int)stats->meta.bits[0].pop.gap, (int)stats->meta.bits[1].pop.gap,
	       (int)stats->meta.bits[2].pop.gap, (int)stats->meta.bits[3].pop.gap,
	       (int)stats->meta.bits[4].pop.gap, (int)stats->meta.bits[5].pop.gap,
	       (int)stats->meta.bits[6].pop.gap, (int)stats->meta.bits[7].pop.gap);
	printf(".bits.pop.avg   %8d%8d%8d%8d%8d%8d%8d%8d\n",
	       (int)stats->meta.bits[0].pop.avg, (int)stats->meta.bits[1].pop.avg,
	       (int)stats->meta.bits[2].pop.avg, (int)stats->meta.bits[3].pop.avg,
	       (int)stats->meta.bits[4].pop.avg, (int)stats->meta.bits[5].pop.avg,
	       (int)stats->meta.bits[6].pop.avg, (int)stats->meta.bits[7].pop.avg);

	printf("\n");
	printf(".ava.cnt        %8d%8d%8d%8d%8d%8d%8d%8d\n",
	       (int)stats->meta.ava[0].cnt, (int)stats->meta.ava[1].cnt,
	       (int)stats->meta.ava[2].cnt, (int)stats->meta.ava[3].cnt,
	       (int)stats->meta.ava[4].cnt, (int)stats->meta.ava[5].cnt,
	       (int)stats->meta.ava[6].cnt, (int)stats->meta.ava[7].cnt);
	printf(".ava.bit.min    %8d%8d%8d%8d%8d%8d%8d%8d\n",
	       (int)stats->meta.ava[0].bit.min, (int)stats->meta.ava[1].bit.min,
	       (int)stats->meta.ava[2].bit.min, (int)stats->meta.ava[3].bit.min,
	       (int)stats->meta.ava[4].bit.min, (int)stats->meta.ava[5].bit.min,
	       (int)stats->meta.ava[6].bit.min, (int)stats->meta.ava[7].bit.min);
	printf(".ava.bit.max    %8d%8d%8d%8d%8d%8d%8d%8d\n",
	       (int)stats->meta.ava[0].bit.max, (int)stats->meta.ava[1].bit.max,
	       (int)stats->meta.ava[2].bit.max, (int)stats->meta.ava[3].bit.max,
	       (int)stats->meta.ava[4].bit.max, (int)stats->meta.ava[5].bit.max,
	       (int)stats->meta.ava[6].bit.max, (int)stats->meta.ava[7].bit.max);
	printf(".ava.bit.sum    %8d%8d%8d%8d%8d%8d%8d%8d\n",
	       (int)stats->meta.ava[0].bit.sum, (int)stats->meta.ava[1].bit.sum,
	       (int)stats->meta.ava[2].bit.sum, (int)stats->meta.ava[3].bit.sum,
	       (int)stats->meta.ava[4].bit.sum, (int)stats->meta.ava[5].bit.sum,
	       (int)stats->meta.ava[6].bit.sum, (int)stats->meta.ava[7].bit.sum);
	printf(".ava.bit.gap    %8d%8d%8d%8d%8d%8d%8d%8d\n",
	       (int)stats->meta.ava[0].bit.gap, (int)stats->meta.ava[1].bit.gap,
	       (int)stats->meta.ava[2].bit.gap, (int)stats->meta.ava[3].bit.gap,
	       (int)stats->meta.ava[4].bit.gap, (int)stats->meta.ava[5].bit.gap,
	       (int)stats->meta.ava[6].bit.gap, (int)stats->meta.ava[7].bit.gap);
	printf(".ava.bit.avg    %8d%8d%8d%8d%8d%8d%8d%8d\n",
	       (int)stats->meta.ava[0].bit.avg, (int)stats->meta.ava[1].bit.avg,
	       (int)stats->meta.ava[2].bit.avg, (int)stats->meta.ava[3].bit.avg,
	       (int)stats->meta.ava[4].bit.avg, (int)stats->meta.ava[5].bit.avg,
	       (int)stats->meta.ava[6].bit.avg, (int)stats->meta.ava[7].bit.avg);

	printf("\n");
	printf(".ava.pop.min    %8d%8d%8d%8d%8d%8d%8d%8d\n",
	       (int)stats->meta.ava[0].pop.min, (int)stats->meta.ava[1].pop.min,
	       (int)stats->meta.ava[2].pop.min, (int)stats->meta.ava[3].pop.min,
	       (int)stats->meta.ava[4].pop.min, (int)stats->meta.ava[5].pop.min,
	       (int)stats->meta.ava[6].pop.min, (int)stats->meta.ava[7].pop.min);
	printf(".ava.pop.max    %8d%8d%8d%8d%8d%8d%8d%8d\n",
	       (int)stats->meta.ava[0].pop.max, (int)stats->meta.ava[1].pop.max,
	       (int)stats->meta.ava[2].pop.max, (int)stats->meta.ava[3].pop.max,
	       (int)stats->meta.ava[4].pop.max, (int)stats->meta.ava[5].pop.max,
	       (int)stats->meta.ava[6].pop.max, (int)stats->meta.ava[7].pop.max);
	printf(".ava.pop.sum    %8d%8d%8d%8d%8d%8d%8d%8d\n",
	       (int)stats->meta.ava[0].pop.sum, (int)stats->meta.ava[1].pop.sum,
	       (int)stats->meta.ava[2].pop.sum, (int)stats->meta.ava[3].pop.sum,
	       (int)stats->meta.ava[4].pop.sum, (int)stats->meta.ava[5].pop.sum,
	       (int)stats->meta.ava[6].pop.sum, (int)stats->meta.ava[7].pop.sum);
	printf(".ava.pop.gap    %8d%8d%8d%8d%8d%8d%8d%8d\n",
	       (int)stats->meta.ava[0].pop.gap, (int)stats->meta.ava[1].pop.gap,
	       (int)stats->meta.ava[2].pop.gap, (int)stats->meta.ava[3].pop.gap,
	       (int)stats->meta.ava[4].pop.gap, (int)stats->meta.ava[5].pop.gap,
	       (int)stats->meta.ava[6].pop.gap, (int)stats->meta.ava[7].pop.gap);
	printf(".ava.pop.avg    %8d%8d%8d%8d%8d%8d%8d%8d\n",
	       (int)stats->meta.ava[0].pop.avg, (int)stats->meta.ava[1].pop.avg,
	       (int)stats->meta.ava[2].pop.avg, (int)stats->meta.ava[3].pop.avg,
	       (int)stats->meta.ava[4].pop.avg, (int)stats->meta.ava[5].pop.avg,
	       (int)stats->meta.ava[6].pop.avg, (int)stats->meta.ava[7].pop.avg);

	fflush(stdout);
}




#endif // _PrimeStats_h_
