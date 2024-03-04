@kphp_should_fail
KPHP_COMPOSER_ROOT={dir}
/restricted to call concatStr\(\), #vk\/strings is not required by @msg/
/restricted to call rpcFunctionCallingVkStrings\(\), #vk\/rpc is not required by @msg/
/restricted to call concatStr\(\), #vk\/strings is not required by #vk\/rpc/
<?php
require_once __DIR__ . '/vendor/autoload.php';
require_once __DIR__ . '/Messages121/messages121.php';

messagesFunctionCallingVkStrings();
rpcFunctionCallingVkStrings();
