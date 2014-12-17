/**
 * @file
 *
 * @author jeff.daily@pnnl.gov
 *
 * Copyright (c) 2014 Battelle Memorial Institute.
 *
 * All rights reserved. No warranty, explicit or implicit, provided.
 */
#include "config.h"

#include <assert.h>
#include <stdlib.h>

#include <emmintrin.h>
#include <smmintrin.h>

#include "parasail.h"
#include "parasail_internal.h"
#include "blosum/blosum_map.h"

#define NEG_INF_8 (INT8_MIN)

/* shift given vector v, insert val, return shifted val */
static inline __m128i vshift8(const __m128i v, const int val)
{
    __m128i ret = _mm_srli_si128(v, 1);
    ret = _mm_insert_epi8(ret, val, 15);
    return ret;
}

#ifdef PARASAIL_TABLE
static inline void arr_store_si128(
        int *array,
        __m128i vWscore,
        int i,
        int s1Len,
        int j,
        int s2Len)
{
    if (0 <= i+0 && i+0 < s1Len && 0 <= j-0 && j-0 < s2Len) {
        array[(i+0)*s2Len + (j-0)] = (int8_t)_mm_extract_epi8(vWscore, 15);
    }
    if (0 <= i+1 && i+1 < s1Len && 0 <= j-1 && j-1 < s2Len) {
        array[(i+1)*s2Len + (j-1)] = (int8_t)_mm_extract_epi8(vWscore, 14);
    }
    if (0 <= i+2 && i+2 < s1Len && 0 <= j-2 && j-2 < s2Len) {
        array[(i+2)*s2Len + (j-2)] = (int8_t)_mm_extract_epi8(vWscore, 13);
    }
    if (0 <= i+3 && i+3 < s1Len && 0 <= j-3 && j-3 < s2Len) {
        array[(i+3)*s2Len + (j-3)] = (int8_t)_mm_extract_epi8(vWscore, 12);
    }
    if (0 <= i+4 && i+4 < s1Len && 0 <= j-4 && j-4 < s2Len) {
        array[(i+4)*s2Len + (j-4)] = (int8_t)_mm_extract_epi8(vWscore, 11);
    }
    if (0 <= i+5 && i+5 < s1Len && 0 <= j-5 && j-5 < s2Len) {
        array[(i+5)*s2Len + (j-5)] = (int8_t)_mm_extract_epi8(vWscore, 10);
    }
    if (0 <= i+6 && i+6 < s1Len && 0 <= j-6 && j-6 < s2Len) {
        array[(i+6)*s2Len + (j-6)] = (int8_t)_mm_extract_epi8(vWscore, 9);
    }
    if (0 <= i+7 && i+7 < s1Len && 0 <= j-7 && j-7 < s2Len) {
        array[(i+7)*s2Len + (j-7)] = (int8_t)_mm_extract_epi8(vWscore, 8);
    }
    if (0 <= i+8 && i+8 < s1Len && 0 <= j-8 && j-8 < s2Len) {
        array[(i+8)*s2Len + (j-8)] = (int8_t)_mm_extract_epi8(vWscore, 7);
    }
    if (0 <= i+9 && i+9 < s1Len && 0 <= j-9 && j-9 < s2Len) {
        array[(i+9)*s2Len + (j-9)] = (int8_t)_mm_extract_epi8(vWscore, 6);
    }
    if (0 <= i+10 && i+10 < s1Len && 0 <= j-10 && j-10 < s2Len) {
        array[(i+10)*s2Len + (j-10)] = (int8_t)_mm_extract_epi8(vWscore, 5);
    }
    if (0 <= i+11 && i+11 < s1Len && 0 <= j-11 && j-11 < s2Len) {
        array[(i+11)*s2Len + (j-11)] = (int8_t)_mm_extract_epi8(vWscore, 4);
    }
    if (0 <= i+12 && i+12 < s1Len && 0 <= j-12 && j-12 < s2Len) {
        array[(i+12)*s2Len + (j-12)] = (int8_t)_mm_extract_epi8(vWscore, 3);
    }
    if (0 <= i+13 && i+13 < s1Len && 0 <= j-13 && j-13 < s2Len) {
        array[(i+13)*s2Len + (j-13)] = (int8_t)_mm_extract_epi8(vWscore, 2);
    }
    if (0 <= i+14 && i+14 < s1Len && 0 <= j-14 && j-14 < s2Len) {
        array[(i+14)*s2Len + (j-14)] = (int8_t)_mm_extract_epi8(vWscore, 1);
    }
    if (0 <= i+15 && i+15 < s1Len && 0 <= j-15 && j-15 < s2Len) {
        array[(i+15)*s2Len + (j-15)] = (int8_t)_mm_extract_epi8(vWscore, 0);
    }
}
#endif

#ifdef PARASAIL_TABLE
#define FNAME sg_table_diag_sse41_128_8
#else
#define FNAME sg_diag_sse41_128_8
#endif

parasail_result_t* FNAME(
        const char * const restrict _s1, const int s1Len,
        const char * const restrict _s2, const int s2Len,
        const int open, const int gap, const int matrix[24][24])
{
    const int N = 16; /* number of values in vector */
    const int PAD = N-1; /* N 8-bit values in vector, so N - 1 */
    const int PAD2 = PAD*2;
    int * const restrict s1 = parasail_memalign_int(16, s1Len+PAD);
    int * const restrict s2B= parasail_memalign_int(16, s2Len+PAD2);
    int * const restrict _tbl_pr = parasail_memalign_int(16, s2Len+PAD2);
    int * const restrict _del_pr = parasail_memalign_int(16, s2Len+PAD2);
    int * const restrict s2 = s2B+PAD; /* will allow later for negative indices */
    int * const restrict tbl_pr = _tbl_pr+PAD;
    int * const restrict del_pr = _del_pr+PAD;
#if PARASAIL_TABLE
    parasail_result_t *result = parasail_result_new_table1(s1Len, s2Len);
#else
    parasail_result_t *result = parasail_result_new();
#endif
    int i = 0;
    int j = 0;
    int score = NEG_INF_8;
    __m128i vSaturationCheck = _mm_setzero_si128();
    __m128i vNegLimit = _mm_set1_epi8(INT8_MIN);
    __m128i vPosLimit = _mm_set1_epi8(INT8_MAX);
    __m128i vNegInf = _mm_set1_epi8(NEG_INF_8);
    __m128i vNegInf0 = _mm_srli_si128(vNegInf, 1); /* shift in a 0 */
    __m128i vOpen = _mm_set1_epi8(open);
    __m128i vGap  = _mm_set1_epi8(gap);
    __m128i vZero = _mm_set1_epi8(0);
    __m128i vOne16 = _mm_set1_epi16(1);
    __m128i vN16 = _mm_set1_epi16(N);
    __m128i vNegOne = _mm_set1_epi8(-1);
    __m128i vNegOne16 = _mm_set1_epi16(-1);
    __m128i vILo16 = _mm_set_epi16(8,9,10,11,12,13,14,15);
    __m128i vIHi16 = _mm_set_epi16(0,1,2,3,4,5,6,7);
    __m128i vJresetLo16 = _mm_set_epi16(-8,-9,-10,-11,-12,-13,-14,-15);
    __m128i vJresetHi16 = _mm_set_epi16(0,-1,-2,-3,-4,-5,-6,-7);
    __m128i vMax = vNegInf;
    __m128i vILimit16 = _mm_set1_epi16(s1Len);
    __m128i vILimit116 = _mm_sub_epi16(vILimit16, vOne16);
    __m128i vJLimit16 = _mm_set1_epi16(s2Len);
    __m128i vJLimit116 = _mm_sub_epi16(vJLimit16, vOne16);

    /* convert _s1 from char to int in range 0-23 */
    for (i=0; i<s1Len; ++i) {
        s1[i] = MAP_BLOSUM_[(unsigned char)_s1[i]];
    }
    /* pad back of s1 with dummy values */
    for (i=s1Len; i<s1Len+PAD; ++i) {
        s1[i] = 0; /* point to first matrix row because we don't care */
    }

    /* convert _s2 from char to int in range 0-23 */
    for (j=0; j<s2Len; ++j) {
        s2[j] = MAP_BLOSUM_[(unsigned char)_s2[j]];
    }
    /* pad front of s2 with dummy values */
    for (j=-PAD; j<0; ++j) {
        s2[j] = 0; /* point to first matrix row because we don't care */
    }
    /* pad back of s2 with dummy values */
    for (j=s2Len; j<s2Len+PAD; ++j) {
        s2[j] = 0; /* point to first matrix row because we don't care */
    }

    /* set initial values for stored row */
    for (j=0; j<s2Len; ++j) {
        tbl_pr[j] = 0;
        del_pr[j] = NEG_INF_8;
    }
    /* pad front of stored row values */
    for (j=-PAD; j<0; ++j) {
        tbl_pr[j] = NEG_INF_8;
        del_pr[j] = NEG_INF_8;
    }
    /* pad back of stored row values */
    for (j=s2Len; j<s2Len+PAD; ++j) {
        tbl_pr[j] = NEG_INF_8;
        del_pr[j] = NEG_INF_8;
    }

    /* iterate over query sequence */
    for (i=0; i<s1Len-N; i+=N) {
        __m128i vNscore = vNegInf0;
        __m128i vWscore = vZero;
        __m128i vIns = vNegInf;
        __m128i vDel = vNegInf;
        __m128i vJLo16 = vJresetLo16;
        __m128i vJHi16 = vJresetHi16;
        const int * const restrict matrow0 = matrix[s1[i+0]];
        const int * const restrict matrow1 = matrix[s1[i+1]];
        const int * const restrict matrow2 = matrix[s1[i+2]];
        const int * const restrict matrow3 = matrix[s1[i+3]];
        const int * const restrict matrow4 = matrix[s1[i+4]];
        const int * const restrict matrow5 = matrix[s1[i+5]];
        const int * const restrict matrow6 = matrix[s1[i+6]];
        const int * const restrict matrow7 = matrix[s1[i+7]];
        const int * const restrict matrow8 = matrix[s1[i+8]];
        const int * const restrict matrow9 = matrix[s1[i+9]];
        const int * const restrict matrow10 = matrix[s1[i+10]];
        const int * const restrict matrow11 = matrix[s1[i+11]];
        const int * const restrict matrow12 = matrix[s1[i+12]];
        const int * const restrict matrow13 = matrix[s1[i+13]];
        const int * const restrict matrow14 = matrix[s1[i+14]];
        const int * const restrict matrow15 = matrix[s1[i+15]];
        /* iterate over database sequence */
        for (j=0; j<N; ++j) {
            __m128i vMat;
            __m128i vNWscore = vNscore;
            vNscore = vshift8(vWscore, tbl_pr[j]);
            vDel = vshift8(vDel, del_pr[j]);
            vDel = _mm_max_epi8(
                    _mm_subs_epi8(vNscore, vOpen),
                    _mm_subs_epi8(vDel, vGap));
            vIns = _mm_max_epi8(
                    _mm_subs_epi8(vWscore, vOpen),
                    _mm_subs_epi8(vIns, vGap));
            vMat = _mm_set_epi8(
                    matrow0[s2[j-0]],
                    matrow1[s2[j-1]],
                    matrow2[s2[j-2]],
                    matrow3[s2[j-3]],
                    matrow4[s2[j-4]],
                    matrow5[s2[j-5]],
                    matrow6[s2[j-6]],
                    matrow7[s2[j-7]],
                    matrow8[s2[j-8]],
                    matrow9[s2[j-9]],
                    matrow10[s2[j-10]],
                    matrow11[s2[j-11]],
                    matrow12[s2[j-12]],
                    matrow13[s2[j-13]],
                    matrow14[s2[j-14]],
                    matrow15[s2[j-15]]
                    );
            vNWscore = _mm_adds_epi8(vNWscore, vMat);
            vWscore = _mm_max_epi8(vNWscore, vIns);
            vWscore = _mm_max_epi8(vWscore, vDel);
            /* as minor diagonal vector passes across the j=-1 boundary,
             * assign the appropriate boundary conditions */
            {
                __m128i cond = _mm_packs_epi16(
                        _mm_cmpeq_epi16(vJLo16,vNegOne),
                        _mm_cmpeq_epi16(vJHi16,vNegOne));
                vWscore = _mm_andnot_si128(cond, vWscore);
                vDel = _mm_andnot_si128(cond, vDel);
                vDel = _mm_or_si128(vDel, _mm_and_si128(cond, vNegInf));
                vIns = _mm_andnot_si128(cond, vIns);
                vIns = _mm_or_si128(vIns, _mm_and_si128(cond, vNegInf));
            }
            /* check for saturation */
            {
                vSaturationCheck = _mm_or_si128(vSaturationCheck,
                        _mm_or_si128(
                            _mm_cmpeq_epi8(vWscore, vNegLimit),
                            _mm_cmpeq_epi8(vWscore, vPosLimit)));
            }
#ifdef PARASAIL_TABLE
            arr_store_si128(result->score_table, vWscore, i, s1Len, j, s2Len);
#endif
            tbl_pr[j-15] = (int8_t)_mm_extract_epi8(vWscore,0);
            del_pr[j-15] = (int8_t)_mm_extract_epi8(vDel,0);
            vJLo16 = _mm_adds_epi16(vJLo16, vOne16);
            vJHi16 = _mm_adds_epi16(vJHi16, vOne16);
        }
        for (j=N; j<s2Len-1; ++j) {
            __m128i vMat;
            __m128i vNWscore = vNscore;
            vNscore = vshift8(vWscore, tbl_pr[j]);
            vDel = vshift8(vDel, del_pr[j]);
            vDel = _mm_max_epi8(
                    _mm_subs_epi8(vNscore, vOpen),
                    _mm_subs_epi8(vDel, vGap));
            vIns = _mm_max_epi8(
                    _mm_subs_epi8(vWscore, vOpen),
                    _mm_subs_epi8(vIns, vGap));
            vMat = _mm_set_epi8(
                    matrow0[s2[j-0]],
                    matrow1[s2[j-1]],
                    matrow2[s2[j-2]],
                    matrow3[s2[j-3]],
                    matrow4[s2[j-4]],
                    matrow5[s2[j-5]],
                    matrow6[s2[j-6]],
                    matrow7[s2[j-7]],
                    matrow8[s2[j-8]],
                    matrow9[s2[j-9]],
                    matrow10[s2[j-10]],
                    matrow11[s2[j-11]],
                    matrow12[s2[j-12]],
                    matrow13[s2[j-13]],
                    matrow14[s2[j-14]],
                    matrow15[s2[j-15]]
                    );
            vNWscore = _mm_adds_epi8(vNWscore, vMat);
            vWscore = _mm_max_epi8(vNWscore, vIns);
            vWscore = _mm_max_epi8(vWscore, vDel);
            /* check for saturation */
            {
                vSaturationCheck = _mm_or_si128(vSaturationCheck,
                        _mm_or_si128(
                            _mm_cmpeq_epi8(vWscore, vNegLimit),
                            _mm_cmpeq_epi8(vWscore, vPosLimit)));
            }
#ifdef PARASAIL_TABLE
            arr_store_si128(result->score_table, vWscore, i, s1Len, j, s2Len);
#endif
            tbl_pr[j-15] = (int8_t)_mm_extract_epi8(vWscore,0);
            del_pr[j-15] = (int8_t)_mm_extract_epi8(vDel,0);
            vJLo16 = _mm_adds_epi16(vJLo16, vOne16);
            vJHi16 = _mm_adds_epi16(vJHi16, vOne16);
        }
        for (j=s2Len-1; j<s2Len+PAD; ++j) {
            __m128i vMat;
            __m128i vNWscore = vNscore;
            vNscore = vshift8(vWscore, tbl_pr[j]);
            vDel = vshift8(vDel, del_pr[j]);
            vDel = _mm_max_epi8(
                    _mm_subs_epi8(vNscore, vOpen),
                    _mm_subs_epi8(vDel, vGap));
            vIns = _mm_max_epi8(
                    _mm_subs_epi8(vWscore, vOpen),
                    _mm_subs_epi8(vIns, vGap));
            vMat = _mm_set_epi8(
                    matrow0[s2[j-0]],
                    matrow1[s2[j-1]],
                    matrow2[s2[j-2]],
                    matrow3[s2[j-3]],
                    matrow4[s2[j-4]],
                    matrow5[s2[j-5]],
                    matrow6[s2[j-6]],
                    matrow7[s2[j-7]],
                    matrow8[s2[j-8]],
                    matrow9[s2[j-9]],
                    matrow10[s2[j-10]],
                    matrow11[s2[j-11]],
                    matrow12[s2[j-12]],
                    matrow13[s2[j-13]],
                    matrow14[s2[j-14]],
                    matrow15[s2[j-15]]
                    );
            vNWscore = _mm_adds_epi8(vNWscore, vMat);
            vWscore = _mm_max_epi8(vNWscore, vIns);
            vWscore = _mm_max_epi8(vWscore, vDel);
            /* check for saturation */
            {
                vSaturationCheck = _mm_or_si128(vSaturationCheck,
                        _mm_or_si128(
                            _mm_cmpeq_epi8(vWscore, vNegLimit),
                            _mm_cmpeq_epi8(vWscore, vPosLimit)));
            }
#ifdef PARASAIL_TABLE
            arr_store_si128(result->score_table, vWscore, i, s1Len, j, s2Len);
#endif
            tbl_pr[j-15] = (int8_t)_mm_extract_epi8(vWscore,0);
            del_pr[j-15] = (int8_t)_mm_extract_epi8(vDel,0);
            /* as minor diagonal vector passes across the j limit
             * boundary, extract the last value of the row */
            {
                __m128i cond_j = _mm_packs_epi16(
                        _mm_cmpeq_epi16(vJLo16, vJLimit116),
                        _mm_cmpeq_epi16(vJHi16, vJLimit116));
                __m128i cond_max = _mm_cmpgt_epi8(vWscore, vMax);
                __m128i cond_all = _mm_and_si128(cond_max, cond_j);
                vMax = _mm_andnot_si128(cond_all, vMax);
                vMax = _mm_or_si128(vMax, _mm_and_si128(cond_all, vWscore));
            }
            vJLo16 = _mm_adds_epi16(vJLo16, vOne16);
            vJHi16 = _mm_adds_epi16(vJHi16, vOne16);
        }
        vILo16 = _mm_adds_epi16(vILo16, vN16);
        vIHi16 = _mm_adds_epi16(vIHi16, vN16);
    }
    for (/*i=?*/; i<s1Len; i+=N) {
        __m128i vNscore = vNegInf0;
        __m128i vWscore = vZero;
        __m128i vIns = vNegInf;
        __m128i vDel = vNegInf;
        __m128i vJLo16 = vJresetLo16;
        __m128i vJHi16 = vJresetHi16;
        const int * const restrict matrow0 = matrix[s1[i+0]];
        const int * const restrict matrow1 = matrix[s1[i+1]];
        const int * const restrict matrow2 = matrix[s1[i+2]];
        const int * const restrict matrow3 = matrix[s1[i+3]];
        const int * const restrict matrow4 = matrix[s1[i+4]];
        const int * const restrict matrow5 = matrix[s1[i+5]];
        const int * const restrict matrow6 = matrix[s1[i+6]];
        const int * const restrict matrow7 = matrix[s1[i+7]];
        const int * const restrict matrow8 = matrix[s1[i+8]];
        const int * const restrict matrow9 = matrix[s1[i+9]];
        const int * const restrict matrow10 = matrix[s1[i+10]];
        const int * const restrict matrow11 = matrix[s1[i+11]];
        const int * const restrict matrow12 = matrix[s1[i+12]];
        const int * const restrict matrow13 = matrix[s1[i+13]];
        const int * const restrict matrow14 = matrix[s1[i+14]];
        const int * const restrict matrow15 = matrix[s1[i+15]];
        __m128i vIltLimit = _mm_packs_epi16(
                _mm_cmplt_epi16(vILo16, vILimit16),
                _mm_cmplt_epi16(vIHi16, vILimit16));
        __m128i vIeqLimit1 = _mm_packs_epi16(
                _mm_cmpeq_epi16(vILo16, vILimit116),
                _mm_cmpeq_epi16(vIHi16, vILimit116));
        /* iterate over database sequence */
        for (j=0; j<N; ++j) {
            __m128i vMat;
            __m128i vNWscore = vNscore;
            vNscore = vshift8(vWscore, tbl_pr[j]);
            vDel = vshift8(vDel, del_pr[j]);
            vDel = _mm_max_epi8(
                    _mm_subs_epi8(vNscore, vOpen),
                    _mm_subs_epi8(vDel, vGap));
            vIns = _mm_max_epi8(
                    _mm_subs_epi8(vWscore, vOpen),
                    _mm_subs_epi8(vIns, vGap));
            vMat = _mm_set_epi8(
                    matrow0[s2[j-0]],
                    matrow1[s2[j-1]],
                    matrow2[s2[j-2]],
                    matrow3[s2[j-3]],
                    matrow4[s2[j-4]],
                    matrow5[s2[j-5]],
                    matrow6[s2[j-6]],
                    matrow7[s2[j-7]],
                    matrow8[s2[j-8]],
                    matrow9[s2[j-9]],
                    matrow10[s2[j-10]],
                    matrow11[s2[j-11]],
                    matrow12[s2[j-12]],
                    matrow13[s2[j-13]],
                    matrow14[s2[j-14]],
                    matrow15[s2[j-15]]
                    );
            vNWscore = _mm_adds_epi8(vNWscore, vMat);
            vWscore = _mm_max_epi8(vNWscore, vIns);
            vWscore = _mm_max_epi8(vWscore, vDel);
            /* as minor diagonal vector passes across the j=-1 boundary,
             * assign the appropriate boundary conditions */
            {
                __m128i cond = _mm_packs_epi16(
                        _mm_cmpeq_epi16(vJLo16,vNegOne16),
                        _mm_cmpeq_epi16(vJHi16,vNegOne16));
                vWscore = _mm_andnot_si128(cond, vWscore);
                vDel = _mm_andnot_si128(cond, vDel);
                vDel = _mm_or_si128(vDel, _mm_and_si128(cond, vNegInf));
                vIns = _mm_andnot_si128(cond, vIns);
                vIns = _mm_or_si128(vIns, _mm_and_si128(cond, vNegInf));
            }
            /* check for saturation */
            {
                vSaturationCheck = _mm_or_si128(vSaturationCheck,
                        _mm_or_si128(
                            _mm_cmpeq_epi8(vWscore, vNegLimit),
                            _mm_cmpeq_epi8(vWscore, vPosLimit)));
            }
#ifdef PARASAIL_TABLE
            arr_store_si128(result->score_table, vWscore, i, s1Len, j, s2Len);
#endif
            tbl_pr[j-15] = (int8_t)_mm_extract_epi8(vWscore,0);
            del_pr[j-15] = (int8_t)_mm_extract_epi8(vDel,0);
            /* as minor diagonal vector passes across the i limit
             * boundary, extract the last value of the column */
            {
                __m128i cond_i = _mm_and_si128(
                        vIeqLimit1,
                        _mm_and_si128(
                            _mm_packs_epi16(
                                _mm_cmpgt_epi16(vJLo16, vNegOne16),
                                _mm_cmpgt_epi16(vJHi16, vNegOne16)),
                            _mm_packs_epi16(
                                _mm_cmplt_epi16(vJLo16, vJLimit16),
                                _mm_cmplt_epi16(vJHi16, vJLimit16)))
                        );
                __m128i cond_max = _mm_cmpgt_epi8(vWscore, vMax);
                __m128i cond_all = _mm_and_si128(cond_max, cond_i);
                vMax = _mm_andnot_si128(cond_all, vMax);
                vMax = _mm_or_si128(vMax, _mm_and_si128(cond_all, vWscore));
            }
            vJLo16 = _mm_adds_epi16(vJLo16, vOne16);
            vJHi16 = _mm_adds_epi16(vJHi16, vOne16);
        }
        for (j=N; j<s2Len-1; ++j) {
            __m128i vMat;
            __m128i vNWscore = vNscore;
            vNscore = vshift8(vWscore, tbl_pr[j]);
            vDel = vshift8(vDel, del_pr[j]);
            vDel = _mm_max_epi8(
                    _mm_subs_epi8(vNscore, vOpen),
                    _mm_subs_epi8(vDel, vGap));
            vIns = _mm_max_epi8(
                    _mm_subs_epi8(vWscore, vOpen),
                    _mm_subs_epi8(vIns, vGap));
            vMat = _mm_set_epi8(
                    matrow0[s2[j-0]],
                    matrow1[s2[j-1]],
                    matrow2[s2[j-2]],
                    matrow3[s2[j-3]],
                    matrow4[s2[j-4]],
                    matrow5[s2[j-5]],
                    matrow6[s2[j-6]],
                    matrow7[s2[j-7]],
                    matrow8[s2[j-8]],
                    matrow9[s2[j-9]],
                    matrow10[s2[j-10]],
                    matrow11[s2[j-11]],
                    matrow12[s2[j-12]],
                    matrow13[s2[j-13]],
                    matrow14[s2[j-14]],
                    matrow15[s2[j-15]]
                    );
            vNWscore = _mm_adds_epi8(vNWscore, vMat);
            vWscore = _mm_max_epi8(vNWscore, vIns);
            vWscore = _mm_max_epi8(vWscore, vDel);
            /* check for saturation */
            {
                vSaturationCheck = _mm_or_si128(vSaturationCheck,
                        _mm_or_si128(
                            _mm_cmpeq_epi8(vWscore, vNegLimit),
                            _mm_cmpeq_epi8(vWscore, vPosLimit)));
            }
#ifdef PARASAIL_TABLE
            arr_store_si128(result->score_table, vWscore, i, s1Len, j, s2Len);
#endif
            tbl_pr[j-15] = (int8_t)_mm_extract_epi8(vWscore,0);
            del_pr[j-15] = (int8_t)_mm_extract_epi8(vDel,0);
            /* as minor diagonal vector passes across the i limit
             * boundary, extract the last value of the column */
            {
                __m128i cond_i = _mm_and_si128(
                        vIeqLimit1,
                        _mm_and_si128(
                            _mm_packs_epi16(
                                _mm_cmpgt_epi16(vJLo16, vNegOne16),
                                _mm_cmpgt_epi16(vJHi16, vNegOne16)),
                            _mm_packs_epi16(
                                _mm_cmplt_epi16(vJLo16, vJLimit16),
                                _mm_cmplt_epi16(vJHi16, vJLimit16)))
                        );
                __m128i cond_max = _mm_cmpgt_epi8(vWscore, vMax);
                __m128i cond_all = _mm_and_si128(cond_max, cond_i);
                vMax = _mm_andnot_si128(cond_all, vMax);
                vMax = _mm_or_si128(vMax, _mm_and_si128(cond_all, vWscore));
            }
            vJLo16 = _mm_adds_epi16(vJLo16, vOne16);
            vJHi16 = _mm_adds_epi16(vJHi16, vOne16);
        }
        for (j=s2Len-1; j<s2Len+PAD; ++j) {
            __m128i vMat;
            __m128i vNWscore = vNscore;
            vNscore = vshift8(vWscore, tbl_pr[j]);
            vDel = vshift8(vDel, del_pr[j]);
            vDel = _mm_max_epi8(
                    _mm_subs_epi8(vNscore, vOpen),
                    _mm_subs_epi8(vDel, vGap));
            vIns = _mm_max_epi8(
                    _mm_subs_epi8(vWscore, vOpen),
                    _mm_subs_epi8(vIns, vGap));
            vMat = _mm_set_epi8(
                    matrow0[s2[j-0]],
                    matrow1[s2[j-1]],
                    matrow2[s2[j-2]],
                    matrow3[s2[j-3]],
                    matrow4[s2[j-4]],
                    matrow5[s2[j-5]],
                    matrow6[s2[j-6]],
                    matrow7[s2[j-7]],
                    matrow8[s2[j-8]],
                    matrow9[s2[j-9]],
                    matrow10[s2[j-10]],
                    matrow11[s2[j-11]],
                    matrow12[s2[j-12]],
                    matrow13[s2[j-13]],
                    matrow14[s2[j-14]],
                    matrow15[s2[j-15]]
                    );
            vNWscore = _mm_adds_epi8(vNWscore, vMat);
            vWscore = _mm_max_epi8(vNWscore, vIns);
            vWscore = _mm_max_epi8(vWscore, vDel);
            /* check for saturation */
            {
                vSaturationCheck = _mm_or_si128(vSaturationCheck,
                        _mm_or_si128(
                            _mm_cmpeq_epi8(vWscore, vNegLimit),
                            _mm_cmpeq_epi8(vWscore, vPosLimit)));
            }
#ifdef PARASAIL_TABLE
            arr_store_si128(result->score_table, vWscore, i, s1Len, j, s2Len);
#endif
            tbl_pr[j-15] = (int8_t)_mm_extract_epi8(vWscore,0);
            del_pr[j-15] = (int8_t)_mm_extract_epi8(vDel,0);
            /* as minor diagonal vector passes across the i or j limit
             * boundary, extract the last value of the column or row */
            {
                __m128i cond_j = _mm_and_si128(
                        vIltLimit,
                        _mm_packs_epi16(
                            _mm_cmpeq_epi16(vJLo16, vJLimit116),
                            _mm_cmpeq_epi16(vJHi16, vJLimit116))
                        );
                __m128i cond_i = _mm_and_si128(
                        vIeqLimit1,
                        _mm_and_si128(
                            _mm_packs_epi16(
                                _mm_cmpgt_epi16(vJLo16, vNegOne16),
                                _mm_cmpgt_epi16(vJHi16, vNegOne16)),
                            _mm_packs_epi16(
                                _mm_cmplt_epi16(vJLo16, vJLimit16),
                                _mm_cmplt_epi16(vJHi16, vJLimit16)))
                        );
                __m128i cond_max = _mm_cmpgt_epi8(vWscore, vMax);
                __m128i cond_all = _mm_and_si128(cond_max,
                        _mm_or_si128(cond_i, cond_j));
                vMax = _mm_andnot_si128(cond_all, vMax);
                vMax = _mm_or_si128(vMax, _mm_and_si128(cond_all, vWscore));
            }
            vJLo16 = _mm_adds_epi16(vJLo16, vOne16);
            vJHi16 = _mm_adds_epi16(vJHi16, vOne16);
        }
        vILo16 = _mm_adds_epi16(vILo16, vN16);
        vIHi16 = _mm_adds_epi16(vIHi16, vN16);
    }

    /* max in vMax */
    for (i=0; i<N; ++i) {
        int8_t value;
        value = (int8_t) _mm_extract_epi8(vMax, 15);
        if (value > score) {
            score = value;
        }
        vMax = _mm_slli_si128(vMax, 1);
    }
    if (_mm_movemask_epi8(vSaturationCheck)) {
        score = INT8_MAX;
    }

    result->score = score;

    free(_del_pr);
    free(_tbl_pr);
    free(s2B);
    free(s1);

    return result;
}
