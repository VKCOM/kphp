---
sort: 2
---

# Invoking MySQL

Basic [PDO](https://www.php.net/manual/en/book.pdo.php) interface is supported for communication with MySQL databases.
You can see the actual interface [here]({{site.url_PDO}}).

With it you can execute simple text SQL queries:
```php
function sample(string $mysql_host, int $mysql_port, string $dbname, string $user, string $password) {
  // Create the connection:
  $db = new PDO(
    "mysql:host=$mysql_host;port=$mysql_port;dbname=$dbname",
    $user,
    $password,
    [PDO::ATTR_TIMEOUT => 2]  // Only PDO::ATTR_TIMEOUT option is supported
  );
  
  // Send simple SELECT like text query:
  $stmt = $db->query("SELECT * FROM SomeTable");
  if ($db->errorCode() == 0) {  // Simple error handling
    var_dump($db->errorInfo());
    return;
  }
  $res = $stmt->fetchAll();
  var_dump($res);
  
  // Send simple modify text query:
  $affected_rows = $db->exec("DELETE FROM SomeTable WHERE id=42");
  var_dump($affected_rows);
}
```

**Note:** Only default behaviour is supported for exposed methods.  
Particularly, it means that features like _Prepared statements_, _Unbuffered fetching mode_ are not supported so far.

**Note:** But the advantage of this PDO::MySQL version is asynchronicity. The main communication methods like `PDO::query` and `PDO::exec` are marked as _resumable_ functions.  
It means that these method calls will be considered as suspendable points by other resumable functions (a.k.a. coroutines).

Also KPHP implements some `mysqli_…()` functions, that can be called MySQL support somehow.

But at VK.com, we use a special *db-proxy* layer between KPHP and MySQL databases. It is a daemon running on every server: it manages connections, performs master-replica routing, handles access rights.

As a result, KPHP just skips credentials passed to *mysqli_connect()*. Moreover, MySQL connection can only be opened to *localhost* with unix socket.
