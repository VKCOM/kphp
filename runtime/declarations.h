#pragma once

struct array_size;

template<class T>
class array;

template<class T>
class class_instance;

class var;

class Unknown {
};

class string;

class string_buffer;

template<typename T>
class Optional;

struct rpc_connection;

template<class LongT>
class LongNumber;

using ULong = LongNumber<unsigned long long>;
using Long  = LongNumber<long long>;
using UInt  = LongNumber<unsigned int>;

