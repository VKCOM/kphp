<?php

namespace VK\RPC;

class RpcQuery007 {
    const CLUSTER_NAME = 'messages';

    static public function makeQuery(string $prefix) {
        # this is ok, since #vk/rpc "requires" #vk/strings in composer.json
        $b = new \VK\Strings\StringBuilder007($prefix);
        $b->append('.');
        $b->append(self::CLUSTER_NAME);
        $q = $b->get();
        echo "make query ", $q, "\n";

        $q2 = Layer007\QueryBuilder007::alsoMakeQuery($prefix);
        var_dump($q === $q2);
    }
}

