<?php

namespace Engines\Messages\Core;

class ClusterConnection {
    static private int $count = 0;

    public int $id;

    public function __construct() {
        $this->id = ++self::$count;
    }

    static public function checkValid() {}
}
