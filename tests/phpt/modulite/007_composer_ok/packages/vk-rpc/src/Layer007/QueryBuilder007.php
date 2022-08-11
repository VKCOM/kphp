<?php

namespace VK\RPC\Layer007;

class QueryBuilder007 {
    static public function alsoMakeQuery(string $prefix): string {
        # this is ok, since @layer "requires" #vk/strings in .modulite.yaml
        $b = new \VK\Strings\StringBuilder007($prefix);
        $b->append(\VK\Strings\Utils007\Str007\StrSlice007::slice1('...'));
        $b->append(\VK\RPC\RpcQuery007::CLUSTER_NAME);
        return $b->get();
    }
}
