<?php

define('GLOBAL_DEF_IN_RPC_007', 0);

function globalFunctionInVkRpc() {}

if (0) {
    class my_tl_f implements \VK\TL\RpcFunction {
        public function getTLFunctionName(): string { return "my_tl_f"; }
        public function getTLFunctionMagic(): int {
          return 0x12345678;
        }

        /**
         * @kphp-inline
         *
         * @return VK\TL\RpcFunctionFetcher
         */
        public function typedStore() {
          return null;
        }

        /**
         * @kphp-inline
         *
         * @return VK\TL\RpcFunctionFetcher
         */
        public function typedFetch() {
          return null;
        }
    }
}

