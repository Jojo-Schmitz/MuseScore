
#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    s7_int ty; /* type of object */
    s7_pointer data; /* scheme-side data */
    void* cd; /* c-side data */
} goo_t;

void init_goo (s7_scheme *sc);
s7_pointer c_make_goo (s7_scheme *sc, uint64_t ty, s7_pointer data, void *cd);
bool c_is_goo(s7_scheme *sc, s7_pointer g);

char* slurp_filestream (const FILE* fp);
char* slurp_stream (const FILE* fp, int wrap);
char* slurp_file (const char *filename);

#ifdef __cplusplus
}
#endif
