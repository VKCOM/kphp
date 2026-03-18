<?php

class Logger {
    public static function log(string $message): void {
        echo "LOG: " . $message . "\n";
    }
}
