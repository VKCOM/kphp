// MemCache

memcache.not_found#32c42422 = memcache.Value;
memcache.strvalue#a6bebb1a value:string flags:int = memcache.Value;

---functions---

// MemCache

@read memcache.get#d33b13ae key:string = memcache.Value;

@write memcache.set#eeeb54c4 key:string flags:int delay:int value:string = Bool;
