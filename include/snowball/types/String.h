
#include <stdint.h>
#include <stdio.h>
#include "../constants.h"

#ifndef SN_STRING_EXPORT_H
#define SN_STRING_EXPORT_H

extern "C" {

typedef struct String {
    const char * buffer;
    snowball_int_t length;
} String;

}

extern "C" DLLEXPORT String* String__init_s(const char* string_ptr);
extern "C" DLLEXPORT String* String__sum_String(String* self, String* sum);

#endif // SN_STRING_EXPORT_H
