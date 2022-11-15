@ok
KPHP_ENABLE_MODULITE=1
<?php
#ifndef KPHP
require_once 'kphp_tester_include.php';
#endif

// what is this test about:
// we use only one class in PHP code,
// but a modulite exports multiple — so, others are unreachable
// previously, this gave an error in KPHP, because KPHP didn't see that classes and raised "Unknown class" for them
// now it works, because "export" from modulites are also considered in collect requires pass
// this is quite important, because Modulite plugin auto-generates config from all sources,
// and some of them may not be reachable at the moment of generation (some wip yet unused code)
echo Utils010\Strings010::class, "\n";
