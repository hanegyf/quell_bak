program         = { function } ;
    # A program consists of one or more function definitions.


function        = "func" identifier [ generic_params ] "(" [ params ] ")" "->" type block ; 
    # Functions may optionally be generic (e.g., func qft<N>(...)).


generic_params  = "<" identifier { "," identifier } ">" ;


params          = param { "," param } ;
param           = identifier ":" type ;

block           = "{" { statement } "}" ;


statement       = let_stmt
                | return_stmt
                | if_stmt
                | for_stmt
                | expr_stmt ;
    # Statements allowed inside blocks.


let_stmt        = "let" pattern ":" type [ "=" expression ] ; 
    # Supports destructuring: 
    # let (q1, c1): (qubit, bit) = measure(q0)


pattern         = identifier
                | tuple_pattern ;


tuple_pattern   = "(" identifier { "," identifier } ")" ;


return_stmt     = "return" [ expression ] ;


if_stmt         = "if" "(" expression ")" block [ "else" block ] ;


expr_stmt       = expression ;


expression      = if_expr
                | lambda
                | binary ;


if_expr         = "if" "(" expression ")" "{"
                    expression
                  "}" "else" "{"
                    expression
                  "}" ;
    # Expression-based conditional.


lambda          = "func" "(" [ params ] ")" "->" type block ;
    # Anonymous functions allowed.


binary          = application
                  { ( "+"                      # gate composition
                    | "==" | "!="              # equal/not equal 
                    | "<" | ">" | "<=" | ">="  # comparison
                    | "+" | "-" | "*" | "/"    # arithmetic
                    )
                    application } ;


application     = primary { primary } ;
    # Lambda-calculus-style application:
    # (H + CX(0,1))(qreg)
    # f(x)(y)


primary         = literal
                | identifier
                | call
                | index
                | tuple
                | paren ;


call            = identifier "(" [ args ] ")" ;
    # Standard call syntax.


args            = expression { "," expression } ;


index           = identifier "[" expression "]" ;
    # Register indexing: r[0]


tuple           = "(" expression "," expression
                   { "," expression } ")" ;


paren           = "(" expression ")" ;


type            = "void"
                | "qubit"
                | "bit"
                | "int"
                | "float"
                | "bool"
                | "qreg" "<" number_or_ident ">"
                | "breg" "<" number_or_ident ">"
                | "(" type "," type { "," type } ")" ;


number_or_ident = number | identifier ;


literal         = number
                | string
                | "true"
                | "false" ;


number          = digit { digit }
                  [ "." digit { digit } ] ;


string          = '"' { character } '"' ;


# print is parsed as a normal call:
# print("1")

# measure(r, c) or measure(q) handled as standard function call


comment         = "#" { character } ;
    # Single-line comments only.

# The lexer should ignore whitespace and comments


identifier      = letter { letter | digit | "_" } ;

letter          = "A".."Z" | "a".."z" ;

digit           = "0".."9" ;

character       = ? any character except '"' ? ;
