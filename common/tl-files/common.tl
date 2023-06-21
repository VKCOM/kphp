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

// Dictionaries
dictionaryField {t:Type} key:string value:t = DictionaryField t;
dictionary#1f4c618f {t:Type} %(Vector %(DictionaryField t)) = Dictionary t;

intKeyDictionaryField {t:Type} key:int value:t = IntKeyDictionaryField t;
intKeyDictionary#07bafc42 {t:Type} %(Vector %(intKeyDictionaryField t)) = IntKeyDictionary t;

longKeyDictionaryField {t:Type} key:long value:t = LongKeyDictionaryField t;
longKeyDictionary#b424d8f1 {t:Type} %(Vector %(longKeyDictionaryField t)) = LongKeyDictionary t;

// Maybe
resultFalse#27930a7b {t:Type} = Maybe t;
resultTrue#3f9c8ef8 {t:Type} result:t = Maybe t;


pair {X:Type} {Y:Type} a:X b:Y = Pair X Y;

true = True; // this can be used as void type and serialized to empty array in PHP

stat#9d56e6b2 %(Dictionary string) = Stat;

rpcInvokeReqExtra#f3ef81a9 {flags:#}
  return_binlog_pos:flags.0?%True
  return_binlog_time:flags.1?%True
  return_pid:flags.2?%True
  return_request_sizes:flags.3?%True
  return_failed_subqueries:flags.4?%True
  return_query_stats:flags.6?%True
  no_result:flags.7?%True // Currently for proxy only. Client goes to proxy, it clears this bit and sends query to engine. Client does not wait for answer.
  // Bits 17, 22, 24 was used in before, but their support was dropped
  wait_binlog_pos:flags.16?long // Perform query only after position in binlog is at least this
  string_forward_keys:flags.18?%(Vector string) // For cluster that are split by string (like pmemcached in some modes) - first specified string is used to choose target, then it is deleted from vector
  int_forward_keys:flags.19?%(Vector long) // First long is used to choose target. Then it is deleted from vector
  string_forward:flags.20?string // Same as string_forward_keys, but it is not deleted
  int_forward:flags.21?long // Same as int_forward_keys, but it is not deleted
  custom_timeout_ms:flags.23?int  // Custom timeout for query
  supported_compression_version:flags.25?int // note, that client support compression, to possibly compress answers
  random_delay:flags.26?double // starting query would be delayed by random number, not grater than given
  return_view_number:flags.27?%True // GMS2 related parameter: return view number in response
  = RpcInvokeReqExtra flags;

rpcReqResultExtra#c5011709 {flags:#} 
    binlog_pos:flags.0?long  binlog_time:flags.1?long 
    engine_pid:flags.2?%net.Pid
    request_size:flags.3?int response_size:flags.3?int
    failed_subqueries:flags.4?int
    compression_version:flags.5?int
    stats:flags.6?%(Dictionary string)
    = RpcReqResultExtra flags;


// ReqResult
reqError#b527877d {X:Type} error_code:int error:string = ReqResult X;
reqResultHeader#8cc84ce1 {X:Type} flags:# extra:%(RpcReqResultExtra flags) result:X = ReqResult X;
//reqResultFalse {X:Type} = ReqResult X;
_ {X:Type} result:X = ReqResult X;

rpcReqResult#63aeda4e {X:Type} query_id:long result:(ReqResult X) = RpcReqResult X;
rpcReqError#7ae432f5 {X:Type} query_id:long error_code:int error:string = RpcReqResult X;

rpcPong#8430eaa7 ping_id:long = RpcPong;

net.pid ip:int port_pid:int utime:int = net.Pid;

left {X:Type} {Y:Type} value:X = Either X Y;
right {X:Type} {Y:Type} value:Y = Either X Y;

---functions---

@any rpcDestActor#7568aabd {X:Type} actor_id:long query:!X = X;
@any rpcDestActorFlags#f0a5acf7 {X:Type} actor_id:long flags:# extra:%(RpcInvokeReqExtra flags) query:!X = X;
@any rpcDestFlags#e352035e {X:Type} flags:# extra:%(RpcInvokeReqExtra flags) query:!X = X;

@any rpcInvokeReq#2374df3d {X:Type} query_id:long query:!X = RpcReqResult X;

//NEVER use this. Only for debugging engines.
@any @internal engine.setVerbosity verbosity:int = True;
@any @internal engine.setVerbosityType type:string verbosity:int = True;
@any @internal engine.sendSignal signal:int = True;
@any @internal engine.sleep time_ms:int = Bool;

@any engine.nop  = True;
@read engine.readNop = True; // same as nop, but assumed as read in proxy
@write engine.writeNop = True; // same as nop, but assumed as write in proxy
@any engine.stat = Stat;
@any engine.filteredStat stat_names:%(Vector string) = Stat;
@any engine.count = BoolStat;
@any engine.pid = net.Pid;
@any engine.version = String;

@kphp kphp.pingRpcCluster#a677ee41 id:int = Int;
