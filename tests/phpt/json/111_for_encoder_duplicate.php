@kphp_should_fail
/@kphp-json for VK\\Utils\\MyJsonEncoder 'rename' is duplicated/
/Class SomeUnknown not found after @kphp-json for/
<?php

require_once 'include/MyJsonEncoder.php';

use VK\Utils\MyJsonEncoder;

class A {
    /**
     * @kphp-json for \VK\Utils\MyJsonEncoder rename = id2
     * @kphp-json for MyJsonEncoder rename = my_id
     * @kphp-json for SomeUnknown rename = my_id
     */
    public int $id = 0;
    /**
     * @kphp-json for SomeUnknown rename = my_id
     */
    public string $name = '';
}

