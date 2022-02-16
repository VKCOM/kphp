@kphp_should_fail
/Several readonly modifiers are not allowed/
/Several readonly modifiers are not allowed/
/Several readonly modifiers are not allowed/
<?php

class Foo {
    public readonly readonly string $prop_readonly = "hello";
    public readonly readonly $prop_readonly1 = "hello";
    readonly readonly $prop_readonly1 = "hello";
}
