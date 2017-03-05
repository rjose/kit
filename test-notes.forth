lex-notes

## Opens a database connection and stores it in the variable notes-db
: notes-open  "notes.db" sqlite3-open   notes-db ! ;

## Closes a database connection
: notes-close  notes-db @  sqlite3-close ;

## Redefines .q to close the databases first
: .q  notes-close .q ;

## Opens database connections
notes-open

# Become interactive
# .i

today-notes . .q
