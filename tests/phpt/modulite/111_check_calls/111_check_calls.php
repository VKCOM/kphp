@kphp_should_fail
KPHP_ENABLE_MODULITE=1
/restricted to call Utils111\\Hidden111::demo\(\), it's internal in @utils/
/restricted to call globalDemo\(\), it's not required by @feed/
/restricted to call Messages111\\Core111\\Core111::demo1\(\), @msg\/core is internal in @msg/
/restricted to call Utils111\\Strings111::hidden1\(\), it's internal in @utils/
/restricted to call Utils111\\Hidden111::demo\(\), it's internal in @utils/
/restricted to call Feed111\\Post111::demo\(\), @feed is not required by @msg/
/restricted to call Utils111\\Strings111::normal\(\), @utils is not required by @msg/
/restricted to call Utils111\\Hidden111::demo2\(\), it's internal in @utils/
/restricted to call Utils111\\Strings111::hidden2\(\), it's internal in @utils/
/restricted to call Child3::hidden3Func\(\), it's internal in @parent\/child1\/child2\/child3/
/restricted to call Child3::hidden3Func\(\), @parent\/child1\/child2 is internal in @parent\/child1/
/restricted to call plainPublic1\(\), @plain is not required by @feed/
/restricted to call plainHidden1\(\), @plain is not required by @feed/
/restricted to call plainHidden2\(\), it's internal in @plain/
/new Utils111\\Hidden111;/
/restricted to use Utils111\\Hidden111, it's internal in @utils/
/restricted to call GlobalCl::staticFn\(\), it's not required by @utils/
/restricted to call Utils111\\Impl111\\Hasher111::calc1\(\), @utils\/impl is internal in @utils/
/restricted to call Utils111\\Impl111\\Hasher111::calc2\(\), @utils\/impl is internal in @utils/
/restricted to call Utils111\\Strings111::hiddenGeneric<T>\(\), it's internal in @utils/
<?php

require_once __DIR__ . '/plain/plain.php';

function globalDemo() {
    Utils111\Strings111::hidden2();
    Utils111\Strings111::normal();
    plainHidden2();
    new Utils111\Hidden111;
}

globalDemo();
Feed111\Post111::demo();
Messages111\User111::demo();
Feed111\Post111::demoRequireUnexported();
Utils111\Impl111\Hasher111::calc2();
Feed111\Post111::demoCallInternalGeneric();
