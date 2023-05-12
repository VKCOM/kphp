@kphp_should_fail
/You cannot extends final class: Bar/
<?php

enum Foo {}

class Bar extends Foo {}