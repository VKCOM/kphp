---
sort: 2
title: nginx streaming to KPHP
---

# Preferred setup: nginx streaming to KPHP

Usually, you'll set up nginx in front of KPHP. Then KPHP is just a `proxy_pass` backend for nginx.

Remember, that when you launch *N* workers — KPHP can serve *N* requests simultaneously.   
The master process opens `--http-port/-H`, allowing workers to accept connections on it, as they are forks. 

**If all workers are busy**, this connection is put into a system backlog and will be handled once any worker becomes free. This backlog has a system-defined maximum size, and when overflowed, all new connections would be denied. For an external observer (nginx in our case), sending a request to *all-workers-busy-KPHP* seems like "takes a long time to connect", so you can configure connection timeout to handle this situation somehow.

For instance, if you have many KPHP-backends and one or many nginx-frontends (on separate servers), you can use `proxy_next_upstream` for balancing between KPHP.

