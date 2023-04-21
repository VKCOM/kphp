@kphp_should_fail
/in global scope/
/restricted to use global \$in_plain_115, it's not required by @plain/
/in Post115::demo/
/restricted to use global \$somehow, it's not required by @feed/
/in a lambda inside internal/
/global \$gg;/
/restricted to use global \$gg, it's not required by @plain/
/global \$g115, \$somehow;/
/restricted to use global \$g115, it's not required by @feed\/infra/
/restricted to use global \$somehow, it's not required by @feed\/infra/
<?php

\Feed115\Post115::demo();

require_once 'plain/plain.php';
plain_115();

