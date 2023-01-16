@kphp_runtime_should_warn php8
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
