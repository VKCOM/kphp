@kphp_should_fail
KPHP_ENABLE_MODULITE=1
/in global scope/
/restricted to use Feed113\\Infra113\\Hidden113::\$HIDDEN_2, it's internal in @feed\/infra/
/in Post113::demo/
/restricted to use Feed113\\Infra113\\Strings113::\$str_hidden, it's internal in @feed\/infra/
/restricted to use Feed113\\Infra113\\Hidden113::\$HIDDEN_1, it's internal in @feed\/infra/
/in other113/
/restricted to use Feed113\\Post113::\$ONE, @feed is not required by @other/
<?php

\Feed113\Post113::demo();
\Feed113\Infra113\Strings113::demo();
echo Feed113\Infra113\Hidden113::$HIDDEN_2;

require_once 'Other113/other113.php';
other113();
