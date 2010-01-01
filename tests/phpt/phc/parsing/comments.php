@ok
<?php
	$a = /* nested */ 1 + /* comment */ 2;

	echo 1;		// (1) Single line comment not at the start of the line
	echo 2;		# (2) Alternative style
	
	/*
	 * Statements
	 */

	// (3) Single line comment at the start of the line
	echo 3;

	// (4) Alternative style line comment at the start of the line
	echo 4;

	echo 41; /* comment 41 */ echo 42; /* echo 42 */
	echo /* 43 */ 43, /* 44 */ 44;

	echo 5;

	$b = null;
	// Some assignment
	$a = $b;

	// Non-expression statement
	echo "break 5"; // moved to comments2.php

	/*
	 * (6) Other constructs 
	 */
	
	// (7) Some function definition
	function f7()
	{
		echo "In f7 (without comment)";
	}

	// (12) if-statement
	if(true)
	{
		echo 13;
	}
	// Comment for the else part
	else
	{
		// And another comment
		echo 14;
	}

	// (15) while-statement
	while(true)
	{
		echo 16;
		break;
	}

	// (17) do-while statement
	do
	{
		echo 18;
		break;
	}
	while(true);

	// 19 for loop
	for(19;false;17)
	{
		echo 20;
	}

	// 21 foreach
	foreach(array (1) as $x)
	{
		echo 22;
	}

/*	// 23 switch
	switch(23)
	{
		// Case 1
		case 1:
			break;
		// Case 2
		case 2:
			break;
		// Default case
		default:
			break;
	}
	// 25 Try statement
	try
	{
		25;
	}
	// First catch
	catch(FirstException $e1)
	{
//		break;
	}
	// Second catch
	catch(SecondException $e2)
	{
//		break;
	}
*/

	// 26 Preceding if-comment
	if(true) // 26 Same-line if comment
	{
		echo "break 26";
	}

	// 27 This function has 
	// 28 more than one comment 
	function f()
	{
		return 27 + 28;
	}

	/* Multiline comment */
	echo "break 29";

	/* Semi Nested /* multiline comment */ 
	echo "break 30";

	// More than one in-line comment
	echo "break 31"; /* comment 1 */ /* comment 2 */

	echo "break 32";

	// This no longer breaks
	$x = /* one */ 33 + /* two */ 34;

	/*
	 * So does this
	 */
	echo "break 35";
?>
