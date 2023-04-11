@kphp_should_fail
/in global scope/
/restricted to use Feed113\\Infra113\\Hidden113, it's internal in @feed\/infra/
/in Post113::demo/
/restricted to use Feed113\\Infra113\\Strings113, it's internal in @feed\/infra/
/restricted to use Feed113\\Infra113\\Hidden113, it's internal in @feed\/infra/
/in other113/
/restricted to use Feed113\\Post113, @feed is not required by @other/
/restricted to use Feed113\\Impl113\\Impl113, @feed\/impl is internal in @feed/
/in Strings113::demo/
/restricted to use Feed113\\Impl113\\Inner113\\Inner113, @feed\/impl\/inner is internal in @feed\/impl/
<?php

\Feed113\Post113::demo();
\Feed113\Infra113\Strings113::demo();
echo Feed113\Infra113\Hidden113::$HIDDEN_2;

require_once 'Other113/other113.php';
other113();
