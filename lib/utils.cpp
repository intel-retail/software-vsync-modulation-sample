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
#include "utils.h"
#include "debug.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>


/**
* @brief
* Function to extract bits from a register value
* @param value - The register value
* @param start_bit - The starting bit of the field
* @param end_bit - The ending bit of the field
* @return extracted bits as a 32-bit value
*/
uint32_t extract_bits(uint32_t value, int start_bit, int end_bit)
{
	int num_bit  = end_bit - start_bit + 1;
	uint32_t mask = (1 << num_bit) - 1;
	return (value >> start_bit) & mask;
}

/**
 * @brief
 * Function to convert an integer to a binary string
 * @param value - The integer value
 * @param num_bits - The number of bits to convert
 * @param binary_str - The output binary string
 * @return void
 */
void to_binary_string(uint32_t value, int num_bits, char* binary_str)
{
	int first_one_found = 0;

	for (int i = num_bits - 1; i >= 0; i--) {
		if (value & (1 << i)) {
			first_one_found = 1;
			binary_str[num_bits - 1 - i] = '1';
		} else {
			// Always add '0' after the first '1' or if it's the last bit
			if (first_one_found || i == 0) {
				binary_str[num_bits - 1 - i] = '0';
			} else {
				binary_str[num_bits - 1 - i] = ' ';
			}
		}
	}
	binary_str[num_bits] = '\0';
}

/**
 * @brief
 * Function to parse and print register values
 * @param reg_value - The register value
 * @param offset - The offset of the register
 * @param reg_info - The register information
 * @return void
 */
void print_register(uint32_t reg_value, int offset, const register_info* reg_info)
{
	char binary_str[33]; // To hold the binary representation

	PRINT("Register Name: %s  [offset = 0x%X]\n", reg_info->name, offset);
	to_binary_string(reg_value, 32, binary_str);
	PRINT("\tValue: 0x%08X (%-5u, 0b%-s)\n", reg_value, reg_value, binary_str + strspn(binary_str, " "));

	for (int i = 0; i < MAX_FIELDS && reg_info->fields[i].name[0] != '\0'; i++)  {
		const field_info* field = &reg_info->fields[i];
		uint32_t field_value = extract_bits(reg_value, field->start_bit, field->end_bit);
		to_binary_string(field_value, field->end_bit - field->start_bit + 1, binary_str);
		PRINT("\t\t%20s [%2d:%-2d]: 0x%-6X, %-5u, 0b%-s\n", field->name,
			   field->start_bit, field->end_bit, field_value, field_value,
			   binary_str + strspn(binary_str, " "));
	}
}
