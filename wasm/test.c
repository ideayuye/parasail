#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <parasail.h>
#include <parasail/matrices/blosum62.h>
#include <parasail/matrices/pam50.h>
#include <emscripten.h>

int main(int arc, char** argv) {
    /*int major = 0;
    int minor = 0;
    int patch = 0;
    parasail_result_t* result = NULL;
    parasail_matrix_t* matrix = NULL;
    parasail_profile_t* profile = NULL;

    parasail_version(&major, &minor, &patch);
    printf("parasail is using C lib version %d.%d.%d\n", major, minor, patch);

    printf("\ntest1\n");
    printf("result = parasail_sw(\"asdf\", 4, \"asdf\", 4, 10, 1, &parasail_blosum62)\n");
    result = parasail_sw("asdf", 4, "asdf", 4, 10, 1, &parasail_blosum62);
    if (result->score != 20) {
        printf("failed\n");
        return EXIT_FAILURE;
    }
    else {
        printf("pass\n");
    }
    parasail_result_free(result);*/
	return 0;
}

char* to_buffer(char* qr) {
    char* buffer = (char*)malloc(strlen(qr) + 1);
	strcpy(buffer, qr);
    return buffer;
}

// 把结果页转换成js能接收的格式
int* transfer_result(parasail_result_t* result, parasail_traceback_t* traceback) {
    int* res = malloc(sizeof(int)*6);
    char* qr = to_buffer(traceback->query);
    char* com = to_buffer(traceback->comp);
    char* target = to_buffer(traceback->ref);
    res[0] = (int)qr; // 第一条序列比对结果
    res[1] = (int)com; 
    res[2] = (int)target; // 第二条序列比对结果
    res[3] = result->score; // 分数
    res[4] = result->end_query; // 输入第一条序列的匹配结束位置
    res[5] = result->end_ref; // 输入第二条序列的匹配结束位置
    return res;
}

EMSCRIPTEN_KEEPALIVE
int* parasail_sw_wrap(char* s, char* t, char* matrixname) {
    const parasail_matrix_t* matrix = parasail_matrix_lookup(matrixname);
    // if (matrix == NULL) {
    //     matrix = &parasail_blosum62;
    // }
    printf("matrix: %s \n", matrixname);

    int lena = strlen(s);
    int lenb = strlen(t);
    // parasail_matrix_t* matrix = &parasail_blosum62;

    parasail_result_t* result = NULL;
    result = parasail_sw_trace_scan(s, lena, t, lenb, 10, 1, matrix);

    // parasail_traceback_generic(
    //     s, strlen(s), t, strlen(t),
    //     "A", "B", matrix,
    //     result, '|', '+', '-', 79, 10, 1);

    parasail_traceback_t* traceback = NULL;

    /* test new traceback string functions */
    traceback = parasail_result_get_traceback(result, s, lena, t, lenb, matrix, '|', ':', ' ');
    // printf("\nTraceback string function\n");
    // printf("query: %s\n", traceback->query);
    // printf("align: %s\n", traceback->comp);
    // printf("target: %s\n", traceback->ref);
    // printf("score: %d; match: %d; end_query: %d; end_ref: %d; \n", result->score, result->stats->matches, result->end_query, result->end_ref);
    // printf("\n");
    int* res = transfer_result(result, traceback);
    parasail_traceback_free(traceback);
    parasail_result_free(result);
    return res;
}

EMSCRIPTEN_KEEPALIVE
int* parasail_nw_wrap(char* s, char* t, char* matrixname) {
    const parasail_matrix_t* matrix = parasail_matrix_lookup(matrixname);
    printf("matrix: %s \n", matrixname);

    int lena = strlen(s);
    int lenb = strlen(t);
    // parasail_matrix_t* matrix = &parasail_blosum62;

    parasail_result_t* result = NULL;
    result = parasail_nw_trace_scan(s, lena, t, lenb, 10, 1, matrix);

    parasail_traceback_generic(
        s, strlen(s), t, strlen(t),
        "A", "B", matrix,
        result, '|', '+', '-', 79, 10, 1);

    parasail_traceback_t* traceback = NULL;

    /* test new traceback string functions */
    traceback = parasail_result_get_traceback(result, s, lena, t, lenb, matrix, '|', ':', ' ');
    
    int* res = transfer_result(result, traceback);
    parasail_traceback_free(traceback);
    parasail_result_free(result);
    return res;
}

EMSCRIPTEN_KEEPALIVE
int* create_buffer(int length) {
	int* mem = (int*)malloc(length);
	//// 初始化 WebAssembly 内存
	//for (int i = 0; i < 8; ++i) {
	//	mem[i] = i + 1;
	//}
	//printf("hdha: %d \n", mem[2]);
	return mem;
}

EMSCRIPTEN_KEEPALIVE
void free_str_memory(char* str) {
	// Free the allocated memory
	free(str);
}


