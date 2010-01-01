@ok
<?php
	// All these constructs get unparsed differently depending on whether the
	// --next-line-curlies option is used
  unset ($a);
  unset ($b);
  unset ($c);
  unset ($d);
  unset ($x);
	unset ($y);
	
	if($x != NULL)
		echo "hi";
	else
		echo "bye";

	if($x != NULL)
		echo "hi";
	elseif($y != NULL)
		echo "ho";
	else
		echo "bo";

	while($c != NULL)
		echo "do something";

	do
		echo "do something else";
	while($c != NULL);

	for($a; $b != NULL; $c)
		echo $d;

	foreach(array (1) as $key => $val)
		echo "booh";

/*	switch($a)
	{
		case 1:
			break;
		case 2:
			break;
		default:
			break;
	}

	try
	{
		echo "do something that could fail";
	}
	catch(FirstException $e)
	{
		echo "deal with the failure";
	}
	catch(SecondException $e)
	{
		echo "and again";
	}

	echo "final statement";*/
?>
