<?php

namespace VK\RPC;

class RpcLogger123 {
    static private function log(string $s) {
        echo "rpc log: $s\n";
    }

    static public function callUtils() {
        $hash = Utils123\Hasher123::calcHash('123');
        self::log("hash = $hash");
    }

    static public function logWithHash(string $in) {
        $hash = Utils123\Hasher123::calcHash($in);
        self::log("hash of $in = $hash");
        Utils123\Impl123\HasherImpl123::calc2();
        \VK\Strings\Internals123\Str123\StrSlice123::slice1('asdf');
    }
}
