<?php

namespace VK;

class StringBuilder120 {
    private string $s = '';

    public function __construct() {
    }

    public function useSymbolsFromMonolith() {
        echo MONOLITH_CONST, "\n";
        echo \VK\RPC\RpcQuery120::CLUSTER_NAME, "\n";
    }
}
