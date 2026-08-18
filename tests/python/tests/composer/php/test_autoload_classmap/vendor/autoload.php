<?php

// autoload.php — simple classmap loader for test fixture
// KPHP recognises this file by path and skips it; PHP executes it.

spl_autoload_register(function(string $class): void {
    static $map = [
        'Logger'        => __DIR__ . '/../src/Logger.php',
        'Api\\ApiClient' => __DIR__ . '/../src/Api/ApiClient.php',
    ];
    if (isset($map[$class])) {
        require_once $map[$class];
    }
});
