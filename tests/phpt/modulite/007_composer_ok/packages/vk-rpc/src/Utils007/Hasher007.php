<?php

namespace VK\RPC\Utils007;

class Hasher007 {
    static public function calcHash(string $s): int {
        if (0) echo GLOBAL_DEF_IN_RPC;
        if (0) echo MONOLITH_CONST;
        return intval($s) ^ (2+4+8);
    }

    static public function callRpcLogger() {
        \VK\RPC\RpcLogger007::logWithHash('890');
        \globalFunctionInVkRpc();
        strlen('s');
    }
}
