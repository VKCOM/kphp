---
sort: 2
---

# Invoking MySQL

```danger
:(
```

KPHP implements some *mysqli_…()* functions, that can be called MySQL support somehow.

But, at VK.com, we use a special *db-proxy* layer between KPHP and MySQL databases. It is a daemon running on every server: it manages connections, performs master-replica routing, handles access rights.

As a result, KPHP just skips credentials passed to *mysqli_connect()*. Moreover, MySQL connection can only be opened to *localhost* with unix socket.

So, MySQL support for "real-world usage" is also still to be done.
