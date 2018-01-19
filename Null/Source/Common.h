#pragma once

#include "Config.h"

#if NULL_CLIBS
#include <cassert>
#include <cstring>
#include <cstdlib>
#include <ctime>
#include <csignal>
#else
#undef  assert
#define assert(expression) ((void)0)
#endif

#if NULL_STL
#include <algorithm>
#include <functional>
#include <string>
#include <filesystem>
#include <iostream>
// threading
#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>
// containers
#include <vector>
#include <deque>
#include <map>
#include <set>
#include <list>
#include <unordered_map>
#include <unordered_set>
#endif


namespace Null
{
	using uchar = unsigned __int8;
	using i32 = __int32;
	using u32 = unsigned __int32;
	using i64 = __int64;
	using u64 = unsigned __int64;

	using String = std::string;
	template <typename ProtoType>
	using Function = std::function<ProtoType>;

	// Collections
	template <typename First, typename Second>
	using Pair = std::pair<First, Second>;
	template <typename T>
	using Array = std::vector<T>;
	template <typename T>
	using Deque = std::deque<T>;
	template <typename T>
	using List = std::list<T>;
	template <typename Key, typename Value, typename Compare = std::less<Key>, typename Allocator = std::allocator<std::pair<Key, Value>>>
	using Map = std::map<Key, Value, Compare, Allocator>;

	
	constexpr u32 InvalidIdx   = (u32)-1;
	constexpr u64 InvalidIdx64 = (u64)-1;
}
