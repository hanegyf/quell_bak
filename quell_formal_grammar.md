program         = { function } EOF ;
    # A program consists of zero or more function definitions


function        = "func" identifier [ generic_params ]
                  "(" [ params ] ")" "->" type
                  block ;


generic_params  = "<" identifier { "," identifier } [ "," ] ">" ;


params          = param { "," param } [ "," ] ;
param           = identifier ":" type ;


block           = "{" { statement } [ expression ] "}" ;
    # Expression-block: optionally ends with a value expression (no trailing ';' on that final expression)


statement       = let_stmt
                | return_stmt
                | if_stmt
                | gate_stmt
                | expr_stmt
                | ";" ;
    # Allow empty statement as a standalone ';'


let_stmt        = "let" pattern ":" type [ "=" expression ] ";" ;


pattern         = identifier
                | tuple_pattern ;


tuple_pattern   = "(" [ identifier { "," identifier } [ "," ] ] ")" ;
    # Allows:
    #   ()          -- unit pattern (optional; you can forbid later if you want)
    #   (x)         -- 1-tuple pattern (NOTE: syntactically same as parenthesized identifier)
    #   (x,)        -- 1-tuple pattern (recommended canonical spelling)
    #   (x,y,)      -- trailing comma


return_stmt     = "return" [ expression ] ";" ;


if_stmt         = "if" "(" expression ")" block [ "else" block ] ;
    # Statement form; block can yield a value but that value is ignored unless returned/used explicitly


gate_stmt       = gate_name gate_operands ";" ;
    # Dedicated quantum gate statements.


gate_name       = "H" | "X" | "Y" | "Z" | "S" | "T"
                | "CX" | "CZ"
                | "CNOT"
                | "SWAP"
                | "MEASURE"
                | "RESET" ;
    # Can collapse synonyms later (e.g. CX == CNOT), or keep both


gate_operands   = gate_operand { gate_operand } ;
    # Space-separated operands
    # Example: CX q0 q1;
    # Example: MEASURE q0 c0;


gate_operand    = identifier
                | index
                | number ;
    # Allow either symbolic names (q0), indexed registers (qreg[0]), or raw numeric indices (0)


expr_stmt       = expression ";" ;


expression      = if_expr
                | lambda
                | composition ;


if_expr         = "if" "(" expression ")" expr_block "else" expr_block ;
    # Expression-based conditional


expr_block      = "{" { statement } [ expression ] "}" ;
    # Same shape as block, but used in expression position.


lambda          = "func" "(" [ params ] ")" "->" type
                  expr_block ;
    # Anonymous function, uses expr_block to allow returning a value expression


composition     = equality { "." equality } ;
    # Function composition operator, left-associative here
    # Semantics can be "f . g" = (x -> f(g(x))) like Haskell


equality        = comparison { ( "==" | "!=" ) comparison } ;


comparison      = additive { ( "<" | ">" | "<=" | ">=" ) additive } ;


additive        = multiplicative { ( "+" | "-" ) multiplicative } ;


multiplicative  = application { ( "*" | "/" ) application } ;


application     = postfix { postfix } ;
    # Juxtaposition application:
    # f(x) is a call expression; but also allow:
    #   f x
    #   f x y


postfix         = primary { call_suffix | index_suffix } ;


call_suffix     = "(" [ args ] ")" ;
    # Crucial change: allows calling any expression, not just identifiers
    # (f)(x), (func(...) -> ... {...})(y), etc


index_suffix    = "[" expression "]" ;


args            = expression { "," expression } [ "," ] ;


primary         = literal
                | identifier
                | tuple
                | "(" expression ")"
                | expr_block ;
    # expr_block as a primary enables block-expressions in general, e.g.:
    # let x: int = { let y:int = 2; y + 1 };


tuple           = "(" [ expression { "," expression } [ "," ] ] ")" ;
    # Allows:
    #   ()        unit value
    #   (x)       parenthesized expression (NOT a tuple)
    #   (x,)      1-tuple
    #   (x,y,)    trailing comma


type            = "void"
                | "qubit"
                | "bit"
                | "int"
                | "float"
                | "bool"
                | "qreg" "<" number_or_ident ">"
                | "breg" "<" number_or_ident ">"
                | "(" type { "," type } [ "," ] ")" ;
    # Type tuples allow 1-tuples too, (int,)


number_or_ident = number | identifier ;


literal         = number
                | string
                | "true"
                | "false" ;


number          = digit { digit }
                  [ "." digit { digit } ] ;


string          = '"' { character } '"' ;


comment         = "#" { not_newline_character } ;
    # Single-line comment; lexer discards it


identifier      = ( letter | "_" ) { letter | digit | "_" } ;


letter          = "A".."Z" | "a".."z" ;
digit           = "0".."9" ;


character       = ? any character except '"' ? ;
not_newline_character = ? any character except newline ? ;
