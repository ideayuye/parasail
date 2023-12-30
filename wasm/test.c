#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <parasail.h>
#include <parasail/matrices/blosum62.h>
#include <parasail/matrices/pam50.h>
#include <emscripten.h>

int main(int arc, char** argv) {
	printf("it is work!\n");
    int major = 0;
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
    parasail_result_free(result);
	return 0;
}

char* to_buffer(char* qr) {
    char* buffer = (char*)malloc(strlen(qr) + 1);
	strcpy(buffer, qr);
    return buffer;
}

EMSCRIPTEN_KEEPALIVE
int add (int a, int b) {
	
	//print_value(a+b);
	/*for (int i = 0; i < sizeof(men); i++) {
		printf("val: %d /n", men[i]);
	}*/
	return a + b + 100;
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
    result = parasail_sw_trace(s, lena, t, lenb, 10, 1, matrix);

    parasail_traceback_t* traceback = NULL;

    /* test new traceback string functions */
    traceback = parasail_result_get_traceback(result, s, lena, t, lenb, matrix, '|', ':', '.');
    printf("\nTraceback string function\n");
    printf("query: %s\n", traceback->query);
    printf("align: %s\n", traceback->comp);
    printf("target: %s\n", traceback->ref);
    printf("\n");
    int* res = malloc(sizeof(int)*3);
    char* qr = to_buffer(traceback->query);
    char* com = to_buffer(traceback->comp);
    char* ref = to_buffer(traceback->ref);
    res[0] = (int)qr;
    res[1] = (int)com;
    res[2] = (int)ref;
    parasail_traceback_free(traceback);
    parasail_result_free(result);
    return res;
}

// char* get_query(parasail_traceback_t* traceback) {
//     char* qr = traceback->query;
//     char* buffer = (char*)malloc(strlen(qr) + 1);
//     // Copy the string to the allocated memory
// 	strcpy(buffer, qr);
//     // parasail_traceback_free(traceback);

//     // parasail_result_free(result);
//     return buffer;
// }

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

EMSCRIPTEN_KEEPALIVE
int* createIntArray() {
    int* array = (int*)malloc(5 * sizeof(int));
    for (int i = 0; i < 5; ++i) {
        array[i] = i * 2;
    }
    return array;
}





