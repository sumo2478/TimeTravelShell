# Test simple command
echo hello world!

# Test AND command
echo hello && echo bob
echo bob && echo it worked
ls -rejk && echo this should not print

# Test OR command
echo hello || echo this should not print
ls -rejk || echo this should print
