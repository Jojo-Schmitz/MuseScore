#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "s7/s7.h"
#include "utils.h"

static int goo_type_tag = 0;

static s7_pointer goo_to_string (s7_scheme *sc, s7_pointer args)
{
  s7_pointer result;
  goo_t *o = (goo_t *)s7_c_object_value(s7_car(args));
  char *data_str = s7_object_to_c_string(sc, o->data);
  int data_str_len = strlen(data_str);
  char *str = (char *)calloc(data_str_len + 32, sizeof(char));
  snprintf(str, data_str_len + 32, "<goo %li %s>", o->ty, data_str);
  free(data_str);
  result = s7_make_string(sc, str);
  free(str);
  return(result);
}

static s7_pointer free_goo (s7_scheme *sc, s7_pointer obj)
{
  (void) sc;
  goo_t *o = (goo_t *)s7_c_object_value(obj);
  //FIX: hook to g_user_free
  //if (o->cd && g_user_free) {
  //    user_free(o);
  //}
  free(s7_c_object_value(obj));
  return(NULL);
}

static s7_pointer mark_goo (s7_scheme *sc, s7_pointer obj)
{
  (void) sc;
  goo_t *o = (goo_t *)s7_c_object_value(obj);
  s7_mark(o->data);
  return(NULL);
}

s7_pointer c_make_goo (s7_scheme *sc, uint64_t ty, s7_pointer data, void *cd)
{
  goo_t *o = (goo_t *)malloc(sizeof(goo_t));
  o->ty = ty;
  o->data = data;
  o->cd = cd;
  return(s7_make_c_object(sc, goo_type_tag, (void *)o));
}

static s7_pointer make_goo (s7_scheme *sc, s7_pointer args)
{
  goo_t *o = (goo_t *)malloc(sizeof(goo_t));
  o->cd = NULL;
  o->ty = s7_integer(s7_car(args));
  s7_pointer s = s7_cdr(args);
  if (s != s7_nil(sc)) {
    o->data = s7_car(s);
    s = s7_cdr(s);
    if (s != s7_nil(sc)) {
      o->cd = s7_car(s);
    } else {
      o->cd = s7_nil(sc);
    }
  } else {
    o->data = s7_nil(sc);
  }
  return(s7_make_c_object(sc, goo_type_tag, (void *)o));
}

static s7_pointer is_goo(s7_scheme *sc, s7_pointer args)
{
  return(s7_make_boolean(sc, 
                         s7_is_c_object(s7_car(args)) &&
                         s7_c_object_type(s7_car(args)) == goo_type_tag));
}

bool c_is_goo(s7_scheme *sc, s7_pointer g)
{
    return s7_is_c_object(g) &&
           s7_c_object_type(g) == goo_type_tag;
}

static s7_pointer goo_ty(s7_scheme *sc, s7_pointer args)
{
  goo_t *o = (goo_t *)s7_c_object_value(s7_car(args));
  return(s7_make_integer(sc, o->ty));
}

static s7_pointer set_goo_ty(s7_scheme *sc, s7_pointer args)
{
  (void) sc;
  goo_t *o = (goo_t *)s7_c_object_value(s7_car(args));
  o->ty = s7_integer(s7_cadr(args));
  return(s7_cadr(args));
}

static s7_pointer goo_data(s7_scheme *sc, s7_pointer args)
{
  (void) sc;
  goo_t *o = (goo_t *)s7_c_object_value(s7_car(args));
  return(o->data);
}

static s7_pointer set_goo_data(s7_scheme *sc, s7_pointer args)
{
  (void) sc;
  goo_t *o = (goo_t *)s7_c_object_value(s7_car(args));
  o->data = s7_cadr(args);
  return(o->data);
}

static s7_pointer goo_is_equal(s7_scheme *sc, s7_pointer args) 
{
  goo_t *d1, *d2;
  s7_pointer p1 = s7_car(args);
  s7_pointer p2 = s7_cadr(args);
  if (p1 == p2) 
    return(s7_t(sc));
  if ((!s7_is_c_object(p2)) ||
      (s7_c_object_type(p2) != goo_type_tag))
    return(s7_f(sc));
  d1 = (goo_t *)s7_c_object_value(p1);
  d2 = (goo_t *)s7_c_object_value(p2);
  return(s7_make_boolean(sc,
                          (d1->ty == d2->ty) &&
                          (s7_is_equal(sc, d1->data, d2->data))));
}

void init_goo (s7_scheme *sc)
{
  goo_type_tag = s7_make_c_type(sc, "goo");
  s7_c_type_set_gc_free(sc, goo_type_tag, free_goo);
  s7_c_type_set_gc_mark(sc, goo_type_tag, mark_goo);
  s7_c_type_set_is_equal(sc, goo_type_tag, goo_is_equal);
  s7_c_type_set_to_string(sc, goo_type_tag, goo_to_string);
  
  s7_define_function(sc, "make-goo", make_goo, 1, 2, false, "(make-goo ty data) makes a new goo");
  s7_define_function(sc, "goo?", is_goo, 1, 0, false, "(goo? anything) returns #t if its argument is a goo object");

  s7_define_variable(sc, "goo-ty", 
                     s7_dilambda(sc, "goo-ty", goo_ty, 1, 0, set_goo_ty, 2, 0, "goo ty field"));

  s7_define_variable(sc, "goo-data", 
                     s7_dilambda(sc, "goo-data", goo_data, 1, 0, set_goo_data, 2, 0, "goo data field"));
}


char* slurp_stream (const FILE* fp, int wrap) {
    int BSZ = 64;
    int bufSize = BSZ;
    char* buffer = (char *) malloc(bufSize);
    if (!buffer) return NULL;
    char block[BSZ*2];
    int finSize = 0;
    memset(block, 0, BSZ*2);
    while (1) {
        memset(block, 0, BSZ);
        fread(block, BSZ, 1, fp); // can't rely on return value from final fread (is 0)
        if (feof(fp)) {
            char *newbuffer = (char *) malloc(bufSize + BSZ);
            memcpy(newbuffer, buffer, finSize);
            for(int i = 0; i < BSZ; i++) {
                newbuffer[finSize] = 0;
                if (block[i] == 0) break;
                newbuffer[finSize] = block[i];
                finSize++;
            }
            free(buffer);
            buffer = newbuffer;
            break;
        } else {
            char *newbuffer = (char *) malloc(bufSize + BSZ);
            memcpy(newbuffer, buffer, finSize);
            memcpy(newbuffer+finSize, block, BSZ);
            finSize += BSZ;
            bufSize += BSZ;
            free(buffer);
            buffer = newbuffer;
        }
    }
    if (!wrap) {
        return buffer;
    }
    char *prefix = "(begin "; int plen = strlen(prefix);
    char *suffix = ")";
    char *fin = (char *) malloc(finSize + plen + 2);
    strcpy(fin, prefix);
    memcpy(fin+plen, buffer, finSize);
    fin[plen + finSize] = ')';
    fin[plen + finSize + 1] = 0;
    free(buffer);
    return fin;
}

char* slurp_filestream (const FILE* fp) {
    char * buffer = 0;
    size_t len;
    if (fp) {
        fseek(fp, 0, SEEK_END);
        len = ftell (fp);
        fseek(fp, 0, SEEK_SET);
        buffer = malloc(len);
        if (! buffer) return NULL;
        fread(buffer, 1, len, fp);
    }
    return buffer;
}

char* slurp_file (const char *filename) {
    FILE* fp = fopen(filename, "r");
    if (!fp) {
        fprintf(stderr, "ERROR: cant open file %s: %s\n", filename, strerror(errno));
        return NULL;
    }
    char *buffer = slurp_filestream(fp);
    fclose(fp);
    return buffer;
}

