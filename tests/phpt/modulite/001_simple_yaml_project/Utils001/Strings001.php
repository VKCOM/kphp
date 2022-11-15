<?php
namespace Utils001;

class Strings001 {
    static public function lowercase(string $s): string {
        global $g_count_001;
        static $count = 0;  // not global
        $count++;

        // allow using symbols in "require"
        \GlobalCl001::globalMethod();
        echo \GlobalCl001::ZERO, "\n";
        echo \GlobalCl001::$state, "\n";

        // allow using symbols from a class in "require
        echo \GlobalEnum001::RED, ' ', \GlobalEnum001::GREEN, "\n";

        // allow using built-in constants, classes, etc.
        if (0) echo PHP_INT_MAX;
        if (0) \JsonEncoder::getLastError();
        if (0) echo \JsonEncoder::rename_policy;
        if (0) new \ArrayIterator([]);
        if (0) instance_cache_delete('');
        $f = function(\Exception $ex) {};

        return strtolower($s);
    }

    /**
     * @param ?tuple(int, \GlobalA001) $arg
     * @return ?shape(x: string, y: ?\GlobalB001[])
     */
    static public function hasPhpdoc($arg) {
        return null;
    }
}
