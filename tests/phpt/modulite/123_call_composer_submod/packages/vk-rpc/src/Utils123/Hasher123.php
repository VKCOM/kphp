<?php

namespace VK\RPC\Utils123;

class Hasher123 {
    static public function calcHash(string $s): int {
        return Impl123\HasherImpl123::calcHash($s);
    }

    static public function callRpcLogger() {
        \VK\RPC\RpcLogger123::logWithHash('890');
    }
}
