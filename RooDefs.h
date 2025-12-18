#pragma once
#include <stdint.h>
#include <unistd.h>
#include "ARM.h"

enum _resTypes
{
	FAILURE = 0,
	INTERCEPT_SUCCESS,
	INTERCEPT_FAIL,
	ATTACH_SUCCESS,
	ATTACH_FAIL,
	DETACH_SUCCESS,
	DETACH_FAIL,
	LOAD_ARG_SUCCESS,
	LOAD_ARG_FAIL,
	PROTECT_REGION_SUCCESS,
	PROTECT_REGION_FAIL,
	WRITER_SUCCESSFUL,
	WRITER_UNSUCCESSFUL,
	RESTORE_SUCCESS,
	RESTORE_FAIL,
	CREATE_NATIVE_SUCCESS,
	CREATE_NATIVE_FAIL

};

typedef uint8_t byte;
typedef uint32_t instruction;
typedef uint64_t address;
typedef uint16_t roo_return_t;
typedef int va_length;
typedef bool roo_boolean;

#define MAX_PARAM_SIZE = 512;

#define INT2_MASK 0x00000003U
#define INT3_MASK 0x00000007U
#define INT4_MASK 0x0000000fU
#define INT5_MASK 0x0000001fU
#define INT6_MASK 0x0000003fU
#define INT8_MASK 0x000000ffU
#define INT10_MASK 0x000003ffU
#define INT11_MASK 0x000007ffU
#define INT12_MASK 0x00000fffU
#define INT14_MASK 0x00003fffU
#define INT16_MASK 0x0000ffffU
#define INT18_MASK 0x0003ffffU
#define INT19_MASK 0x0007ffffU
#define INT24_MASK 0x00ffffffU
#define INT26_MASK 0x03ffffffU
#define INT28_MASK 0x0fffffffU
#define INT32_MASK 0xffffffffU

//ARM:
#define ARM_INSTR_SIZE = 4;
static const int _PAGE_SIZE = (int)sysconf(_SC_PAGE_SIZE);