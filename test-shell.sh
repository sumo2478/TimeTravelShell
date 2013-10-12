./timetrash test_script.sh > output
sh test_script.sh >expected_output
diff -u expected_output output
rm output
rm expected_output
