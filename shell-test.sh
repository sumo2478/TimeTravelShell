# Test simple command
echo hello world!

# Test AND command
echo first and test && echo end of first and test
echo second and test && echo end of second and test
ls -rejk && echo this should not print 
echo fourth and test && echo fourth and test && echo fourth end of and test
ls -rejk && echo fifth should not print && echo end of fifth
echo sixth and test && ls -rejk && echo end of sixth and test

# Test OR command
echo first or command || echo end first or command
ls -rejk || echo middle of second or command || echo end of second or test
echo third or command || echo middle third or command || echo end of third or test
ls -rejk || ls -rejk || echo end of fourth or test
echo fifth or test || ls -rejk || echo end of fifth or test

