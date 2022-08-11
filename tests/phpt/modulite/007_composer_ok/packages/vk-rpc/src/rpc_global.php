<?php

define('GLOBAL_DEF_IN_RPC', 0);

function globalFunctionInVkRpc() {}

if (0) {
    class my_tl_f implements \VK\TL\RpcFunction {
        function getTLFunctionName(): string { return "my_tl_f"; }
    }
}

