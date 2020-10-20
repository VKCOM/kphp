/////
//
// Common Types
//
/////

// Builtin types
int#a8509bda ? = Int;
long#22076cba ? = Long;
float#824dab22 ? = Float;    // 4 bytes -- single precision
double#2210c154 ? = Double;  // 8 bytes -- double precision
string#b5286e24 ? = String;

// Boolean emulation
boolFalse#bc799737 = Bool;
boolTrue#997275b5 = Bool;

// Boolean for diagonal queries
boolStat statTrue:int statFalse:int statUnknown:int = BoolStat;

// Vector
vector#1cb5c415 {t:Type} # [t] = Vector t;
tuple#9770768a {t:Type} {n:#} [t] = Tuple t n;
vectorTotal {t:Type} total_count:int vector:%(Vector t) = VectorTotal t;


statOne key:string value:string = StatOne;
//stat vector:%(Vector %statOne) = Stat;
stat#9d56e6b2 %(Dictionary string) = Stat;

dictionaryField {t:Type} key:string value:t = DictionaryField t;
dictionary#1f4c618f {t:Type} %(Vector %(DictionaryField t)) = Dictionary t;

intKeyDictionaryField {t:Type}
    key:int
    value:t
    = IntKeyDictionaryField t;

intKeyDictionary#07bafc42 {t:Type}
    %(Vector %(intKeyDictionaryField t))
    = IntKeyDictionary t;

longKeyDictionaryField {t:Type}
    key:long
    value:t
    = LongKeyDictionaryField t;

longKeyDictionary#b424d8f1 {t:Type}
    %(Vector %(longKeyDictionaryField t))
    = LongKeyDictionary t;

/////
//
// Result- (Maybe-) types
//
/////

resultFalse#27930a7b {t:Type} = Maybe t;
resultTrue#3f9c8ef8 {t:Type} result:t = Maybe t;


pair {X:Type} {Y:Type} a:X b:Y = Pair X Y;

map {X:Type} {Y:Type} key:X value:Y = Map X Y;

true = True; // this can be used as void type and serialized to empty array in PHP

pid host:int port_pid:int utime:int = Pid;

rpcInvokeReqExtra#f3ef81a9 {flags:#}
  return_binlog_pos:flags.0?%True
  return_binlog_time:flags.1?%True
  return_pid:flags.2?%True
  return_request_sizes:flags.3?%True
  return_failed_subqueries:flags.4?%True
  return_query_stats:flags.6?%True
  // Bits 17, 22, 24 was used in before, but their support was dropped
  wait_binlog_pos:flags.16?long // Perform query only after position in binlog is at least this
  string_forward_keys:flags.18?%(Vector string) // For cluster that are split by string (like pmemcached in some modes) - first specified string is used to choose target, then it is deleted from vector
  int_forward_keys:flags.19?%(Vector long) // First long is used to choose target. Then it is deleted from vector
  string_forward:flags.20?string // Same as string_forward_keys, but it is not deleted
  int_forward:flags.21?long // Same as int_forward_keys, but it is not deleted
  custom_timeout_ms:flags.23?int  // Custom timeout for query
  supported_compression_version:flags.25?int // note, that client support compression, to possibly compress answers
  random_delay:flags.26?double // starting query would be delayed by random number, not grater than given
  = RpcInvokeReqExtra flags;

rpcReqResultExtra#c5011709 {flags:#} 
    binlog_pos:flags.0?long  binlog_time:flags.1?long 
    engine_pid:flags.2?%net.Pid
    request_size:flags.3?int response_size:flags.3?int
    failed_subqueries:flags.4?int
    compression_version:flags.5?int
    stats:flags.6?%(Dictionary string)
    = RpcReqResultExtra flags;


reqError#b527877d {X:Type} error_code:int error:string = ReqResult X;
reqResultHeader#8cc84ce1 {X:Type} flags:# extra:%(RpcReqResultExtra flags) result:X = ReqResult X;
//reqResultFalse {X:Type} = ReqResult X;
_ {X:Type} result:X = ReqResult X;

rpcReqResult#63aeda4e {X:Type} query_id:long result:(ReqResult X) = RpcReqResult X;
rpcReqError#7ae432f5 {X:Type} query_id:long error_code:int error:string = RpcReqResult X;

rpcPong#8430eaa7 ping_id:long = RpcPong;

net.pid ip:int port_pid:int utime:int = net.Pid;
net.expandedPid ip:int port:int pid:int utime:int = net.ExpandedPid;

net.tcp = net.Proto;
net.udp = net.Proto;
net.unix = net.Proto;

net.target host:int port:int = net.Target;

left {X:Type} {Y:Type} value:X = Either X Y;
right {X:Type} {Y:Type} value:Y = Either X Y;

concat {X:Type} {Y:Type} x:X y:Y = Concat X Y;

engine.query {X:Type} query:!X = engine.Query;
engine.queryShortened query:%(VectorTotal int) = engine.Query;

engine.queryResult size:int = engine.QueryResult;
engine.queryResultError error_code:int error_string:string = engine.QueryResult;
engine.queryResultAio = engine.QueryResult;


engine.httpQuery#58300321 fields_mask:#
    uri:fields_mask.0?string
    args:fields_mask.1?%(Dictionary string)
    headers:fields_mask.2?%(Dictionary string)
    = engine.HttpQuery;

engine.httpQueryResponse#284852fc fields_mask:#
    return_code:fields_mask.0?int
    data:fields_mask.1?string
    content_type:fields_mask.2?string
    additional_headers:fields_mask.3?%(Dictionary string)
    = engine.HttpQueryResponse;

engine.reindexStatusNever = engine.ReindexStatus;
engine.reindexStatusRunningOld#ac530b46 pid:int start_time:int = engine.ReindexStatus;
engine.reindexStatusRunning pids:%(Vector int) start_time:int = engine.ReindexStatus;
engine.reindexStatusFailed exit_code:int finish_time:int = engine.ReindexStatus;
engine.reindexStatusSignaled signal:int finish_time:int = engine.ReindexStatus;
engine.reindexStatusDone finish_time:int = engine.ReindexStatus;

// if engine has memory_megabytes memory for metafiles -> how many times disk will be used to load metafiles?
engine.metafilesOneMemoryStat
  memory_megabytes:int
  total_aio_queries:long
  total_aio_bytes:long
  = engine.MetafilesOneMemoryStat;

engine.metafilesStatData#b673669b
  data:%(Vector %engine.MetafilesOneMemoryStat)
  = engine.MetafilesStat;

engine.switchedToMasterMode = engine.SwitchMasterReplicaModeResult;
engine.switchedToReplicaMode = engine.SwitchMasterReplicaModeResult;
engine.alreadyInMasterMode = engine.SwitchMasterReplicaModeResult;
engine.alreadyInReplicaMode = engine.SwitchMasterReplicaModeResult;
engine.switchedToMasterModeForcefully bytes_truncated:long = engine.SwitchMasterReplicaModeResult;

engine.failedToSwitchMode error:string = engine.SwitchMasterReplicaModeResult;

engine.mode#b9b7b6c9 {fields_mask:#} read_enabled:fields_mask.0?Bool write_enabled:fields_mask.1?Bool = engine.ReadWriteMode fields_mask;

engine.binlogPrefix binlog_prefix:string snapshot_prefix:string = engine.BinlogPrefix;

---functions---

@any @internal rpcPing#5730a2df ping_id:long = RpcPong;

@any rpcDestActor#7568aabd {X:Type} actor_id:long query:!X = X;
@any rpcDestActorFlags#f0a5acf7 {X:Type} actor_id:long flags:# extra:%(RpcInvokeReqExtra flags) query:!X = X;
@any rpcDestFlags#e352035e {X:Type} flags:# extra:%(RpcInvokeReqExtra flags) query:!X = X;

@any rpcInvokeReq#2374df3d {X:Type} query_id:long query:!X = RpcReqResult X;

@any @internal engine.switchToMasterMode = engine.SwitchMasterReplicaModeResult;
@any @internal engine.switchToReplicaMode = engine.SwitchMasterReplicaModeResult;

// Same as engine.switchToMasterMode, but with binlog truncation
@any @internal engine.switchToMasterModeForcefully = engine.SwitchMasterReplicaModeResult;

@any engine.getReadWriteMode fields_mask:# = engine.ReadWriteMode fields_mask;

// Only for engines, which use rpc_queries.
@any @internal engine.replaceConfigServer cluster_name:string server_offset:int host:string port:int = True;

//NEVER use this. Only for debugging engines.
@any @internal engine.setVerbosity verbosity:int = True;
@any @internal engine.setVerbosityType type:string verbosity:int = True;
@write @internal engine.setPersistentConfigValue name:string value:int = True;
@any @internal engine.setNoPersistentConfigValue name:string value:int = True; // unreliable: may have different value when indexed with/without snapshot
@write @internal engine.setPersistentConfigArray name:string values:%(Vector int) = True;
@any @internal engine.setNoPersistentConfigArray name:string values:%(Vector int) = True;
@any @internal engine.sendSignal signal:int = True;

// The following queries effects work until engine restarts
@any @internal engine.dumpNextQueries num_queries:int = True;
@any @internal engine.dumpLastQueries = True;
@any @internal engine.dumpForceQueries buffers_pressure_threshold:double = True;

@any @internal engine.sendResponseTo {X:Type} pid:%net.Pid query:!X = Bool;

@any engine.nop  = True;
@read engine.readNop = True; // same as nop, but assumed as read in proxy
@write engine.writeNop = True; // same as nop, but assumed as write in proxy
@any engine.stat = Stat;
@any engine.filteredStat stat_names:%(Vector string) = Stat;
@any engine.statNew = Dictionary string;
@write @internal engine.pushStat#f4b19fa2 fields_mask:# stat:fields_mask.0?%Stat = Bool;
@any engine.count = BoolStat;
@any engine.pid = net.Pid;
@any engine.version = String;

@read @internal engine.setMetafileMemory megabytes:int = BoolStat; // supported only by text && messages now.
@write @internal engine.setFsyncInterval seconds:double = BoolStat; // supported only by messages

// ifeq {X:Type} {Y:Type} cond:!X value:X query:!Y  = Either X (Concat X Y);
// ifneq {X:Type} {Y:Type} cond:!X value:X query:!Y = Either X (Concat X Y);
// colon {X:Type} {Y:Type} x:!X y:!Y = Concat X Y;

@any @internal engine.invokeHttpQuery query:%engine.HttpQuery = engine.HttpQueryResponse;

@any @internal engine.sleep time_ms:int = Bool;
@any @internal engine.asyncSleep time_ms:int = Bool;
@any @internal engine.getReindexStatus = engine.ReindexStatus;

@any @internal engine.getBinlogPrefixes = Vector %engine.BinlogPrefix;

@any @internal engine.enableMetafilesAnalyzer enable:Bool = Bool;
@any @internal engine.getExpectedMetafilesStats = Dictionary engine.MetafilesStat;

@any engine.isProduction = Bool;
