// Compiler for PHP (aka KPHP)
// Copyright (c) 2023 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime/kphp_core.h"

namespace kphp_tracing {

// this file is included only from kphp_tracing.cpp just to split that file into separate parts
// see kphp_tracing.cpp and kphp_tracing_binlog.cpp for comments

// the same enum, with exact same constant names and exact same values,
// exists in Go decoder
enum class EventTypeEnum {
  etStringsTableRegister = 1,
  etEnumCreateBlank = 2,
  etEnumSetMeaning = 3,

  etProvideTraceContext = 10,
  etProvideMemUsed = 11,
  etProvideMemAdvanced = 12,
  etProvideRuntimeWarning = 13,

  etSpanCreatedRoot = 20,
  etSpanCreatedTitleDesc = 21,
  etSpanCreatedTitleOnly = 22,
  etSpanFinished = 23,
  etSpanFinishedWithError = 24,
  etSpanRename = 25,
  etSpanExclude = 26,

  etSpanAddAttributeString = 30,
  etSpanAddAttributeInt32 = 31,
  etSpanAddAttributeInt64 = 32,
  etSpanAddAttributeFloat64 = 33,
  etSpanAddAttributeFloat32 = 34,
  etSpanAddAttributeBool = 35,
  etSpanAddAttributeInt32Enum = 36,

  etSpanAddEvent = 40,
  etSpanAddLink = 41,

  etFuncCallStarted = 50,
  etFuncCallFinished = 51,
  etFuncAggregateStarted = 52,
  etFuncAggregateBranch = 53,
  etFuncAggregateFinished = 54,

  etRpcQuerySend = 100,
  etRpcQueryGotResponse = 101,
  etRpcQueryFailed = 102,
  etRpcQueryProvideDetails = 103,

  etJobWorkerLaunch = 110,
  etJobWorkerGotResponse = 111,
  etJobWorkerFailed = 112,

  etScriptWaitNet = 120,
  etScriptShuttingDown = 121,
  etCoroutineStarted = 122,
  etCoroutineFinished = 123,
  etCoroutineSwitchTo = 124,
  etCoroutineProvideDesc = 125,

  etCurlStarted = 130,
  etCurlFinished = 131,
  etCurlFailed = 132,
  etCurlAddAttributeString = 133,
  etCurlAddAttributeInt32 = 134,

  etCurlMultiStarted = 140,
  etCurlMultiAddHandle = 141,
  etCurlMultiRemoveHandle = 142,
  etCurlMultiFinished = 143,

  etExternalProgramStarted = 150,
  etExternalProgramFinished = 151,
  etExternalProgramFailed = 152,

  etFileIOStarted = 160,
  etFileIOFinished = 161,
  etFileIOFailed = 162,
};

class tracing_binary_buffer {
  struct one_chunk {
    int *buf;
    size_t size_bytes;
    one_chunk *prev_chunk;
  };

  one_chunk *last_chunk{nullptr};
  int *pos{nullptr};  // points inside cur_chunk->buf
  bool use_heap_memory{false};

public:

  void set_use_heap_memory();
  void init_and_alloc();
  void alloc_next_chunk_if_not_enough(int reserve_bytes);
  void finish_cur_chunk_start_next();
  void clear(bool real_deallocate);

  int get_cur_chunk_size() const { return (pos - last_chunk->buf) * 4; }
  int calc_total_size() const;
  bool empty() const { return last_chunk == nullptr || pos == last_chunk->buf; }

  void write_event_type(EventTypeEnum eventType, int custom24bits);
  void write_int32(int v) { *pos++ = v; }
  void write_uint32(unsigned int v) { *pos++ = v; }
  void write_int64(int64_t v);
  void write_uint64(uint64_t v);
  void write_float32(float v) { *pos++ = *reinterpret_cast<int *>(&v); }
  void write_float64(double v);

  int register_string_if_const(const string &v) {
    if (!v.is_reference_counter(ExtraRefCnt::for_global_const)) {
      return 0;
    }
    return register_string_in_table(v);
  }
  int register_string_in_table(const string &v);

  void write_string(const string &v, int idx_in_table) {
    if (idx_in_table > 0) {
      *pos++ = idx_in_table << 8;
    } else {
      write_string_inlined(v);
    }
  }
  void write_string_inlined(const string &v);

  void append_enum_values(int enumID, const string &enumName, const array<string> &enumKV);
  void append_current_php_script_strings();
  void output_to_json_log(const char *json_without_binlog);
};

extern tracing_binary_buffer cur_binlog;

// this is a stateless "singleton" with high-level events
// methods of this class are called if tracing_level > 0, there are no checks inside
// they all write into cur_binlog
// the same functions, with exact same signatures, exist in Go decoder (kphpBinlogReplay)
struct BinlogWriter {
  static void provideTraceContext(uint64_t int1, uint64_t int2) {
    cur_binlog.write_event_type(EventTypeEnum::etProvideTraceContext, 0);
    cur_binlog.write_uint64(int1);
    cur_binlog.write_uint64(int2);
  }

  static void provideMemUsed(int memoryUsedBytes) {
    // write >> 8 to fit 24 bits next to eventType:
    // for basic trace level, "precision" of 256 bytes is enough
    // for full trace level, advanced mem details are written instead
    cur_binlog.write_event_type(EventTypeEnum::etProvideMemUsed, memoryUsedBytes >> 8);
  }

  static void provideMemAdvanced(int memoryUsedBytes, int allocationsCount, int allocatedTotal) {
    cur_binlog.write_event_type(EventTypeEnum::etProvideMemAdvanced, allocationsCount);
    cur_binlog.write_int32(memoryUsedBytes);
    cur_binlog.write_int32(allocatedTotal);
  }

  static void provideRuntimeWarning(int errorCode, const string &warningMessage) {
    cur_binlog.write_event_type(EventTypeEnum::etProvideRuntimeWarning, errorCode);
    cur_binlog.write_string_inlined(warningMessage);
  }

  static void onSpanCreatedRoot(const string &title, double startTimestamp) {
    int idx1 = cur_binlog.register_string_if_const(title);
    cur_binlog.write_event_type(EventTypeEnum::etSpanCreatedRoot, 1);
    cur_binlog.write_string(title, idx1);
    cur_binlog.write_float64(startTimestamp);
  }

  static void onSpanCreatedTitleDesc(int spanID, const string &title, const string &shortDesc, float timeOffset) {
    int idx1 = cur_binlog.register_string_if_const(title);
    int idx2 = cur_binlog.register_string_if_const(shortDesc);
    cur_binlog.write_event_type(EventTypeEnum::etSpanCreatedTitleDesc, spanID);
    cur_binlog.write_string(title, idx1);
    cur_binlog.write_string(shortDesc, idx2);
    cur_binlog.write_float32(timeOffset);
  }

  static void onSpanCreatedTitleOnly(int spanID, const string &title, float timeOffset) {
    int idx1 = cur_binlog.register_string_if_const(title);
    cur_binlog.write_event_type(EventTypeEnum::etSpanCreatedTitleOnly, spanID);
    cur_binlog.write_string(title, idx1);
    cur_binlog.write_float32(timeOffset);
  }

  static void onSpanFinished(int spanID, float timeOffset) {
    cur_binlog.write_event_type(EventTypeEnum::etSpanFinished, spanID);
    cur_binlog.write_float32(timeOffset);
  }

  static void onSpanFinishedWithError(int spanID, int errorCode, const string &errorMsg, float timeOffset) {
    cur_binlog.write_event_type(EventTypeEnum::etSpanFinishedWithError, spanID);
    cur_binlog.write_int32(errorCode);
    cur_binlog.write_string_inlined(errorMsg);
    cur_binlog.write_float32(timeOffset);
  }

  static void onSpanRenamed(int spanID, const string &title, const string &shortDesc) {
    int idx1 = cur_binlog.register_string_if_const(title);
    int idx2 = cur_binlog.register_string_if_const(shortDesc);
    cur_binlog.write_event_type(EventTypeEnum::etSpanRename, spanID);
    cur_binlog.write_string(title, idx1);
    cur_binlog.write_string(shortDesc, idx2);
  }

  static void onSpanExcluded(int spanID) {
    cur_binlog.write_event_type(EventTypeEnum::etSpanExclude, spanID);
  }

  static void onSpanAddedAttributeString(int spanID, const string &key, const string &value) {
    int idx1 = cur_binlog.register_string_if_const(key);
    cur_binlog.write_event_type(EventTypeEnum::etSpanAddAttributeString, spanID);
    cur_binlog.write_string(key, idx1);
    cur_binlog.write_string_inlined(value);
  }

  static void onSpanAddedAttributeInt32(int spanID, const string &key, int value) {
    int idx1 = cur_binlog.register_string_if_const(key);
    cur_binlog.write_event_type(EventTypeEnum::etSpanAddAttributeInt32, spanID);
    cur_binlog.write_string(key, idx1);
    cur_binlog.write_int32(value);
  }

  static void onSpanAddedAttributeInt64(int spanID, const string &key, int64_t value) {
    int idx1 = cur_binlog.register_string_if_const(key);
    cur_binlog.write_event_type(EventTypeEnum::etSpanAddAttributeInt64, spanID);
    cur_binlog.write_string(key, idx1);
    cur_binlog.write_int64(value);
  }

  static void onSpanAddedAttributeFloat64(int spanID, const string &key, double value) {
    int idx1 = cur_binlog.register_string_if_const(key);
    cur_binlog.write_event_type(EventTypeEnum::etSpanAddAttributeFloat64, spanID);
    cur_binlog.write_string(key, idx1);
    cur_binlog.write_float64(value);
  }

  static void onSpanAddedAttributeFloat32(int spanID, const string &key, float value) {
    int idx1 = cur_binlog.register_string_if_const(key);
    cur_binlog.write_event_type(EventTypeEnum::etSpanAddAttributeFloat32, spanID);
    cur_binlog.write_string(key, idx1);
    cur_binlog.write_float32(value);
  }

  static void onSpanAddedAttributeBool(int spanID, const string &key, bool value) {
    int idx1 = cur_binlog.register_string_if_const(key);
    cur_binlog.write_event_type(EventTypeEnum::etSpanAddAttributeBool, value ? 65536 + spanID : spanID);
    cur_binlog.write_string(key, idx1);
  }

  static void onSpanAddedAttributeInt32Enum(int spanID, const string &key, int enumID, int value) {
    int idx1 = cur_binlog.register_string_if_const(key);
    cur_binlog.write_event_type(EventTypeEnum::etSpanAddAttributeInt32Enum, spanID);
    cur_binlog.write_string(key, idx1);
    cur_binlog.write_int32(enumID);
    cur_binlog.write_int32(value);
  }

  static void onSpanAddedEvent(int spanID, int eventSpanID, const string &name, float timeOffset) {
    int idx1 = cur_binlog.register_string_if_const(name);
    cur_binlog.write_event_type(EventTypeEnum::etSpanAddEvent, spanID);
    cur_binlog.write_int32(eventSpanID);
    cur_binlog.write_string(name, idx1);
    cur_binlog.write_float32(timeOffset);
  }

  static void onSpanAddedLink(int spanID, int anotherSpanID) {
    cur_binlog.write_event_type(EventTypeEnum::etSpanAddLink, spanID);
    cur_binlog.write_int32(anotherSpanID);
  }

  static void onFuncCallStarted(int spanID, const string &fName, float timeOffset) {
    int idx1 = cur_binlog.register_string_in_table(fName);
    cur_binlog.write_event_type(EventTypeEnum::etFuncCallStarted, spanID);
    cur_binlog.write_string(fName, idx1);
    cur_binlog.write_float32(timeOffset);
  }

  static void onFuncCallFinished(int spanID, float timeOffset) {
    cur_binlog.write_event_type(EventTypeEnum::etFuncCallFinished, spanID);
    cur_binlog.write_float32(timeOffset);
  }

  static void onFuncAggregateStarted(int funcCallMask, float timeOffset) {
    cur_binlog.write_event_type(EventTypeEnum::etFuncAggregateStarted, funcCallMask);
    cur_binlog.write_float32(timeOffset);
  }

  static void onFuncAggregateBranch(int funcCallMask) {
    cur_binlog.write_event_type(EventTypeEnum::etFuncAggregateBranch, funcCallMask);
  }

  static void onFuncAggregateFinished(int funcCallMask, float timeOffset) {
    cur_binlog.write_event_type(EventTypeEnum::etFuncAggregateFinished, funcCallMask);
    cur_binlog.write_float32(timeOffset);
  }

  static void onRpcQuerySend(int rpcQueryID, int actorOrPort, unsigned int tlMagic, int bytesSent, float timeOffset, bool isNoResult) {
    cur_binlog.write_event_type(EventTypeEnum::etRpcQuerySend, bytesSent);
    cur_binlog.write_int32(isNoResult ? -rpcQueryID : rpcQueryID);
    cur_binlog.write_uint32(static_cast<unsigned int>(actorOrPort));
    cur_binlog.write_uint32(tlMagic);
    cur_binlog.write_float32(timeOffset);
  }

  static void onRpcQueryGotResponse(int rpcQueryID, int bytesRecv, float timeOffset) {
    cur_binlog.write_event_type(EventTypeEnum::etRpcQueryGotResponse, bytesRecv);
    cur_binlog.write_int32(rpcQueryID);
    cur_binlog.write_float32(timeOffset);
  }

  static void onRpcQueryFailed(int rpcQueryID, int errorCodePositive, float timeOffset) {
    cur_binlog.write_event_type(EventTypeEnum::etRpcQueryFailed, errorCodePositive);
    cur_binlog.write_int32(rpcQueryID);
    cur_binlog.write_float32(timeOffset);
  }

  static void onRpcQueryProvideDetails(const string &details, bool isTypedRpc) {
    cur_binlog.write_event_type(EventTypeEnum::etRpcQueryProvideDetails, isTypedRpc ? 1 : 0);
    cur_binlog.write_string_inlined(details);
  }

  static void onJobWorkerLaunch(int jobID, const string &jobClassName, float timeOffset, bool isNoReply) {
    // it's not const, but they are also from a limited set
    int idx1 = cur_binlog.register_string_in_table(jobClassName);
    cur_binlog.write_event_type(EventTypeEnum::etJobWorkerLaunch, 0);
    cur_binlog.write_int32(isNoReply ? -jobID : jobID);
    cur_binlog.write_string(jobClassName, idx1);
    cur_binlog.write_float32(timeOffset);
  }

  static void onJobWorkerGotResponse(int jobID, float timeOffset) {
    cur_binlog.write_event_type(EventTypeEnum::etJobWorkerGotResponse, 0);
    cur_binlog.write_int32(jobID);
    cur_binlog.write_float32(timeOffset);
  }
  
  static void onJobWorkerFailed(int jobID, int errorCodePositive, float timeOffset) {
    cur_binlog.write_event_type(EventTypeEnum::etJobWorkerFailed, errorCodePositive);
    cur_binlog.write_int32(jobID);
    cur_binlog.write_float32(timeOffset);
  }

  static void onWaitNet(int microseconds) {
    static constexpr int32_t UPPER_BOUND_MICRO_TIME_MASK = 0x00f00000; // 15'728'640
    static constexpr int32_t MAX_MILLI_TIME_MASK         = 0x000fffff; //  1'048'575
    // Here is dynamic precision hack to store wait times > 16 sec in 24 bits:
    //    - if time < UPPER_BOUND_MICRO_TIME_MASK (~15.73 sec) it's stored as microseconds
    //    - else it's stored as milliseconds in lowest 20 bits (e.g. max value is ~ 1048.58 sec)
    // Note: It's compatible with any binlog reader version.
    //       The only problem is that old binlog reader will see values between ~15.73 sec and ~16.78 sec for large wait times
    int wait_time_encoded;
    if (microseconds < UPPER_BOUND_MICRO_TIME_MASK) {
      wait_time_encoded = microseconds;
    } else {
      wait_time_encoded = microseconds / 1000;
      if (wait_time_encoded > MAX_MILLI_TIME_MASK) {
        wait_time_encoded = MAX_MILLI_TIME_MASK;
      }
      wait_time_encoded |= UPPER_BOUND_MICRO_TIME_MASK;
    }
    cur_binlog.write_event_type(EventTypeEnum::etScriptWaitNet, wait_time_encoded);
  }

  static void onScriptShuttingDown(float timeOffset, bool isRegularShutdown) {
    cur_binlog.write_event_type(EventTypeEnum::etScriptShuttingDown, isRegularShutdown ? 1 : 0);
    cur_binlog.write_float32(timeOffset);
  }

  static void onCoroutineStarted(int coroutineID, float timeOffset) {
    cur_binlog.write_event_type(EventTypeEnum::etCoroutineStarted, coroutineID);
    cur_binlog.write_float32(timeOffset);
  }

  static void onCoroutineFinished(int coroutineID, float timeOffset) {
    cur_binlog.write_event_type(EventTypeEnum::etCoroutineFinished, coroutineID);
    cur_binlog.write_float32(timeOffset);
  }

  static void onCoroutineSwitchTo(int coroutineID) {
    cur_binlog.write_event_type(EventTypeEnum::etCoroutineSwitchTo, coroutineID);
  }

  static void onCoroutineProvideDesc(int coroutineID, const string &desc) {
    cur_binlog.write_event_type(EventTypeEnum::etCoroutineProvideDesc, coroutineID);
    cur_binlog.write_string_inlined(desc);
  }

  static void onCurlStarted(int curlHandleID, const string &url, float timeOffset) {
    cur_binlog.write_event_type(EventTypeEnum::etCurlStarted, 0);
    cur_binlog.write_int32(curlHandleID);
    cur_binlog.write_string_inlined(url);
    cur_binlog.write_float32(timeOffset);
  }

  static void onCurlFinished(int curlHandleID, int bytesRecv, float timeOffset) {
    cur_binlog.write_event_type(EventTypeEnum::etCurlFinished, bytesRecv);
    cur_binlog.write_int32(curlHandleID);
    cur_binlog.write_float32(timeOffset);
  }

  static void onCurlFailed(int curlHandleID, int errorCodePositive, float timeOffset) {
    cur_binlog.write_event_type(EventTypeEnum::etCurlFailed, errorCodePositive);
    cur_binlog.write_int32(curlHandleID);
    cur_binlog.write_float32(timeOffset);
  }

  static void onCurlAddedAttributeString(int curlHandleID, const string &key, const string &value) {
    int idx1 = cur_binlog.register_string_if_const(key);
    cur_binlog.write_event_type(EventTypeEnum::etCurlAddAttributeString, 0);  // curlHandleID may not fit 24 bits
    cur_binlog.write_int32(curlHandleID);
    cur_binlog.write_string(key, idx1);
    cur_binlog.write_string_inlined(value);
  }

  static void onCurlAddedAttributeInt32(int curlHandleID, const string &key, int value) {
    int idx1 = cur_binlog.register_string_if_const(key);
    cur_binlog.write_event_type(EventTypeEnum::etCurlAddAttributeInt32, 0);
    cur_binlog.write_int32(curlHandleID);
    cur_binlog.write_string(key, idx1);
    cur_binlog.write_int32(value);
  }

  static void onCurlMultiStarted(int curlMultiID, float timeOffset) {
    cur_binlog.write_event_type(EventTypeEnum::etCurlMultiStarted, 0);
    cur_binlog.write_int32(curlMultiID);
    cur_binlog.write_float32(timeOffset);
  }

  static void onCurlMultiAddHandle(int curlMultiID, int curlHandleID, const string &url, float timeOffset) {
    cur_binlog.write_event_type(EventTypeEnum::etCurlMultiAddHandle, 0);
    cur_binlog.write_int32(curlMultiID);
    cur_binlog.write_int32(curlHandleID);
    cur_binlog.write_string_inlined(url);
    cur_binlog.write_float32(timeOffset);
  }

  static void onCurlMultiRemoveHandle(int curlMultiID, int curlHandleID, int bytesRecv, float timeOffset) {
    cur_binlog.write_event_type(EventTypeEnum::etCurlMultiRemoveHandle, bytesRecv);
    cur_binlog.write_int32(curlMultiID);
    cur_binlog.write_int32(curlHandleID);
    cur_binlog.write_float32(timeOffset);
  }

  static void onCurlMultiFinished(int curlMultiID, float timeOffset) {
    cur_binlog.write_event_type(EventTypeEnum::etCurlMultiFinished, 0);
    cur_binlog.write_int32(curlMultiID);
    cur_binlog.write_float32(timeOffset);
  }

  static void onExternalProgramStarted(int execID, int funcID, const string &command, float timeOffset) {
    cur_binlog.write_event_type(EventTypeEnum::etExternalProgramStarted, funcID);
    cur_binlog.write_int32(execID);
    cur_binlog.write_string_inlined(command);
    cur_binlog.write_float32(timeOffset);
  }

  static void onExternalProgramFinished(int execID, int exitCodePositive, float timeOffset) {
    cur_binlog.write_event_type(EventTypeEnum::etExternalProgramFinished, exitCodePositive);
    cur_binlog.write_int32(execID);
    cur_binlog.write_float32(timeOffset);
  }

  static void onExternalProgramFailed(int execID, int errorCodePositive, float timeOffset) {
    cur_binlog.write_event_type(EventTypeEnum::etExternalProgramFailed, errorCodePositive);
    cur_binlog.write_int32(execID);
    cur_binlog.write_float32(timeOffset);
  }

  static void onFileIOStarted(int fd, bool isWrite, const string &fileName, float timeOffset) {
    cur_binlog.write_event_type(EventTypeEnum::etFileIOStarted, static_cast<int>(isWrite));
    cur_binlog.write_int32(fd);
    cur_binlog.write_string_inlined(fileName);
    cur_binlog.write_float32(timeOffset);
  }

  static void onFileIOFinished(int fd, int bytes, float timeOffset) {
    cur_binlog.write_event_type(EventTypeEnum::etFileIOFinished, bytes);
    cur_binlog.write_int32(fd);
    cur_binlog.write_float32(timeOffset);
  }

  static void onFileIOFailed(int fd, int errorCodePositive, float timeOffset) {
    cur_binlog.write_event_type(EventTypeEnum::etFileIOFailed, errorCodePositive);
    cur_binlog.write_int32(fd);
    cur_binlog.write_float32(timeOffset);
  }
};

void free_tracing_binlog_lib();

} // namespace kphp_tracing
