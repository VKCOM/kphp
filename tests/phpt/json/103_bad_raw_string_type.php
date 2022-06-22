@kphp_should_fail
/Field AA::\$ss1 is @kphp-json 'raw_string', but it's not a string/
/Field AA::\$ss2 is @kphp-json 'raw_string', but it's not a string/
<?php

class AA {
    /** @kphp-json raw_string */
    public ?string $ss1 = '';
    /** @kphp-json raw_string */
    public array $ss2 = [];
}

JsonEncoder::encode(new AA);
