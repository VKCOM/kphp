---
sort: 2
---

# Graceful restart

Assume we are deploying a new version of our web-site (a new compiled KPHP binary). A previous (current) version is already launched and running.

When executed with the same `--http-port`, the new KPHP instance doesn't kill the old KPHP process immediately. Instead, it allows currently busy workers to be finished: it starts **accepting new connections** on a specified port. An old binary is automatically shut down when all its workers become inactive. 

KPHP doesn't guide you to versioning binaries and removing old ones. You organize it depending on your needs, taking possible rollbacks into account.  
