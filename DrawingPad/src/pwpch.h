#pragma once

#ifdef _DEBUG
#define DBG_NEW new( _NORMAL_BLOCK, __FILE__, __LINE__)
#include <cstdlib>
#include <crtdbg.h>
#else
#define DBG_NEW new
#endif

#include <iostream>
#include <memory>
#include <algorithm>
#include <functional>
#include <list>

#include <string>
#include <sstream>
#include <vector>
#include <unordered_map>
#include <unordered_set>