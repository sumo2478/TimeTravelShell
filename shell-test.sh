# Test simple command
echo hello world!
ls -rejk

# Test input/output redirection
echo this is an input test > n_file
cat < n_file
echo another message > n_file
cat < n_file

cat < n_file >other_file

# Test Pipe command
cat n_file | cat
who | sort > other_file
cat other_file
sort < other_file | cat > n_file
cat n_file

(cat n_file | cat) && echo first pipe test
(ls -rexfdjk | cat ) && echo second pipe test
(cat n_file | ls -rexfjkds) && echo third pipe test
(echo hello && echo bob) > n_file && echo final
cat < n_file

rm n_file
rm other_file

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

# Test SUBSHELL command
(echo subshell command)
(echo first shell || echo second shell) && echo outside shell
(echo first) && ( echo out)
(echo first &&echo second || (ls -rejk && echo should not output))

# Test SEQUENCE command
echo first sequence; echo second sequence; echo final sequence
(echo second sequence test ; echo middle of second sequence) && echo end of second sequence

