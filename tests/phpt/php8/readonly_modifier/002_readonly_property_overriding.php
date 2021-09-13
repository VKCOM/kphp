@kphp_should_fail
/You may not override field: `prop_readonly`, in class: `Boo`/
/Cannot redeclare readonly Foo::prop_readonly as writeable Boo::prop_readonly/
/Cannot redeclare writeable Goo::prop_readonly as readonly Doo::prop_readonly/
<?php

class Foo {
    public readonly string $prop_readonly = "hello";
}

class Boo extends Foo {
    public string $prop_readonly = "hello";
}

class Goo {
    public string $prop_readonly = "hello";
}

class Doo extends Goo {
    public readonly string $prop_readonly = "hello";
}
