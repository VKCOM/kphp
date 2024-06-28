@kphp_should_fail
\Iterable type isn't supported\
<?php
interface Example {
    public function method(array $array): iterable;
}

class ExampleImplementation implements Example {
    public function method(iterable $it): array {
        // Parameter broadened and return narrowed.
        return $it;
    }
}

$arr = [1,2,3];
$obj = new ExampleImplementation();
$obj->method($arr);
