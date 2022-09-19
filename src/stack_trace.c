#include "stack_trace.h"

#include "debug.h"
#include "tlsf/tlsf.h"

#include <stddef.h>
#include <stdio.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <DbgHelp.h>

#define MAX_STACK_TRACE_FRAME_COUNT 16

typedef struct stack_trace_t
{
	void* address;
	void* stack_trace[MAX_STACK_TRACE_FRAME_COUNT];
	int  frame_count;
	struct stack_trace_t* next;
} stack_trace_t;

stack_trace_t* stack_trace_create(void* address, stack_trace_t* next_stack_trace_node)
{
	stack_trace_t* stack_trace = VirtualAlloc(NULL,
		sizeof(stack_trace_t) + tlsf_pool_overhead(),
		MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

	if (!stack_trace)
	{
		debug_print(
			k_print_error,
			"OUT OF MEMORY FOR STACK TRACE!\n");
		return NULL;
	}

	stack_trace->address = address;
	stack_trace->frame_count = CaptureStackBackTrace(2, MAX_STACK_TRACE_FRAME_COUNT, stack_trace->stack_trace, NULL);
	stack_trace->next = next_stack_trace_node;

	return stack_trace;
}

void stack_trace_destroy(stack_trace_t* source_stack_trace)
{
	stack_trace_t* stack_trace = source_stack_trace;
	while (stack_trace)
	{
		stack_trace_t* next_stack_trace = stack_trace->next;
		VirtualFree(stack_trace, 0, MEM_RELEASE);
		stack_trace = next_stack_trace;
	}
}

stack_trace_t* stack_trace_find_by_address(stack_trace_t* source_stack_trace, void* address)
{
	stack_trace_t* stack_trace = source_stack_trace;
	while (stack_trace != NULL)
	{
		stack_trace_t* next_stack_trace = stack_trace->next;

		if (stack_trace->address == address)
		{
			return stack_trace;
		}

		stack_trace = next_stack_trace;
	}
	return NULL;
}

void stack_trace_log(stack_trace_t* source_stack_trace)
{
	if (source_stack_trace)
	{
		const DWORD64* stack = (DWORD64*)source_stack_trace->stack_trace;
		int frame_count = source_stack_trace->frame_count;

		HANDLE process = GetCurrentProcess();
		SymInitialize(process, NULL, TRUE);

		DWORD sym_options = SymGetOptions();
		sym_options |= SYMOPT_LOAD_LINES | SYMOPT_UNDNAME;
		SymSetOptions(sym_options);

		char buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(TCHAR)];
		PIMAGEHLP_SYMBOL64 p_symbol = (PIMAGEHLP_SYMBOL64)buffer;

		p_symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
		p_symbol->MaxNameLength = MAX_SYM_NAME;

		DWORD displacement;
		IMAGEHLP_LINE64 line;
		for (int i = 0; i < frame_count; i++)
		{
			DWORD64 address = (DWORD64)(stack[i]);
			DWORD64 sym_addr_displacement;

			SymGetSymFromAddr64(process, address, &sym_addr_displacement, p_symbol);

			if (strcmp(p_symbol->Name, "invoke_main") == 0)
			{
				break;
			}

			if (SymGetLineFromAddr64(process, address, &displacement, &line))
			{
				IMAGEHLP_LINE64 line;

				SymGetSymFromAddr64(process, address, &sym_addr_displacement, p_symbol);
				if (SymGetLineFromAddr64(process, address, &displacement, &line))
				{
					debug_print(k_print_error, "[%d] %s\n", i, p_symbol->Name);
				}
			}
		}

		SymCleanup(process);
	}
}
