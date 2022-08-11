<?php

namespace VK\RPC;

class Rpc124 {
    static public function useStrings() {
        \VK\Strings\Strings124::$count++;
        \VK\Strings\Impl124\Hidden124::$inited = true;
        \VK\Strings\Impl124\Append124::$inited = true;
        echo \VK\Strings\Strings124::concat('a', 'b'), "\n";
    }
}
