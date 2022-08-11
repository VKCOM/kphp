<?php

namespace Engines\Messages\Core;

class MsgLogger {
    static public function logMessage(?ClusterConnection $conn) {
        self::logMessageInternal($conn);
    }

    static public function logMessageInternal(?ClusterConnection $conn, ?string $postfix = null) {
        echo "msg log ", ($conn ? $conn->id : "null"), ($postfix ?? ''), "\n";
    }
}
