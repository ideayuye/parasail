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

#include <stdint.h>
#include <stdlib.h>

#include <emmintrin.h>

#include "parasail.h"
#include "parasail/memory.h"
#include "parasail/internal_sse.h"
#include "parasail/matrices/blosum_map.h"

#define NEG_INF (INT64_MIN/(int64_t)(2))
#define MAX(a,b) ((a)>(b)?(a):(b))

static inline __m128i _mm_blendv_epi8_rpl(__m128i a, __m128i b, __m128i mask) {
    a = _mm_andnot_si128(mask, a);
    a = _mm_or_si128(a, _mm_and_si128(mask, b));
    return a;
}

static inline __m128i _mm_cmpgt_epi64_rpl(__m128i a, __m128i b) {
    __m128i_64_t A;
    __m128i_64_t B;
    A.m = a;
    B.m = b;
    A.v[0] = (A.v[0]>B.v[0]) ? 0xFFFFFFFFFFFFFFFF : 0;
    A.v[1] = (A.v[1]>B.v[1]) ? 0xFFFFFFFFFFFFFFFF : 0;
    return A.m;
}

static inline __m128i _mm_insert_epi64_rpl(__m128i a, int64_t i, const int imm) {
    __m128i_64_t A;
    A.m = a;
    A.v[imm] = i;
    return A.m;
}

static inline __m128i _mm_max_epi64_rpl(__m128i a, __m128i b) {
    __m128i_64_t A;
    __m128i_64_t B;
    A.m = a;
    B.m = b;
    A.v[0] = (A.v[0]>B.v[0]) ? A.v[0] : B.v[0];
    A.v[1] = (A.v[1]>B.v[1]) ? A.v[1] : B.v[1];
    return A.m;
}

static inline int64_t _mm_extract_epi64_rpl(__m128i a, const int imm) {
    __m128i_64_t A;
    A.m = a;
    return A.v[imm];
}

#define _mm_rlli_si128_rpl(a,imm) _mm_or_si128(_mm_slli_si128(a,imm),_mm_srli_si128(a,16-imm))

static inline __m128i _mm_cmplt_epi64_rpl(__m128i a, __m128i b) {
    __m128i_64_t A;
    __m128i_64_t B;
    A.m = a;
    B.m = b;
    A.v[0] = (A.v[0]<B.v[0]) ? 0xFFFFFFFFFFFFFFFF : 0;
    A.v[1] = (A.v[1]<B.v[1]) ? 0xFFFFFFFFFFFFFFFF : 0;
    return A.m;
}

static inline __m128i _mm_cmpeq_epi64_rpl(__m128i a, __m128i b) {
    __m128i_64_t A;
    __m128i_64_t B;
    A.m = a;
    B.m = b;
    A.v[0] = (A.v[0]==B.v[0]) ? 0xFFFFFFFFFFFFFFFF : 0;
    A.v[1] = (A.v[1]==B.v[1]) ? 0xFFFFFFFFFFFFFFFF : 0;
    return A.m;
}


#ifdef PARASAIL_TABLE
static inline void arr_store_si128(
        int *array,
        __m128i vH,
        int32_t t,
        int32_t seglen,
        int32_t d,
        int32_t dlen)
{
    array[(0*seglen+t)*dlen + d] = (int64_t)_mm_extract_epi64_rpl(vH, 0);
    array[(1*seglen+t)*dlen + d] = (int64_t)_mm_extract_epi64_rpl(vH, 1);
}
#endif

#ifdef PARASAIL_TABLE
#define FNAME sw_stats_table_scan_sse2_128_64
#else
#define FNAME sw_stats_scan_sse2_128_64
#endif

parasail_result_t* FNAME(
        const char * const restrict s1, const int s1Len,
        const char * const restrict s2, const int s2Len,
        const int open, const int gap, const parasail_matrix_t *matrix)
{
    int32_t i = 0;
    int32_t j = 0;
    int32_t k = 0;
    int32_t segNum = 0;
    const int32_t n = 24; /* number of amino acids in table */
    const int32_t segWidth = 2;
    const int32_t segLen = (s1Len + segWidth - 1) / segWidth;
    __m128i* const restrict pvP  = parasail_memalign___m128i(16, n * segLen);
    __m128i* const restrict pvPm = parasail_memalign___m128i(16, n * segLen);
    __m128i* const restrict pvPs = parasail_memalign___m128i(16, n * segLen);
    __m128i* const restrict pvE  = parasail_memalign___m128i(16, segLen);
    __m128i* const restrict pvHt = parasail_memalign___m128i(16, segLen);
    __m128i* const restrict pvFt = parasail_memalign___m128i(16, segLen);
    __m128i* const restrict pvMt = parasail_memalign___m128i(16, segLen);
    __m128i* const restrict pvSt = parasail_memalign___m128i(16, segLen);
    __m128i* const restrict pvLt = parasail_memalign___m128i(16, segLen);
    __m128i* const restrict pvEx = parasail_memalign___m128i(16, segLen);
    __m128i* const restrict pvH  = parasail_memalign___m128i(16, segLen);
    __m128i* const restrict pvM  = parasail_memalign___m128i(16, segLen);
    __m128i* const restrict pvS  = parasail_memalign___m128i(16, segLen);
    __m128i* const restrict pvL  = parasail_memalign___m128i(16, segLen);
    __m128i vGapO = _mm_set1_epi64x(open);
    __m128i vGapE = _mm_set1_epi64x(gap);
    __m128i vZero = _mm_setzero_si128();
    __m128i vOne = _mm_set1_epi64x(1);
    __m128i vNegInf = _mm_set1_epi64x(NEG_INF);
    int64_t score = NEG_INF;
    int64_t matches = 0;
    int64_t similar = 0;
    int64_t length = 0;
    __m128i vMaxH = vZero;
    __m128i vMaxM = vZero;
    __m128i vMaxS = vZero;
    __m128i vMaxL = vZero;
    const int64_t segLenXgap = -segLen*gap;
    __m128i insert_mask = _mm_cmpeq_epi64_rpl(_mm_setzero_si128(),
            _mm_set_epi64x(0,1));
    __m128i vSegLenXgap_reset = _mm_blendv_epi8_rpl(vNegInf,
            _mm_set1_epi64x(segLenXgap),
            insert_mask);
    
#ifdef PARASAIL_TABLE
    parasail_result_t *result = parasail_result_new_table3(segLen*segWidth, s2Len);
#else
    parasail_result_t *result = parasail_result_new();
#endif

    parasail_memset___m128i(pvM, vZero, segLen);
    parasail_memset___m128i(pvS, vZero, segLen);
    parasail_memset___m128i(pvL, vZero, segLen);

    /* Generate query profile and match profile.
     * Rearrange query sequence & calculate the weight of match/mismatch.
     * Don't alias. */
    {
        int32_t index = 0;
        for (k=0; k<n; ++k) {
            for (i=0; i<segLen; ++i) {
                __m128i_64_t p;
                __m128i_64_t m;
                __m128i_64_t s;
                j = i;
                for (segNum=0; segNum<segWidth; ++segNum) {
                    p.v[segNum] = j >= s1Len ? 0 : matrix->matrix[matrix->size*k+parasail_blosum_map[(unsigned char)s1[j]]];
                    m.v[segNum] = j >= s1Len ? 0 : (k == parasail_blosum_map[(unsigned char)s1[j]]);
                    s.v[segNum] = p.v[segNum] > 0;
                    j += segLen;
                }
                _mm_store_si128(&pvP[index], p.m);
                _mm_store_si128(&pvPm[index], m.m);
                _mm_store_si128(&pvPs[index], s.m);
                ++index;
            }
        }
    }

    /* initialize H and E */
    {
        int32_t index = 0;
        for (i=0; i<segLen; ++i) {
            __m128i_64_t h;
            __m128i_64_t e;
            for (segNum=0; segNum<segWidth; ++segNum) {
                h.v[segNum] = 0;
                e.v[segNum] = NEG_INF;
            }
            _mm_store_si128(&pvH[index], h.m);
            _mm_store_si128(&pvE[index], e.m);
            ++index;
        }
    }

    /* outer loop over database sequence */
    for (j=0; j<s2Len; ++j) {
        __m128i vE;
        __m128i vHt;
        __m128i vFt;
        __m128i vH;
        __m128i *pvW;
        __m128i vW;
        __m128i *pvC;
        __m128i *pvD;
        __m128i vC;
        __m128i vD;
        __m128i vM;
        __m128i vMp;
        __m128i vMt;
        __m128i vS;
        __m128i vSp;
        __m128i vSt;
        __m128i vL;
        __m128i vLp;
        __m128i vLt;
        __m128i vEx;

        /* calculate E */
        for (i=0; i<segLen; ++i) {
            vH = _mm_load_si128(pvH+i);
            vE = _mm_load_si128(pvE+i);
            vE = _mm_max_epi64_rpl(
                    _mm_sub_epi64(vE, vGapE),
                    _mm_sub_epi64(vH, vGapO));
            _mm_store_si128(pvE+i, vE);
        }

        /* calculate Ht */
        vH = _mm_slli_si128(_mm_load_si128(pvH+(segLen-1)), 8);
        vMp= _mm_slli_si128(_mm_load_si128(pvM+(segLen-1)), 8);
        vSp= _mm_slli_si128(_mm_load_si128(pvS+(segLen-1)), 8);
        vLp= _mm_slli_si128(_mm_load_si128(pvL+(segLen-1)), 8);
        vLp= _mm_add_epi64(vLp, vOne);
        pvW = pvP + parasail_blosum_map[(unsigned char)s2[j]]*segLen;
        pvC = pvPm+ parasail_blosum_map[(unsigned char)s2[j]]*segLen;
        pvD = pvPs+ parasail_blosum_map[(unsigned char)s2[j]]*segLen;
        for (i=0; i<segLen; ++i) {
            __m128i cond_max;
            /* load values we need */
            vE = _mm_load_si128(pvE+i);
            vW = _mm_load_si128(pvW+i);
            /* compute */
            vH = _mm_add_epi64(vH, vW);
            vHt = _mm_max_epi64_rpl(vH, vE);
            vHt = _mm_max_epi64_rpl(vHt, vZero);
            /* statistics */
            vC = _mm_load_si128(pvC+i);
            vD = _mm_load_si128(pvD+i);
            vMp = _mm_add_epi64(vMp, vC);
            vSp = _mm_add_epi64(vSp, vD);
            vEx = _mm_cmpgt_epi64_rpl(vE, vH);
            vM = _mm_load_si128(pvM+i);
            vS = _mm_load_si128(pvS+i);
            vL = _mm_load_si128(pvL+i);
            vL = _mm_add_epi64(vL, vOne);
            vMt = _mm_blendv_epi8_rpl(vMp, vM, vEx);
            vSt = _mm_blendv_epi8_rpl(vSp, vS, vEx);
            vLt = _mm_blendv_epi8_rpl(vLp, vL, vEx);
            cond_max = _mm_cmpeq_epi64_rpl(vHt, vZero);
            vEx = _mm_andnot_si128(cond_max, vEx);
            vMt = _mm_andnot_si128(cond_max, vMt);
            vSt = _mm_andnot_si128(cond_max, vSt);
            vLt = _mm_andnot_si128(cond_max, vLt);
            /* store results */
            _mm_store_si128(pvHt+i, vHt);
            _mm_store_si128(pvEx+i, vEx);
            _mm_store_si128(pvMt+i, vMt);
            _mm_store_si128(pvSt+i, vSt);
            _mm_store_si128(pvLt+i, vLt);
            /* prep for next iteration */
            vH = _mm_load_si128(pvH+i);
            vMp = vM;
            vSp = vS;
            vLp = vL;
        }

        /* calculate Ft */
        vHt = _mm_slli_si128(_mm_load_si128(pvHt+(segLen-1)), 8);
        vFt = vNegInf;
        for (i=0; i<segLen; ++i) {
            vFt = _mm_sub_epi64(vFt, vGapE);
            vFt = _mm_max_epi64_rpl(vFt, vHt);
            vHt = _mm_load_si128(pvHt+i);
        }
        {
            __m128i vFt_save = vFt;
            __m128i segLenXgap = vSegLenXgap_reset;
            for (i=0; i<segWidth-1; ++i) {
                __m128i vFtt = _mm_slli_si128(vFt, 8);
                segLenXgap = _mm_rlli_si128_rpl(segLenXgap, 8);
                vFtt = _mm_add_epi64(vFtt, segLenXgap);
                vFt = _mm_max_epi64_rpl(vFt, vFtt);
            }
            vFt = _mm_blendv_epi8_rpl(vFt_save, vFt, insert_mask);
        }
        vHt = _mm_slli_si128(_mm_load_si128(pvHt+(segLen-1)), 8);
        vFt = _mm_slli_si128(vFt, 8);
        vFt = _mm_insert_epi64_rpl(vFt, NEG_INF, 0);
        for (i=0; i<segLen; ++i) {
            vFt = _mm_sub_epi64(vFt, vGapE);
            vFt = _mm_max_epi64_rpl(vFt, vHt);
            vHt = _mm_load_si128(pvHt+i);
            _mm_store_si128(pvFt+i, vFt);
        }

        /* calculate H,M,L */
        vMp = vZero;
        vSp = vZero;
        vLp = vOne;
        vC = _mm_cmpeq_epi64_rpl(vZero, vZero); /* check if prefix sum is needed */
        vC = _mm_srli_si128(vC, 8); /* zero out last value */
        for (i=0; i<segLen; ++i) {
            /* load values we need */
            vHt = _mm_load_si128(pvHt+i);
            vFt = _mm_load_si128(pvFt+i);
            /* compute */
            vFt = _mm_sub_epi64(vFt, vGapO);
            vH = _mm_max_epi64_rpl(vHt, vFt);
            /* statistics */
            vEx = _mm_load_si128(pvEx+i);
            vMt = _mm_load_si128(pvMt+i);
            vSt = _mm_load_si128(pvSt+i);
            vLt = _mm_load_si128(pvLt+i);
            vEx = _mm_or_si128(
                    _mm_and_si128(vEx, _mm_cmpeq_epi64_rpl(vHt, vFt)),
                    _mm_cmplt_epi64_rpl(vHt, vFt));
            vM = _mm_blendv_epi8_rpl(vMt, vMp, vEx);
            vS = _mm_blendv_epi8_rpl(vSt, vSp, vEx);
            vL = _mm_blendv_epi8_rpl(vLt, vLp, vEx);
            vMp = vM;
            vSp = vS;
            vLp = _mm_add_epi64(vL, vOne);
            vC = _mm_and_si128(vC, vEx);
            /* store results */
            _mm_store_si128(pvH+i, vH);
            _mm_store_si128(pvEx+i, vEx);
            
#ifdef PARASAIL_TABLE
            arr_store_si128(result->score_table, vH, i, segLen, j, s2Len);
#endif
        }
        {
            vLp = _mm_sub_epi64(vLp, vOne);
            {
                __m128i_64_t uMp, uSp, uLp, uC;
                uC.m = vC;
                uMp.m = vMp;
                uMp.v[1] = uC.v[1] ? uMp.v[0] : uMp.v[1];
                vMp = uMp.m;
                uSp.m = vSp;
                uSp.v[1] = uC.v[1] ? uSp.v[0] : uSp.v[1];
                vSp = uSp.m;
                uLp.m = vLp;
                uLp.v[1] = uC.v[1] ? uLp.v[1] + uLp.v[0] : uLp.v[1];
                vLp = uLp.m;
            }
            vLp = _mm_add_epi64(vLp, vOne);
        }
        /* final pass for M,L */
        vMp = _mm_slli_si128(vMp, 8);
        vSp = _mm_slli_si128(vSp, 8);
        vLp = _mm_slli_si128(vLp, 8);
        for (i=0; i<segLen; ++i) {
            /* load values we need */
            vH = _mm_load_si128(pvH+i);
            /* statistics */
            vEx = _mm_load_si128(pvEx+i);
            vMt = _mm_load_si128(pvMt+i);
            vSt = _mm_load_si128(pvSt+i);
            vLt = _mm_load_si128(pvLt+i);
            vM = _mm_blendv_epi8_rpl(vMt, vMp, vEx);
            vS = _mm_blendv_epi8_rpl(vSt, vSp, vEx);
            vL = _mm_blendv_epi8_rpl(vLt, vLp, vEx);
            vMp = vM;
            vSp = vS;
            vLp = _mm_add_epi64(vL, vOne);
            /* store results */
            _mm_store_si128(pvM+i, vM);
            _mm_store_si128(pvS+i, vS);
            _mm_store_si128(pvL+i, vL);
            
#ifdef PARASAIL_TABLE
            arr_store_si128(result->matches_table, vM, i, segLen, j, s2Len);
            arr_store_si128(result->similar_table, vS, i, segLen, j, s2Len);
            arr_store_si128(result->length_table, vL, i, segLen, j, s2Len);
#endif
            /* update max vector seen so far */
            {
                __m128i cond_max = _mm_cmpgt_epi64_rpl(vH, vMaxH);
                vMaxH = _mm_blendv_epi8_rpl(vMaxH, vH, cond_max);
                vMaxM = _mm_blendv_epi8_rpl(vMaxM, vM, cond_max);
                vMaxS = _mm_blendv_epi8_rpl(vMaxS, vS, cond_max);
                vMaxL = _mm_blendv_epi8_rpl(vMaxL, vL, cond_max);
            }
        }
    }

    /* max in vec */
    for (j=0; j<segWidth; ++j) {
        int64_t value = (int64_t) _mm_extract_epi64_rpl(vMaxH, 1);
        if (value > score) {
            score = value;
            matches = (int64_t) _mm_extract_epi64_rpl(vMaxM, 1);
            similar = (int64_t) _mm_extract_epi64_rpl(vMaxS, 1);
            length = (int64_t) _mm_extract_epi64_rpl(vMaxL, 1);
        }
        vMaxH = _mm_slli_si128(vMaxH, 8);
        vMaxM = _mm_slli_si128(vMaxM, 8);
        vMaxS = _mm_slli_si128(vMaxS, 8);
        vMaxL = _mm_slli_si128(vMaxL, 8);
    }

    

    result->score = score;
    result->matches = matches;
    result->similar = similar;
    result->length = length;

    parasail_free(pvL);
    parasail_free(pvS);
    parasail_free(pvM);
    parasail_free(pvH);
    parasail_free(pvEx);
    parasail_free(pvLt);
    parasail_free(pvSt);
    parasail_free(pvMt);
    parasail_free(pvFt);
    parasail_free(pvHt);
    parasail_free(pvE);
    parasail_free(pvPs);
    parasail_free(pvPm);
    parasail_free(pvP);

    return result;
}

