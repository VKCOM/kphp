@kphp_runtime_should_warn php8
/Unhandled match value 'Array/
<?php

$options = [
    'monthly',
    'credit-card1',
];

echo match ($options) {
    ['monthly', 'direct-debit'] => 1,
    ['yearly', 'credit-card']   => 2,
    ['monthly', 'credit-card']  => 3,
};
