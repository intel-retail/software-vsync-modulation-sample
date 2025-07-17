/*
 * Copyright © 2024 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 */

#ifndef _UTILS_H
#define _UTILS_H

#include <cstdint>
#include <string>
#include <vector>
// Define a fixed-size array for fields (adjust MAX_FIELDS if needed)
#define MAX_FIELDS 32

// Structure to hold field information
typedef struct {
    char name[21]; // Assuming field names won't exceed 20 characters
    int start_bit;
    int end_bit;
} field_info;

// Structure to hold register information
typedef struct {
    char name[21]; // Assuming register names won't exceed 20 characters
    field_info fields[MAX_FIELDS+1];
} register_info;

void print_register(uint32_t reg_value, int offset,
						   const register_info* reg_info);

#endif
