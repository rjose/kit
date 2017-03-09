## \file tasks.forth
#
# \brief App for managing tasks

lex-tasks

# TODO: Figure out why this doesn't work
#: foreach   map pop ;

#
# ======================================
# Integrate with notes
# ======================================

## Redefines a note word like N, S, M, and E so that they link the note to the
#  current task.
#
# (note-word -- )
: redefine-note-word
       "pop
        : `0   `0
                cur-task-id @
                tasks-db @ sqlite3-last-id
                link-note
        ;" ,
;


[ "N" "S" "M" "E" ] "redefine-note-word" map pop


## ======================================
## Printing notes
## ======================================
#
### Prints notes for current chunk of work
#: c  notes-last-chunk . ;
#
### Prints notes logged today
#: today  notes-today . ;
#
### Prints time since latest 'S' or 'E' note
##  This is useful because it shows the elapsed time for a chunk of work
##  or the amount of time of a break.
#: t  time ;


### Prints all notes associated with a task
## (task -- [Note])
#: notes     notes-for . ;
#
## FYI: Here's how to push a task onto the stack: <id> Task
#
### Prints hierarchy of a task
##  (Task -- )
#: ph    descendants
#        "'value' field negate" sort
#        "parent-id" to-hierarchy
#        . ;


### Prints hierarchy of a task
##  (task-id -- )
#: ph    hierarchy "task_value" descending print-task-hierarchy ;
#
#
### Prints hierarchy of a task (incomplete tasks)
##  (task-id -- )
#: pih    hierarchy incomplete "task_value" descending print-task-hierarchy ;
#
#
### Prints ancestors of current task ('w'here)
#: w   ancestors print-task-hierarchy ;
#
### Prints children of current task
#: sub   children
#        "task_value" descending
#        print-tasks ;
#
### (str -- tasks)
#: /     search
#        "task_value" descending
#        print-tasks ;
#
### Lists all incomplete tasks
#: todo    all incomplete
#          "task_value" descending
#          print-task-hierarchy
#          ;
#
#: to      todo ;
#
### Lists all incomplete top level tasks
#: l1    level-1 incomplete
#        "task_value" descending
#        print-tasks ;
#
### Prints all tasks as a tree
#: ap    all "task_value" descending print-task-hierarchy ;
#
#
## ======================================
## Updating tasks
## ======================================
#
### Helper function to pull fresh cur-task data from db after a change
#: refresh-cur-task  *cur-task @ task_id g ;
#
### Marks current task as done
#: x
#    *cur-task @ task_id   # (task id)
#    1 is-done!            # (task)
#    pop                   # ()
#    refresh-cur-task
#    "Marking task complete" N
#;
#
### Marks current task as not done
#: /x
#    *cur-task @ task_id   # (task id)
#    0 is-done!            # (task)
#    pop                   # ()
#    refresh-cur-task
#    "Marking task incomplete" N
#;
#
### Marks specified task as done
##  (task-id -- )
#: X   1 is-done! ;
#
### Marks specified task as undone
##  (task-id -- )
#: /X   0 is-done! ;
#
#
## ======================================
## Misc
## ======================================
#
### Redefine 'm'ove task so the cur-task gets updated
#: m    m refresh-cur-task ;
#
## Redefines .q to close the databases first
: .q   close-db .q ;
#
#
### Goes to the task with the most recent note
#: active    last-active-id g ;
#
#
### Move task to a thought or a bug
## (task-id -- )
#: bug    bug-tasks-id m ;
#: th    thought-tasks-id m ;
#: misc  misc-tasks-id m ;


# ======================================
# STARTUP
# ======================================

# Open the databases
open-db

# Go to last active task
# active

# 0 T .

# 0 T "This is a task" ++
# Become interactive
#.i

all "id" "parent_id" forest .
.q
