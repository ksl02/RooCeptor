#include "RooMaps.h"
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

address RooMaps::getModuleBase(const char* moduleName) 
{
	FILE* fp;
	unsigned long addr = 0x0;
	char filename[32], buffer[1024];
	snprintf(filename, sizeof(filename), "/proc/%d/maps", getpid());
	fp = fopen(filename, "rt");
	if (fp != NULL) {
		while (fgets(buffer, sizeof(buffer), fp)) {
			if (strstr(buffer, moduleName)) {
				addr = (uintptr_t)strtoul(buffer, NULL, 16);
				break;
			}
		}
		fclose(fp);

	}
	return (address)addr;
}

void RooMaps::getModuleRange(const char* moduleName, long* start_addr, long* end_addr) 
{
	FILE* fp;
	char* pch;
	char filename[32];
	char line[1024];

	*start_addr = 0;
	if (end_addr) {
		*end_addr = 0;
	}

	int pid = getpid();
	if (pid == 0) {
		snprintf(filename, sizeof(filename), "/proc/%d/maps", pid);
	}
	else {
		snprintf(filename, sizeof(filename), "/proc/self/maps");
	}

	fp = fopen(filename, "r");

	if (fp != NULL) {
		while (fgets(line, sizeof(line), fp)) {
			if (strstr(line, moduleName)) {
				pch = strtok(line, "-");
				*start_addr = strtoul(pch, NULL, 16);
				pch = strtok(NULL, "-");
				if (end_addr)
					*end_addr = strtoul(pch, NULL, 16);
				break;
			}
		}

		fclose(fp);
	}
}