@ok
KPHP_ENABLE_MODULITE=1
KPHP_COMPOSER_ROOT={dir}
<?php
require_once __DIR__ . '/vendor/autoload.php';
require_once __DIR__ . '/Utils007/utils007.php';

global $global_map;

define('MONOLITH_CONST', 'cc');

echo START_MODE, "\n";
\VK\Strings\StringBuilder007::$inited = true;
\VK\RPC\RpcQuery007::makeQuery('dot');
\VK\RPC\RpcLogger007::callUtils();
\VK\RPC\Utils007\Hasher007::callRpcLogger();
\VK\Strings\StringErrBase007::printCurDesc();
\VK\RPC\RpcErr007::printCurDesc();
utilsSummarize();
\Engines\Messages\Channels\ChannelsRep::createChannelsRep()->logWith('global');
