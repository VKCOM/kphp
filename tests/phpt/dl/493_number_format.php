@ok k2_skip
<?php

#var_dump(number_format(1e80, 0, '', ' '));
#var_dump(number_format(1e300, 0, '', ' '));
#var_dump(number_format(1e320, 0, '', ' '));
#var_dump(number_format(1e1000, 0, '', ' '));

echo "round 0.045 = " . round(0.045, 2) . "\n";
echo "number format 0.045 = " . number_format(0.045, 2) . "\n\n";
echo "round 0.055 = " . round(0.055, 2) . "\n";
echo "number format 0.055 = " . number_format(0.055, 2) . "\n\n";
echo "round 5.045 = " . round(5.045, 2) . "\n";
echo "number format 5.045 = " . number_format(5.045, 2) . "\n\n";
echo "round 5.055 = " . round(5.055, 2) . "\n";
echo "number format 5.055 = " . number_format(5.055, 2) . "\n\n";
echo "round 3.025 = " . round(3.025, 2) . "\n";
echo "number format 3.025 = " . number_format(3.025, 2) . "\n\n";
echo "round 4.025 = " . round(4.025, 2) . "\n";
echo "number format 4.025 = " . number_format(4.025, 2) . "\n\n";

echo number_format(-1234.0, 0, '', '') . "\n";
echo number_format(-1234.9, 0, '', '') . "\n";
echo number_format(1234.0, 0, '', '') . "\n";
echo number_format(1234.9, 0, '', '') . "\n";
echo number_format(-1234.00001, 4, '', '') . "\n";
echo number_format(-1234.99996, 4, '', '') . "\n";
echo number_format(1234.00001, 4, '', '') . "\n";
echo number_format(1234.99996, 4, '', '') . "\n";

echo number_format(1234.5678, 4, '', '') . "\n";
echo number_format(1234.5678, 4, NULL, ',') . "\n";
echo number_format(1234.5678, 4, 0, ',') . "\n";
echo number_format(1234.5678, 4);

	var_dump(number_format(0.0001, 1));
	var_dump(number_format(0.0001, 0));

	echo number_format(0.25, 2, '', '')."\n";
	echo number_format(1234, 2, '', ',')."\n";

	$num = 0+"1234.56";  
	echo number_format($num,2);
	echo "\n";
