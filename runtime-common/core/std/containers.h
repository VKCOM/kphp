//  Compiler for PHP (aka KPHP)
//  Copyright (c) 2024 LLC «V Kontakte»
//  Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <deque>
#include <functional>
#include <list>
#include <map>
#include <queue>
#include <stack>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace kphp {

namespace stl {

template<class Key, class T, template<class> class Allocator, class Hash = std::hash<Key>, class KeyEqual = std::equal_to<Key>>
using unordered_map = std::unordered_map<Key, T, Hash, KeyEqual, Allocator<std::pair<const Key, T>>>;

template<class T, template<class> class Allocator, class Hash = std::hash<T>, class KeyEqual = std::equal_to<T>>
using unordered_set = std::unordered_set<T, Hash, KeyEqual, Allocator<T>>;

template<class Key, class Value, template<class> class Allocator, class Cmp = std::less<Key>>
using map = std::map<Key, Value, Cmp, Allocator<std::pair<const Key, Value>>>;

template<class Key, class Value, template<class> class Allocator, class Cmp = std::less<Key>>
using multimap = std::multimap<Key, Value, Cmp, Allocator<std::pair<const Key, Value>>>;

template<class T, template<class> class Allocator>
using deque = std::deque<T, Allocator<T>>;

template<class T, template<class> class Allocator>
using queue = std::queue<T, std::deque<T, Allocator<T>>>;

template<class T, template<class> class Allocator>
using list = std::list<T, Allocator<T>>;

template<class T, template<class> class Allocator>
using vector = std::vector<T, Allocator<T>>;

template<class T, template<class> class Allocator>
using stack = std::stack<T, vector<T, Allocator>>;

template<template<class> class Allocator>
using string = std::basic_string<char, std::char_traits<char>, Allocator<char>>;

} // namespace stl

} // namespace kphp
