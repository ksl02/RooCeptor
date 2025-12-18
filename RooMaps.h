#pragma once
#include "RooDefs.h"

class RooMaps
{
public:
	static address getModuleBase(const char* moduleName);
	static void getModuleRange(const char* moduleName, long* start_addr, long* end_addr);
};

