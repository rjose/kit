lex-sequence

# [ 1 2 3 4 ] .

# [ 1 2 3 4 ] len . .

# [ 2 1 3 7 ] "dup" sort

: dup-neg   dup negate ;

[ 2 1 3 7 ] "dup-neg" sort
