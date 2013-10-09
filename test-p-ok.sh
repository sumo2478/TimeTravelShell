#! /bin/sh

# UCLA CS 111 Lab 1 - Test that valid syntax is processed correctly.

tmp=$0-$$.tmp
mkdir "$tmp" || exit

(
cd "$tmp" || exit

cat >test.sh <<'EOF'
true

g++ -c foo.c

: : :

cat < /etc/passwd | tr a-z A-Z | sort -u || echo sort failed!

a b<c > d

cat < /etc/passwd | tr a-z A-Z | sort -u > out || echo sort failed!

a&&b||
 c &&
  d | e && f|

g<h

# This is a weird example: nobody would ever want to run this.
a<b>c|d<e>f|g<h>i

another example # this should be ignored

another example# this should also be ignored; extra info

test_semicolon;this should be printed too

# multiple comments
# another comment
a && b #final comment

areallylongstringabcdefghjiklmnopqwerfasdfjkklsdasdfjklsadfjkldsa ||asdfjksdalfaskdnfkdsnfiaosdnfindifoandf

a &&b; c || d;

((a;b) ||c|d>f);
EOF

cat >test.exp <<'EOF'
# 1
  true
# 2
  g++ -c foo.c
# 3
  : : :
# 4
      cat</etc/passwd \
    |
      tr a-z A-Z \
    |
      sort -u \
  ||
    echo sort failed!
# 5
  a b<c>d
# 6
      cat</etc/passwd \
    |
      tr a-z A-Z \
    |
      sort -u>out \
  ||
    echo sort failed!
# 7
        a \
      &&
        b \
    ||
      c \
  &&
      d \
    |
      e \
  &&
      f \
    |
      g<h
# 8
    a<b>c \
  |
    d<e>f \
  |
    g<h>i
# 9
  another example
# 10
  another example
# 11
  test_semicolon
# 12
  this should be printed too
# 13
    a \
  &&
    b
# 14
    areallylongstringabcdefghjiklmnopqwerfasdfjkklsdasdfjklsadfjkldsa \
  ||
    asdfjksdalfaskdnfkdsnfiaosdnfindifoandf
# 15
    a \
  &&
    b
# 16
    c \
  ||
    d
# 17
  (
     (
        a \
      ;
        b
     ) \
   ||
       c \
     |
       d>f
  )
EOF

../timetrash -p test.sh >test.out 2>test.err || exit

diff -u test.exp test.out || exit
test ! -s test.err || {
  cat test.err
  exit 1
}

) || exit

rm -fr "$tmp"
