@kphp_should_fail
/class InvalidValueTags/
/@kphp-json 'visibility_policy' should be all\|public, got 'hm'/
/public int \$f2;/
/@kphp-json 'skip' should be empty or true\|false\|encode\|decode, got 'hm'/
/class InvalidPlacedTags/
/@kphp-json 'flatten' is duplicated/
/@kphp-json 'skip' is allowed above fields, not above classes/
/public int \$f1;/
/@kphp-json 'visibility_policy' is allowed above classes, not above fields/
/@kphp-json 'flatten' can't be used with some other @kphp-json tags/
/@kphp-json 'skip' can't be used with other @kphp-json tags/
/public int \$f5;/
/@kphp-json 'rename' expected to have a value after '='/
/class VeryInvalidFlatten/
/@kphp-json 'skip_if_default' over a field is disallowed for flatten classes/
/@kphp-json 'flatten' class must have exactly one field/
/@kphp-json 'flatten' class can not extend anything or have derived classes/
<?php

/**
 * @kphp-json flatten
 * @kphp-json flatten
 * @kphp-json skip
 */
class InvalidPlacedTags {
    /** @kphp-json visibility_policy = all */
    public int $f1;
}

/**
 * @kphp-json visibility_policy = hm
 */
class InvalidValueTags {
    /** @kphp-json skip = hm */
    public int $f2;
}

/**
 * @kphp-json visibility_policy = all
 * @kphp-json flatten
 */
class InvalidFlattenWithOthers {
    public int $f3;
}

class InvalidSkipWithOthers {
    /**
     * @kphp-json skip
     * @kphp-json rename = sadf
     */
    public int $f4;
}

class InvalidRename {
    /** @kphp-json rename */
    public int $f5;
}

/**
 * @kphp-json flatten
 */
class VeryInvalidFlatten {
    /** @kphp-json skip_if_default */
    public int $f;
    public int $f2;
}

class Another extends VeryInvalidFlatten {}

