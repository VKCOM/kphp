@ok php8
<?php

$options = [
    'monthly',
    'credit-card',
];

echo match ($options) {
    ['monthly', 'direct-debit'] => 1,
    ['yearly', 'credit-card']   => 2,
    ['monthly', 'credit-card']  => 3,
};
