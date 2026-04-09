Player flow:
  1. Navigate to http://<device>:8080/ → see the dark-themed login pane
  2. Dump the firmware → find AUTH_PASS_KEY and AUTH_PASS_ENC byte arrays
  3. XOR them together → n4bby_sp3ctral
  4. Login with admin / n4bby_sp3ctral → session cookie set → access Spectral Interface
  5. Exploit the /proc/<pid> command injection behind auth
