@kphp_should_fail
KPHP_COMPOSER_ROOT={dir}
/restricted to call VK\\Strings\\Internals123\\Append123::notExportedF\(\), it's internal in #vk\/strings\/@internals/
/in VK\\RPC\\Rpc123::callToStringsInternals/
/restricted to call VK\\Strings\\Internals123\\Append123::notExportedF2\(\), it's internal in #vk\/strings\/@internals/
/restricted to call VK\\RPC\\RpcLogger123::logWithHash\(\), it's not required by #vk\/rpc\/@utils/
/restricted to call VK\\RPC\\Utils123\\Impl123\\HasherImpl123::calcHash\(\), #vk\/rpc\/@utils\/impl is not required by #vk\/rpc\/@utils/
/restricted to call VK\\RPC\\Utils123\\Impl123\\HasherImpl123::calcHash\(\), #vk\/rpc\/@utils\/impl is internal in #vk\/rpc\/@utils/
/restricted to call VK\\RPC\\Utils123\\Impl123\\HasherImpl123::calc2\(\), #vk\/rpc\/@utils\/impl is internal in #vk\/rpc\/@utils/
/restricted to call VK\\Strings\\Internals123\\Str123\\StrSlice123::slice1\(\), #vk\/strings\/@internals\/sub is internal in #vk\/strings\/@internals/
/restricted to call VK\\Strings\\Internals123\\Str123\\StrSlice123::slice2\(\), #vk\/strings\/@internals\/sub is internal in #vk\/strings\/@internals/
<?php
// require_once __DIR__ . '/vendor/autoload.php';

echo \VK\Strings\Internals123\Append123::notExportedF(), "\n";
(new \VK\RPC\Rpc123)->callToStringsInternals();
\VK\RPC\RpcLogger123::callUtils();
\VK\RPC\Utils123\Hasher123::callRpcLogger();
\VK\RPC\Utils123\Impl123\HasherImpl123::calcHash('s');
