char* to_buffer(char* qr);
int* parasail_sw_wrap(char* s, char* t, char* matrixname);
char* create_buffer(int length);
void free_str_memory(char* str);
int* transfer_result(parasail_result_t* result, parasail_traceback_t* traceback);
int* parasail_nw_wrap(char* s, char* t, char* matrixname);
