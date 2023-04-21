@kphp_should_fail
KPHP_COMPOSER_ROOT={dir}
/restricted to use MONOLITH_CONST_120, it does not belong to package #vk\/strings/
/restricted to use VK\\RPC\\RpcQuery120, #vk\/rpc is not required by #vk\/strings in composer.json/
<?php
require_once __DIR__ . '/vendor/autoload.php';

define('MONOLITH_CONST_120', 'cc');

$s = new VK\StringBuilder120;
$s->useSymbolsFromMonolith();
