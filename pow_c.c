#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <sys/sysinfo.h>
#include "pow_c.h"
#include "curl.h"
#include "constants.h"

pthread_mutex_t pow_c_mutex[10];
int stopC[10];
long long int countC[10];

void transform64(unsigned long *lmid, unsigned long *hmid)
{
    unsigned long alpha, beta, gamma, delta;
    unsigned long *lfrom = lmid, *hfrom = hmid;
    unsigned long *lto = lmid + STATE_LENGTH, *hto = hmid + STATE_LENGTH;
	
    for (int r = 0; r < 80; r++) {
        for (int j = 0; j < STATE_LENGTH; j++) {
            int t1 = indices_[j];
            int t2 = indices_[j + 1];
            alpha = lfrom[t1];
            beta = hfrom[t1];
            gamma = hfrom[t2];
            delta = (alpha | (~gamma)) & (lfrom[t2] ^ beta);
            lto[j] = ~delta;
            hto[j] = (alpha ^ gamma) | delta;
        }
        unsigned long *lswap = lfrom, *hswap = hfrom;
        lfrom = lto;
        hfrom = hto;
        lto = lswap;
        hto = hswap;
    }
    for (int j = 0; j < HASH_LENGTH; j++) {
        int t1 = indices_[j];
        int t2 = indices_[j + 1];
        alpha = lfrom[t1];
        beta = hfrom[t1];
        gamma = hfrom[t2];
        delta = (alpha | (~gamma)) & (lfrom[t2] ^ beta);
        lto[j] = ~delta; //6
        hto[j] = (alpha ^ gamma) | delta;
    }
}

int incr(unsigned long *mid_low, unsigned long *mid_high)
{
    int i;
    unsigned long carry = 1;
    for (i = INCR_START; i < HASH_LENGTH && carry; i++) {
        unsigned long low = mid_low[i], high = mid_high[i];
        mid_low[i] = high ^ low;
        mid_high[i] = low;
        carry = high & (~low);
    }
    return i == HASH_LENGTH;
}

void seri(unsigned long *l, unsigned long *h, int n, char *r)
{
    for (int i = HASH_LENGTH - NONCE_LENGTH; i < HASH_LENGTH; i++) {
        int ll = (l[i] >> n) & 1;
        int hh = (h[i] >> n) & 1;
        if (hh == 0 && ll == 1) {
            r[i - HASH_LENGTH + NONCE_LENGTH] = -1;
        }
        if (hh == 1 && ll == 1) {
            r[i - HASH_LENGTH + NONCE_LENGTH] = 0;
        }
        if (hh == 1 && ll == 0) {
            r[i - HASH_LENGTH + NONCE_LENGTH] = 1;
        }
    }
}

int check(unsigned long *l, unsigned long *h, int m)
{
    unsigned long nonce_probe = HBITS;
    for (int i = HASH_LENGTH - m; i < HASH_LENGTH; i++) {
        nonce_probe &= ~(l[i] ^ h[i]);
        if (nonce_probe == 0) return -1;
    }

    for (int i = 0; i < 64; i++) {
        if ((nonce_probe >> i) & 1) return i;
    }
    return -1;
}

long long int loop_cpu(unsigned long *lmid, unsigned long *hmid, int m, char *nonce, int id)
{
    int n = 0;
    long long int i = 0;
    unsigned long lcpy[STATE_LENGTH * 2], hcpy[STATE_LENGTH * 2];
    
    for (i = 0; !incr(lmid, hmid) && !stopC[id]; i++) {
        memcpy(lcpy, lmid, STATE_LENGTH * sizeof(long));
        memcpy(hcpy, hmid, STATE_LENGTH * sizeof(long));
        transform64(lcpy, hcpy);
        if ((n = check(lcpy + STATE_LENGTH, hcpy + STATE_LENGTH, m)) >= 0) {
            seri(lmid, hmid, n, nonce);
            return i * 64;
        }
    }
    return -i * 64 + 1;
}

void para(char in[], unsigned long l[], unsigned long h[])
{
    for (int i = 0; i < STATE_LENGTH; i++) {
        switch (in[i]) {
            case 0:
                l[i] = HBITS;
                h[i] = HBITS;
                break;
            case 1:
                l[i] = LBITS;
                h[i] = HBITS;
                break;
            case -1:
                l[i] = HBITS;
                h[i] = LBITS;
                break;
        }
    }
}

void incrN(int n, unsigned long *mid_low, unsigned long *mid_high)
{
	for (int j = 0; j < n; j++) {
        unsigned long carry = 1;
        for (int i = INCR_START - 27; i < INCR_START && carry; i++) {
            unsigned long low = mid_low[i], high = mid_high[i];
            mid_low[i] = high ^ low;
            mid_high[i] = low;
            carry = high & (~low);
        }
    }
}

long long int pwork(char mid[], int mwm, char nonce[], int n, int id)
{
    unsigned long lmid[STATE_LENGTH] = {0}, hmid[STATE_LENGTH] = {0};
    para(mid, lmid, hmid);
    int offset = HASH_LENGTH - NONCE_LENGTH;
    
    lmid[offset] = LOW0;
    hmid[offset] = HIGH0;
    lmid[offset+1] = LOW1;
    hmid[offset+1] = HIGH1;
    lmid[offset+2] = LOW2;
    hmid[offset+2] = HIGH2;
    lmid[offset+3] = LOW3;
    hmid[offset+3] = HIGH3;
    incrN(n, lmid, hmid);
    
	return loop_cpu(lmid, hmid, mwm, nonce, id);
}

void *pworkThread(void *pitem)
{
    Pwork_struct *pworkInfo = (Pwork_struct *) pitem;
    int task_id = pworkInfo->index;
    pworkInfo->ret = pwork(pworkInfo->mid, pworkInfo->mwm, pworkInfo->nonce, pworkInfo->n, task_id);

    pthread_mutex_lock(&pow_c_mutex[task_id]);
    if (pworkInfo->ret >= 0) {
        stopC[task_id] = 1;
        countC[task_id] += pworkInfo->ret;
        /* This means this thread got the result */
        pworkInfo->n = -1;
    } else {
        countC[task_id] += 1 - pworkInfo->ret;
    }
    pthread_mutex_unlock(&pow_c_mutex[task_id]);
    pthread_exit(NULL);
}

char *PowC(char *trytes, int mwm, int index)
{
    stopC[index] = 0;
    countC[index] = 0;

    Trytes *trytes_t = NULL;
    init_Trytes(&trytes_t);
    trytes_t->toTrytes(trytes_t, trytes, 2673);

    Curl *c = NewCurl();

    /* Preapre trytes passing to curl */
    char tyt[(transactionTrinarySize - HashSize) / 3] = {0};
    for (int i = 0; i < (transactionTrinarySize - HashSize) / 3; i++) {
        tyt[i] = trytes_t->data[i];
    }
    Trytes *inn = NULL;
    init_Trytes(&inn);
    inn->toTrytes(inn, tyt, (transactionTrinarySize - HashSize) / 3);

    c->Absorb(c, inn);

    Trits *tr = trytes_t->toTrits(trytes_t);

    char *c_state = (char *) malloc(c->state->len);
    /* Prepare an array storing tr[transactionTrinarySize - HashSize:] */
    for (int i = 0; i < tr->len - (transactionTrinarySize - HashSize); i++) {
        int idx = transactionTrinarySize - HashSize + i;
        c_state[i] = tr->data[idx];
        
    }
    for (int i =  tr->len - (transactionTrinarySize - HashSize); i < c->state->len; i++) {
        c_state[i] = c->state->data[i];
    }
    
    int num_cpu = get_nprocs_conf() - 1;
    
    pthread_t *threads = (pthread_t *) malloc(sizeof(pthread_t) * num_cpu);
    Pwork_struct *pitem = (Pwork_struct *) malloc(sizeof(Pwork_struct) * num_cpu);
    /* Prepare nonce to each thread */
    char **nonce_array = (char **) malloc(sizeof(char *) * num_cpu);
    
    /* init pthread mutex */
    pthread_mutex_init(&pow_c_mutex[index], NULL);

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

    int rst_len = trytes_t->len - NonceTrinarySize / 3 + nonce->len;
    char *rst = (char *) malloc(rst_len);
    
    for (int i = 0; i < trytes_t->len - NonceTrinarySize / 3; i++) {
        rst[i] = trytes_t->data[i];
    }
    for (int i = 0; i < rst_len - (trytes_t->len - NonceTrinarySize / 3); i++) {
        int idx = trytes_t->len - NonceTrinarySize / 3 + i;
        rst[idx] = nonce->data[i];
    }

    Trytes *last = NULL;
    init_Trytes(&last);
    last->toTrytes(last, rst, rst_len);

    Trytes *last_result = last->Hash(last);

    return last_result->data;
}
