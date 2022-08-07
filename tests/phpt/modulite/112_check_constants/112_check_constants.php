@kphp_should_fail
KPHP_ENABLE_MODULITE=1
/in global scope/
/restricted to use Feed112\\Infra112\\Hidden112::HIDDEN_2, it's internal in @feed\/infra/
/restricted to use OTHER_112_CONST, it's internal in @other/
/in Post112::demo/
/restricted to use Feed112\\Infra112\\Strings112::STR_HIDDEN, it's internal in @feed\/infra/
/restricted to use Feed112\\Infra112\\Hidden112::HIDDEN_1, it's internal in @feed\/infra/
/in a lambda inside Hidden112::demo/
/restricted to use DEF_POST_1, it's internal in @feed/
/in other112/
/restricted to use DEF_POST_PUB, @feed is not required by @other/
/restricted to use GLOBAL_DEF, it's not required by @other/
<?php

\Feed112\Post112::demo();
\Feed112\Infra112\Strings112::demo();
echo Feed112\Infra112\Hidden112::HIDDEN_2;

define('GLOBAL_DEF', 1);

require_once 'Other112/other112.php';
other112();

echo DEF_POST_PUB, "\n";
echo OTHER_112_CONST, "\n";

