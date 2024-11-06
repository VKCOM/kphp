@kphp_should_fail
/Class Sample does not implement \\ArrayAccess/
<?php 
class Sample {

}

function test() {
    $obj = new Sample();
    $obj[0] = '123';
}

test();