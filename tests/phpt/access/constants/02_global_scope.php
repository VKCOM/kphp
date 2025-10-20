@kphp_should_fail
/Access modifier for const allowed in class only/
<?php
public const PUB = 1;
private const PRIV = 2;
protected const PROT = 3;
echo PUB, PRIV, PROT;
