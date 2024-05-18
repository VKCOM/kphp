@ok php8
<?php

trait LoggerTrait{
    public function log(string $s) {
        echo "[logger] " . $s . "\n";
    }
}

enum MyBool {
    use LoggerTrait;
    case T;
    case F;
}

$truth = MyBool::T;

$truth->log("sample");
