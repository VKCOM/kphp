<?php

class GlobWithCtor009 {
    static private int $count = 0;

    public function __construct() {
        self::$count++;
    }

    static public function printInstancesCount() {
        echo "count of GlobWithCtor009 = ", self::$count, "\n";
    }
}
