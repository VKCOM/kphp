@kphp_should_fail
KPHP_COMPOSER_ROOT={dir}
/restricted to use VK\\RPC\\RpcLogger124, it's internal in #vk\/rpc/
/restricted to use VK\\Strings\\Strings124, it's internal in #vk\/strings/
/restricted to use VK\\Strings\\Impl124\\Hidden124, it's internal in #vk\/strings\/@impl/
/restricted to use VK\\Strings\\Impl124\\Hidden124, it's internal in #vk\/strings\/@impl/
/restricted to use VK\\Strings\\Impl124\\Append124, #vk\/strings\/@impl is internal in #vk\/strings/
/restricted to use VK\\Strings\\Impl124\\Append124, #vk\/strings\/@impl is internal in #vk\/strings/
<?php
// require_once __DIR__ . '/vendor/autoload.php';

\VK\RPC\RpcLogger124::$log_inited = true;
\VK\Strings\Impl124\Append124::$more = 1;
\VK\RPC\Rpc124::useStrings();
