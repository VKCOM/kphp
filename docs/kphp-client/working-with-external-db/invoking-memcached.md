---
sort: 1
---

# Invoking memcache(d)

```danger
:(
```

At VK.com, we have a self-written Memcache engine.  
In KPHP, we RPC protocol (it's much more compact than text MC protocol), coupled with async programming.

That's why KPHP doesn't support standard `Memcached` for now.

Technically, this can be done hastily, but a knotty problem is to make it async.
