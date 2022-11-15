<?php

namespace VK\RPC;

class RpcLogger007 {
    static private function log(string $s) {
        echo "rpc log: $s\n";
    }

    static public function callUtils() {
        global $global_map;
        $hash = Utils007\Hasher007::calcHash('123');
        self::log("hash = $hash");

        \globalFunctionInVkRpc();
        if(0) echo GLOBAL_DEF_IN_RPC_007;

        // allow using built-in constants, classes, etc.
        if (0) echo PHP_INT_MAX;
        if (0) \JsonEncoder::getLastError();
        if (0) echo \JsonEncoder::rename_policy;
        if (0) new \ArrayIterator([]);
        if (0) instance_cache_delete('');
        $f = function(\Exception $ex) {};
    }

    static public function logWithHash(string $in, \Exception $ex = null) {
        $hash = Utils007\Hasher007::calcHash($in);
        self::log("hash of $in = $hash");
        \VK\Strings\Utils007\Str007\StrSlice007::slice2('...');
    }
}

