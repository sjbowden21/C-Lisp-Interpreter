# C-Lisp-Interpreter
Implements a command line intepreter for a Lisp dialect. Written in C, 2289 LOC. Executes a read, evaluate, print (REPL) loop when run by the user. Implementation covers a significant portion of its Lisp dialect but is partial (does not include closures for example). The following are implemented:

- data structure to house the data types (tagged enum used)
- number, symbol, boolean, string, list, and error data types
- startup environment of bindings (maps symbols to functions)
- +, -, *, /, =, quit, <, <=, >, >=, !=, and 
not functions
- def, _,  if, cons, head, tail, quote, ord, chr, input, output and type special forms
- freeing of dynamically declared variables, including freeing of environments, sublists when no longer assessible or used, underscore binding when replaced (_ is set to the last sucessful return), parsed list from user input stream, evaluated list/value, return value, etc.
- recursive evaluation to arbitrary depth (i.e. to arbitrary number of nested parenthesis)

# Example Usage

input> (+ 3 4)  
7  
input> (/ 8 4)  
2  
input> (+ 1 (- 8 4) (+ ( / (* 3 4 5) 20) 10))   
18  
input> (= #t #f)  
#f  
input> (= #t #t #t)  
#t  
input> (= #f #f #f #t)  
#f  
input> (= #t 8)  
#f  
input> (< 2 3)  
#t  
input> (> 2 3)  
#f  
input> (>= 10 9 8 7 6)  
#t  
input> (<= 9 9 9 10)  
#t  
input> (<= 9 9 9 10 9)  
#f  
input> (+ 3 #t)  
$error{(type-error + 2 number #t)}  
input> (  
$error{(incomplete-parse ()}  
input> (+ 10 10))  
$error{(invalid-token (+ 10 10)))}  
input> a  
$error{(unbound-symbol a)}  
input> (def a (+ 1 3))  
4  
input> a  
4  
input> _  
4  
input> 'a  
a  
input> ''a  
'a  
input> a  
4  
input> (+ 3 a)  
7  
input> (/ 8 0)  
$error\{division-by-zero}  
input> (if (= 3 3) (def b 2) (def b 4))  
2  
input> b  
2  
input> (def c '(1 2 3))  
(1 2 3)  
input> (cons c 4)  
$error{(type-error cons 1 list 4)}  
input> (def d (cons 4 c))  
(4 1 2 3)  
input> d  
(4 1 2 3)  
input> (head c)  
1  
input> (head d)  
4  
input> (tail d)  
(1 2 3)  



