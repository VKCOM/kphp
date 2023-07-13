@kphp_should_fail
/in global scope/
/restricted to use Feed114\\Infra114\\Strings114, @feed\/infra is internal in @feed/
/in other114/
/restricted to use Feed114\\Post114, @feed is not required by @other/
/restricted to use Feed114\\Infra114\\Hidden114, it's internal in @feed/infra/
/restricted to use Feed114\\More114_2, it's internal in @feed/
/in Hidden114::demo/
/restricted to use Feed114\\More114, it's internal in @feed/
/public \$tup;/
/restricted to use Feed114\\More114_3, it's internal in @feed/
/static \$st;/
/restricted to use Feed114\\More114_4, it's internal in @feed/
/static \$a114;/
/restricted to use GlobalA114, it's not required by @feed\/infra/
/static function demo\(\): \?Infra114\\Hidden114_2/
/restricted to use Feed114\\Infra114\\Hidden114_2, it's internal in @feed\/infra/
/\$object instanceof Glob114/
/restricted to use Glob114, it's not required by @other/
/function other114/
/restricted to use GlobalB114, it's not required by @other/
<?php

if (0) new \GlobalB114;

\Feed114\Post114::demo();
\Feed114\Infra114\Strings114::demo();

require_once 'Other114/other114.php';
other114(null, null);
printGlob114(new Glob114);

/** @var ?\Feed114\Infra114\Strings114 $an */
$an = null;
