## \file tasks.forth
#
# \brief App for managing tasks

lex-tasks

# TODO: Figure out why this doesn't work
#: foreach   map pop ;


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


# ======================================
# Printing notes
# ======================================

## Prints notes for current chunk of work
: c  notes-last-chunk . ;

## Prints notes logged today
: today  notes-today . ;

## Prints time since latest 'S' or 'E' note
#  This is useful because it shows the elapsed time for a chunk of work
#  or the amount of time of a break.
: t  time ;


## Prints all notes associated with a task
# (Task -- [Note])
#: notes     notes-for . ;
#


# ======================================
# Printing tasks
# ======================================

## Selects tasks that aren't done
# ([Task] -- [Task])
: incomplete  "'is_done' @field not" filter ; 

## Sorts tasks in decreasing value
# ([Task] -- [Task])
: in-decreasing-value   "'value' @field negate"  sort ;

## Converts sequence of tasks to a forest of tasks
# ( [Task] -- Forest)
: as-forest  "id" "parent_id" forest ;

## Lists all incomplete tasks
# ( -- )
: todo    all incomplete in-decreasing-value  as-forest .  ;


## Prints all descendants of a task as a forest (including task)
# ( Task -- )
: ph    descendants in-decreasing-value as-forest . ;


## Prints all incomplete descendants of a task as a forest (including task)
# ( Task -- )
: pih    descendants incomplete in-decreasing-value as-forest . ;


## Prints ancestor chain of a task
# ( Task -- )
# : w    ancestors as-forest . ;

## (str -- tasks)
#: /     search   in-decreasing-value . ;


## Prints all incomplete top level tasks
#: l1    all "'parent_id' @field 0 ==" filter  incomplete in-decreasing-value . ;


# ======================================
# Updating tasks
# ======================================

## Pushes current task onto stack
# ( -- Task)
: cur-task    cur-task-id @ T ;

## Marks a task as complete
# (Task -- Task)
# : X  1 "is_done" !field ;


## Marks a task as not complete
# (Task -- Task)
# : /X  0 "is_done" !field ;

## Marks the current task as complete
# ( -- )
# : x    cur-task  X pop ;

## Marks the current task as not complete
# ( -- )
# : /x    cur-task  /X pop ;


#
## ======================================
## Misc
## ======================================
#
### Redefine 'm'ove task so the cur-task gets updated
#: m    m refresh-cur-task ;
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

# all incomplete   "id" "parent_id" forest .

# all incomplete   "id" "parent_id" forest .
# Become interactive
.i

# all "id" "parent_id" forest .

#2 T descendants in-decreasing-value .

# descendants in-decreasing-value as-forest . 
#2 T descendants in-decreasing-value as-forest .
#all in-decreasing-value as-forest .
#all in-decreasing-value .

#all incomplete .

#.q
