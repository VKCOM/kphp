@ok
<?php

class MySQL {
  const DB_DATABASE = 'mydb';
}

class KphpConfiguration {
  const DEFAULT_RUNTIME_OPTIONS = [
    "--mysql-db-name" => \MySQL::DB_DATABASE
  ];
}

var_dump(KphpConfiguration::DEFAULT_RUNTIME_OPTIONS);
