<?php
namespace Utils001;

class Strings001 {
    static public function lowercase(string $s): string {
        static $count = 0;  // not global
        $count++;

        // allow using symbols in "require"
        \GlobalCl::globalMethod();
        echo \GlobalCl::ZERO, "\n";
        echo \GlobalCl::$state, "\n";

        // allow using symbols from a class in "require
        echo \GlobalEnum::RED, ' ', \GlobalEnum::GREEN, "\n";

        // allow using built-in constants, classes, etc.
        if (0) echo PHP_INT_MAX;
        if (0) \JsonEncoder::getLastError();
        if (0) echo \JsonEncoder::rename_policy;
        if (0) new \ArrayIterator([]);
        if (0) instance_cache_delete('');
        $f = function(\Exception $ex) {};

        return strtolower($s);
    }
}
