@kphp_should_fail k2_skip
/Called internal function _by_name_call_method\(\)/
/Called internal function _by_name_construct\(\)/
<?php

class A {  /** @kphp-required */ static function method(){} }

_by_name_call_method('A', 'method');
_by_name_construct();

