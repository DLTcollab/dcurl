#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <sys/sysinfo.h>
#include "pow_sse.h"
#include "curl.h"
#include "constants.h"

pthread_mutex_t pow_sse_mutex[32];
int stopSSE[32];
long long int countSSE[32];

static void transform128(__m128i *lmid, __m128i *hmid)
{
	int t1, t2;
	__m128i alpha, beta, gamma, delta;
	__m128i *lto = lmid + STATE_LENGTH, *hto = hmid + STATE_LENGTH;
	__m128i *lfrom = lmid, *hfrom = hmid;
	
    for (int r = 0; r < 80; r++) {
		for (int j = 0; j < STATE_LENGTH; j++) {
			t1 = indices[j];
			t2 = indices[j + 1];
			alpha = lfrom[t1];
			beta = hfrom[t1];
			gamma = hfrom[t2];
			delta = (alpha | (~gamma)) & (lfrom[t2] ^ beta);
			lto[j] = ~delta;
			hto[j] = (alpha ^ gamma) | delta;
		}
		__m128i *lswap = lfrom, *hswap = hfrom;
		lfrom = lto;
		hfrom = hto;
		lto = lswap;
		hto = hswap;
	}
	for (int j = 0; j < HASH_LENGTH; j++) {
		t1 = indices[j];
		t2 = indices[j + 1];
		alpha = lfrom[t1];
		beta = hfrom[t1];
		gamma = hfrom[t2];
		delta = (alpha | (~gamma)) & (lfrom[t2] ^ beta);
		lto[j] = ~delta;
		hto[j] = (alpha ^ gamma) | delta;
	}
}

static int incr128(__m128i *mid_low, __m128i *mid_high)
{
	int i;
	__m128i carry;
	carry = _mm_set_epi64x(LOW00, LOW01);

	for (i = INCR_START; i < HASH_LENGTH && (i == INCR_START || carry[0]); i++) {
		__m128i low = mid_low[i], high = mid_high[i];
		mid_low[i] = high ^ low;
		mid_high[i] = low;
		carry = high & (~low);
	}
	return i == HASH_LENGTH;
}

static void seri128(__m128i *low, __m128i *high, int n, char *r)
{
	int index = 0;
	
    if (n > 63) {
		n -= 64;
		index = 1;
	}
	
    for (int i = HASH_LENGTH-NONCE_LENGTH; i < HASH_LENGTH; i++)
	{
		unsigned long long ll = (low[i][index] >> n) & 1;
		unsigned long long hh = (high[i][index] >> n) & 1;
		if (hh == 0 && ll == 1) {
			r[i + NONCE_LENGTH - HASH_LENGTH] = -1;
		}
		if (hh == 1 && ll == 1) {
			r[i + NONCE_LENGTH - HASH_LENGTH] = 0;
		}
		if (hh == 1 && ll == 0) {
			r[i + NONCE_LENGTH - HASH_LENGTH] = 1;
		}
	}
}

static int check128(__m128i *l, __m128i *h, int m)
{
	__m128i nonce_probe = _mm_set_epi64x(HBITS, HBITS);
	
    for (int i = HASH_LENGTH - m; i < HASH_LENGTH; i++) {
		nonce_probe &= ~(l[i] ^ h[i]);
		if (nonce_probe[0] == LBITS && nonce_probe[1] == LBITS) {
			return -1;
		}
	}
	for (int j = 0; j < 2; j++) {
		for (int i = 0; i < 64; i++) {
			if ((nonce_probe[j] >> i) & 1) {
				return i + j * 64;
			}
		}
	}
	return -2;
}

static long long int loop128(__m128i *lmid, __m128i *hmid, int m, char *nonce, int id)
{
	int n = 0;
	long long int i = 0;
	__m128i lcpy[STATE_LENGTH * 2], hcpy[STATE_LENGTH * 2];
	
    for (i = 0; !incr128(lmid, hmid) && !stopSSE[id]; i++) {
        for (int j = 0; j < STATE_LENGTH; j++) {
            lcpy[j] = lmid[j];
            hcpy[j] = hmid[j];
        }

        transform128(lcpy, hcpy);

        if ((n = check128(lcpy + STATE_LENGTH, hcpy + STATE_LENGTH, m)) >= 0) {
            seri128(lmid, hmid, n, nonce);
            return i * 128;
        }
    }
    return -i * 128 - 1;
}

static void para128(char in[], __m128i l[], __m128i h[])
{
	for (int i = 0; i < STATE_LENGTH; i++) {
		switch (in[i]) {
			case 0:
				l[i] = _mm_set_epi64x(HBITS, HBITS);
				h[i] = _mm_set_epi64x(HBITS, HBITS);
				break;
			case 1:
				l[i] = _mm_set_epi64x(LBITS, LBITS);
				h[i] = _mm_set_epi64x(HBITS, HBITS);
				break;
			case -1:
				l[i] = _mm_set_epi64x(HBITS, HBITS);
				h[i] = _mm_set_epi64x(LBITS, LBITS);
				break;
		}
	}
}

static void incrN128(int n,__m128i *mid_low, __m128i *mid_high)
{
	for (int j=0;j<n;j++) {
		__m128i carry = _mm_set_epi64x(HBITS, HBITS);
		for (int i = HASH_LENGTH * 2/3 + 4; i < HASH_LENGTH * 2/3 + 4 + 27 && carry[0]; i++) {
			__m128i low = mid_low[i], high = mid_high[i];
			mid_low[i] = high ^ low;
			mid_high[i] = low;
			carry = high & (~low);
		}
	}
}

static long long int pwork128(char mid[], int mwm, char nonce[], int n, int id)
{
	__m128i lmid[STATE_LENGTH], hmid[STATE_LENGTH];
	para128(mid, lmid, hmid);
	int offset = HASH_LENGTH - NONCE_LENGTH;

	lmid[offset] = _mm_set_epi64x(LOW00, LOW01);
	hmid[offset] = _mm_set_epi64x(HIGH00, HIGH01);
	lmid[offset+1] = _mm_set_epi64x(LOW10, LOW11);
	hmid[offset+1] = _mm_set_epi64x(HIGH10, HIGH11);
	lmid[offset+2] = _mm_set_epi64x(LOW20, LOW21);
	hmid[offset+2] = _mm_set_epi64x(HIGH20, HIGH21);
	lmid[offset+3] = _mm_set_epi64x(LOW30, LOW31);
	hmid[offset+3] = _mm_set_epi64x(HIGH30, HIGH31);
	lmid[offset+4] = _mm_set_epi64x(LOW40, LOW41);
	hmid[offset+4] = _mm_set_epi64x(HIGH40, HIGH41);
	incrN128(n, lmid, hmid);
	
    return loop128(lmid, hmid, mwm, nonce, id);
}

static void *pworkThread(void *pitem)
{
    Pwork_struct *pworkInfo = (Pwork_struct *) pitem;
    int task_id = pworkInfo->index;
    pworkInfo->ret = pwork128(pworkInfo->mid, pworkInfo->mwm, pworkInfo->nonce, pworkInfo->n, task_id);

    pthread_mutex_lock(&pow_sse_mutex[task_id]);
    if (pworkInfo->ret >= 0) {
        stopSSE[task_id] = 1;
        countSSE[task_id] += pworkInfo->ret;
        /* This means this thread got the result */
        pworkInfo->n = -1;
    } else {
        countSSE[task_id] += 1 - pworkInfo->ret;
    }
    pthread_mutex_unlock(&pow_sse_mutex[task_id]);
    pthread_exit(NULL);
}

static char *tx_to_cstate(Trytes *tx)
{
    Curl *c = NewCurl();
    char tyt[(transactionTrinarySize - HashSize) / 3] = {0};

    for (int i = 0; i < (transactionTrinarySize - HashSize) / 3; i++) {
        tyt[i] = tx->data[i];
    }

    Trytes *inn = NULL;
    init_Trytes(&inn);
    inn->toTrytes(inn, tyt, (transactionTrinarySize - HashSize) / 3);

    c->Absorb(c, inn);

    Trits *tr = tx->toTrits(tx);

    char *c_state = (char *) malloc(c->state->len);
    /* Prepare an array storing tr[transactionTrinarySize - HashSize:] */
    for (int i = 0; i < tr->len - (transactionTrinarySize - HashSize); i++) {
        int idx = transactionTrinarySize - HashSize + i;
        c_state[i] = tr->data[idx];
    }
    for (int i =  tr->len - (transactionTrinarySize - HashSize); i < c->state->len; i++) {
        c_state[i] = c->state->data[i];
    }
    
    return c_state;
}

static Trytes *nonce_to_result(Trytes *tx, Trytes *nonce)
{
    int rst_len = tx->len - NonceTrinarySize / 3 + nonce->len;
    char *rst = (char *) malloc(rst_len);

    for (int i = 0; i < tx->len - NonceTrinarySize / 3; i++) {
        rst[i] = tx->data[i];
    }
    for (int i = 0; i < rst_len - (tx->len - NonceTrinarySize / 3); i++) {
        int idx = tx->len - NonceTrinarySize / 3 + i;
        rst[idx] = nonce->data[i];
    }

    Trytes *final = NULL;
    init_Trytes(&final);
    final->toTrytes(final, rst, rst_len);

    return final->Hash(final);
}

char *PowSSE(char *trytes, int mwm, int index)
{
    stopSSE[index] = 0;
    countSSE[index] = 0;

    Trytes *trytes_t = NULL;
    init_Trytes(&trytes_t);
    trytes_t->toTrytes(trytes_t, trytes, 2673);
    
    char *c_state = tx_to_cstate(trytes_t);

    int num_cpu = get_nprocs_conf() - 1;
    const char *env_num_cpu = getenv("DCURL_NUM_CPU");
    if (env_num_cpu) {
        num_cpu = atoi(env_num_cpu);
    }
 
    pthread_t *threads = (pthread_t *) malloc(sizeof(pthread_t) * num_cpu);
    Pwork_struct *pitem = (Pwork_struct *) malloc(sizeof(Pwork_struct) * num_cpu);
    /* Prepare nonce to each thread */
    char **nonce_array = (char **) malloc(sizeof(char *) * num_cpu);
    
    /* init pthread mutex */
    pthread_mutex_init(&pow_sse_mutex[index], NULL);

    for (int i = 0; i < num_cpu; i++) {
        pitem[i].mid = c_state;
        pitem[i].mwm = mwm;
        pitem[i].nonce = nonce_array[i] = (char *) malloc(NonceTrinarySize);
        pitem[i].n = i;
        pitem[i].ret = 0;
        pitem[i].index = index;
        pthread_create(&threads[i], NULL, pworkThread, (void *) &pitem[i]);
    }

    int completedIndex = -1;
    for (int i = 0; i < num_cpu; i++) {
        pthread_join(threads[i], NULL);
        if (pitem[i].n == -1) completedIndex = i;
    }

    Trits *nonce_t = NULL;
    init_Trits(&nonce_t);
    nonce_t->toTrits(nonce_t, nonce_array[completedIndex], NonceTrinarySize);
    
    Trytes *nonce = nonce_t->toTrytes(nonce_t);
    
    Trytes *last_result = nonce_to_result(trytes_t, nonce);

    return last_result->data;
}
