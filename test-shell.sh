./timetrash -t shell-test.sh > output
sh shell-test.sh >expected_output
diff -u expected_output output
#rm output
#rm expected_output
