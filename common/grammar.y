/*
 *  grammar.y
 *  Daruma Basic
 *  2015/12/07 Toshi Nagata
 *  Copyright (c) 2015 by Toshi Nagata.  All rights reserved.
 */

%{
#include <stdio.h>
#include <math.h>
#include "daruma.h"
#include "gencode.h"
#include "transmessage.h"
	
#define YYDEBUG_VERBOSE 1
#define YYERROR_VERBOSE 1
#define YYDEBUG 1
	
%}

/*  Yacc tokens  */
/*  (yacc token - BS_IF + 128) = internal token)  */

/*  Reserved words  */
%token BS_IF BS_THEN BS_ELSE BS_ELSEIF BS_ENDIF
%token BS_DO BS_WHILE BS_UNTIL BS_LOOP
%token BS_FOR BS_TO BS_STEP BS_NEXT
%token BS_GOTO BS_FUNC BS_PROC
%token BS_RETURN BS_EXIT
%token BS_ENDPROC BS_ENDFUNC
%token BS_DIM
%token BS_PRINT BS_INPUT BS_LET BS_LOCAL BS_CALL
%token BS_BREAK BS_CONTINUE
%token BS_DATA BS_CDATA BS_READ BS_RESTORE
%token BS_END
%token BS_WAIT
%token BS_ERROR_TOKEN
%token BS_EOF

/*  Built-in commands  */
%token BS_BUILTIN

/*  Numeric and string literals  */
%token BS_INTEGER BS_FLOAT BS_STRING

/*  Generic Symbol and Declared Symbol  */
%token BS_SYMBOL
%token BS_VARNAME
%token BS_DIMNAME
%token BS_LABEL
%token BS_FUNCNAME
%token BS_PROCNAME

%left BS_OR
%left BS_AND
%left BS_XOR
%right BS_NOT
%nonassoc '=' BS_NEQ
%nonassoc '<' BS_LTEQ '>' BS_GTEQ
%left BS_LSHIFT BS_RSHIFT
%left '+' '-'
%left '*' '/' '%'
%right BS_UNARY_MINUS BS_UNARY_PLUS

%%

/*  Program  */

program: statement_loop_EOL end_of_program {
	if (bs_check_undefined_labels(0) < 0) YYERROR;
	if (bs_check_undefined_funcs() < 0) YYERROR;
	if (bs_close_data_statement() < 0) YYERROR;
	bs_code0(C_STOP);
	bs_code0(C_NUL);
	YYACCEPT;
}
;

end_of_program: BS_EOF
| BS_END EOL
;

EOL: '\n'
/*| BS_ERROR_TOKEN */
;

optional_eol:
| optional_eol '\n'
;

DELIM: ':'
| EOL
;

/*  Block of statements delimited by ':' or EOL  */
statement_loop: statement
| statement_loop DELIM statement
;

/*  Empty or statement_loop with delimiter (':' or EOL)  */
/*  Used in the body of for-next, do-loop, func-endfunc  */
statement_loop_delim:
| statement_loop DELIM
/*| statement_loop error DELIM */
;

/*  Empty or statement_loop with EOL  */
/*  Used in the body of program and multiline-if  */
statement_loop_EOL:
| statement_loop EOL
/*| statement_loop error EOL */
;

/*  Inline statements delimited by ':'  */
/*  Used in the body of inline-if  */
multi_statement: inline_statement
| multi_statement ':' inline_statement
;

/*  Statements  */

statement: /* empty */
| inline_statement
| for_statement
| inline_if_statement
| if_statement
| label_statement
| do_statement
| func_proc_definition
| data_statement
| dim_assign_statement
;

/*  FOR statement  */
/*  FOR var=expr TO expr STEP expr <FORTEST n> <BNZ @3> @s */
/*    ...(CONTINUE <BRA @c>... */
/*    ...(BREAK <BRA @e>)...  */
/*  NEXT @c <NEXT n> <BZ @s> @e  */

for_statement: BS_FOR {
	if (($$.u.iv = bs_start_loop(BS_LOOPTYPE_FOR)) < 0) YYERROR;
	bs_code1(C_PUSH_FREF, gParserInfo.lp->coff);
} for_control DELIM statement_loop_delim BS_NEXT {
	if (bs_finalize_loop($2.u.iv) < 0) YYERROR;
}
;

for_control: new_lvalue '=' {
	bs_code0(C_FSTORE_REF);  /*  Store the lvalue reference to the implicit variable 1  */
	bs_code1(C_PUSH_FREF, gParserInfo.lp->coff);
	bs_code0(C_FLOAD_REF);   /*  Reload the lvalue reference  */
} expr BS_TO {
	if (bs_generate_store($1.type, $4.type) < 0) YYERROR;
	bs_code1(C_PUSH_FREF, gParserInfo.lp->coff + sizeof(Off_t)); /* implicit variable 2 */
} expr {
	if (bs_generate_store($1.type | BS_TYPE_LOCAL, $7.type) < 0) YYERROR;
	if ($1.type % BS_TYPE_LOCAL == BS_TYPE_INTEGER) {
		/* implicit variable 3 */
		bs_code1(C_PUSH_FREF, gParserInfo.lp->coff + sizeof(Off_t) + sizeof(Int));
	} else {
		bs_code1(C_PUSH_FREF, gParserInfo.lp->coff + sizeof(Off_t) + sizeof(Float));
	}
} optional_step {
	if (bs_complete_for_header($1.type, $9.type) < 0) YYERROR;
	$$ = $1;
}
;

optional_step: { bs_code1(C_PUSH_INT, 1); $$.type = BS_TYPE_INTEGER; }
| BS_STEP expr { $$ = $2; }
;

/*  DO statements  */
/* DO @s body LOOP <BRA @s> @e */
/* DO @s WHILE expr <BZ @e> body LOOP <BRA @s> @e */
/* DO @s UNTIL expr <BNZ @e> body LOOP <BRA @s> @e */
/* DO @s body LOOP WHILE expr <BNZ @s> @e */
/* DO @s body LOOP UNTIL expr <BZ @s> @e  */
/* body: ... BREAK <BRA @e> ... CONTINUE <BRA @s> ...  */
/* (start addr = continue addr)  */

do_statement: do_header do_cond {
	if (bs_generate_do_cond($2.type) < 0) YYERROR;
} DELIM statement_loop_delim BS_LOOP do_cond {
	if ($7.type != BS_TYPE_NONE) {
		if ($2.type != BS_TYPE_NONE) {
			bs_error(MSG_(BS_M_ONLY_ONE_DO_COND));
			YYERROR;
		}
		if (bs_generate_do_cond($7.type | 0x20) < 0) YYERROR;
	} else {
		bs_code2(C_BRA, gParserInfo.lp->saddr);
	}
	if (bs_finalize_loop($1.u.iv) < 0) YYERROR;
}
;

do_header: BS_DO { if (($$.u.iv = bs_start_loop(BS_LOOPTYPE_DO)) < 0) YYERROR; }
;

do_cond: { $$.type = BS_TYPE_NONE; }
| BS_WHILE expr { $$.type = $2.type; }
| BS_UNTIL expr { $$.type = $2.type | 0x10; }
;

/*  IF statement  */

if_cond: BS_IF conditional_expr BS_THEN {
	$$.u.iv = gParserInfo.ifaddr;
	gParserInfo.ifaddr = bs_code2(C_BZ, kInvalidProgPos);
}
;

elseif_cond: BS_ELSEIF {
	gParserInfo.ifaddr = bs_code2(C_BRA, gParserInfo.ifaddr);
} conditional_expr BS_THEN {
	gParserInfo.ifaddr = bs_code2(C_BZ, gParserInfo.ifaddr);
}
;

else_cond: BS_ELSE { gParserInfo.ifaddr = bs_code2(C_BRA, gParserInfo.ifaddr); }
;

if_statement: if_cond EOL statement_loop_EOL optional_elseif_loop optional_else BS_ENDIF {
	bs_complete_if_statement();
	gParserInfo.ifaddr = $1.u.iv;
}
;

optional_elseif_loop:
| optional_elseif_loop elseif_cond EOL statement_loop_EOL
;

optional_else:
| else_cond EOL statement_loop_EOL
;

/*  Inline IF statement  */

inline_if_statement: if_cond inline_if_body optional_inline_elseif_loop 
optional_inline_else optional_inline_endif {
	bs_complete_if_statement();
	gParserInfo.ifaddr = $1.u.iv;
}
;

inline_if_body: multi_statement
| BS_LABEL { if (bs_generate_goto($1.u.iv)) YYERROR; }
;

optional_inline_elseif_loop: 
| optional_inline_elseif_loop elseif_cond inline_if_body
;

optional_inline_else:
| else_cond inline_if_body
;

optional_inline_endif:
| BS_ENDIF
;

/*  Label statement  */
/*  Label statement is not an inline statement, because it cannot be used
    inside an inline-if-statement  */

label_statement: BS_LABEL { if (bs_define_label($1.u.iv) < 0) YYERROR; }
;

/*  Data statement  */

data_statement: BS_DATA data_value_loop
| BS_CDATA cdata_value_loop
;

data_value_loop: data_value
| data_value_loop ',' data_value
;

data_value: const_expr { if (bs_generate_data(&($1)) < 0) YYERROR; }
;

cdata_value_loop: cdata_value
| cdata_value_loop ',' cdata_value
;

cdata_value: const_expr { if (bs_generate_cdata(&($1)) < 0) YYERROR; }

/*  FUNC/PROC definitions  */

func_proc_definition: func_proc '(' formal_argument_list ')' {
	if (bs_close_formal_argument_list() < 0) YYERROR;
} DELIM statement_loop_delim end_func_proc
;

formal_argument_list:
| formal_argument_list_nonempty
;

formal_argument_list_nonempty: formal_argument
| formal_argument_list_nonempty ',' formal_argument
;

formal_argument: BS_SYMBOL {
	if (($$.u.iv = bs_define_var($1.u.symcode, gParserInfo.fp - (FuncInfo *)gFuncInfoBasePtr)) < 0 ||
		($$.u.iv = bs_check_formal_argument($$.u.iv)) < 0)
		YYERROR;
}
;

func_proc: BS_FUNC new_funcname { if (($$.u.iv = bs_start_funcdef($2.u.iv, 1)) < 0) YYERROR; }
| BS_PROC new_procname { if (($$.u.iv = bs_start_funcdef($2.u.iv, 0)) < 0) YYERROR; }
;

end_func_proc: BS_ENDFUNC { if (bs_finalize_funcdef(1) < 0) YYERROR; }
| BS_ENDPROC { if (bs_finalize_funcdef(0) < 0) YYERROR; }
;

/*  Inline statements  */
/*  Inline statements are simple statements that can be used inside
	an inline-if-statement  */

inline_statement: assign_statement
| let_statement
| dim_statement
| print_statement
| input_statement
| call_statement
| read_statement
| local_statement
| builtin_statement
| wait_statement
| BS_EXIT { bs_code0(C_STOP); }
| BS_GOTO BS_LABEL { if (bs_generate_goto($2.u.iv) < 0) YYERROR; }
| BS_BREAK { if (bs_generate_break_statement() < 0) YYERROR; }
| BS_CONTINUE { if (bs_generate_continue_statement() < 0) YYERROR; }
| BS_RETURN { if (bs_generate_return(0) < 0) YYERROR; }
| BS_RETURN expr { if (bs_generate_return($2.type) < 0) YYERROR; }
| BS_RESTORE BS_LABEL { if (bs_generate_restore($2.u.iv) < 0) YYERROR; }
| BS_SYMBOL { bs_error(MSG_(BS_M_UNDEF_SYMBOL), gConstStringBasePtr + $1.u.symcode.str); YYERROR; }
;

/*  Assignment (Let) statement  */

assign_statement: new_lvalue '=' expr {
	if (($$.type = bs_generate_store($1.type, $3.type)) < 0) YYERROR;
};

let_statement: BS_LET assign_statement
;

/*  DIM statement  */
dim_statement: BS_DIM {
	if (bs_check_outside_control_statement(MSG_(BS_M_DIM_DECL)) < 0) YYERROR;
} dim_declaration_list
;

dim_declaration_list: dim_declaration_with_initializer
| dim_declaration_list ',' dim_declaration_with_initializer
;

dim_declaration_with_initializer: dim_declaration
| dim_declaration '=' { gParserInfo.dim_index = $1.u.iv; } dim_initializer
;

dim_declaration: BS_SYMBOL {
	if (($$.u.iv = bs_define_dim($1.u.symcode)) < 0) YYERROR;
} '(' dim_index_list ')' {
	/*  $4.u.iv is the return value of dim_index_list.
		$2.u.iv is the return value of bs_define_dim  */
	if (bs_generate_def_dim($4.u.iv, $2.u.iv) < 0) YYERROR;
	$$.u.iv = $2.u.iv;
}
;

/*  dim_index_list: push the arguments as integers; yacc value = the number of arguments  */
dim_index_list: dim_index { $$.u.iv = 1; }
| dim_index_list ',' dim_index { $$.u.iv = $1.u.iv + 1; }
;

/*  dim_index: calc expression and coerce into integer; yacc value = don't care  */
dim_index: expr { if (bs_generate_coerce(BS_TYPE_INTEGER, $1.type) < 0) YYERROR; }
;

/*  DIM initializer  */
dim_initializer: '{' {
	bs_code1(C_PREP_DIM, ((DimInfo *)gDimInfoBasePtr)[gParserInfo.dim_index].offset);
} optional_eol dim_initializer_loop optional_eol '}' {
	bs_code1(C_INC_SP, sizeof(Off_t) * 2);
}
;

dim_initializer_loop: dim_initializer_one
| dim_initializer_loop ',' optional_eol dim_initializer_one
;

dim_initializer_one: expr {
	switch ($$.type) {
		case BS_TYPE_INTEGER: bs_code0(C_STORELOOP_INT); break;
		case BS_TYPE_FLOAT:   bs_code0(C_STORELOOP_FLT); break;
		case BS_TYPE_STRING:  bs_code0(C_STORELOOP_STR); break;
	}
}
;

dim_assign_statement: BS_DIMNAME '=' { gParserInfo.dim_index = $1.u.iv; } dim_initializer
;

/*  PRINT statement  */

print_keyword: BS_PRINT
| '?'
;

print_statement: print_keyword print_arguments { bs_code0(C_NEWLINE); }
| print_keyword print_arguments ';'
| print_keyword print_arguments ',' { bs_code1(C_PUTC, '\t'); }
;

print_arguments:
| print_one_argument
| print_arguments ';' print_one_argument
| print_arguments ',' { bs_code1(C_PUTC, '\t'); } print_one_argument
;

print_one_argument: expr {
	switch ($1.type) {
		case BS_TYPE_INTEGER: bs_code0(C_PRINT_INT); break;
		case BS_TYPE_FLOAT:   bs_code0(C_PRINT_FLT); break;
		case BS_TYPE_STRING:  bs_code0(C_PRINT_STR); break;
	}
}
;

/*  INPUT statement  */
input_statement: BS_INPUT {
	Off_t sval = bs_new_literal_string("? ", -1);
	if (sval == kInvalidOff) {
		bs_error(MSG_(BS_M_OUT_OF_MEMORY_INPUT));
		YYERROR;
	}
	bs_code1(C_PUSH_STRL, (sval & ~kMSBOff));
	bs_code0(C_PRINT_STR);
} new_lvalue { if (bs_generate_input($3.type) < 0) YYERROR; }
| BS_INPUT BS_STRING {
	if (($$.type = bs_generate_const_expr(&($2))) < 0) YYERROR;
	bs_code0(C_PRINT_STR);
} ',' new_lvalue { if (bs_generate_input($5.type) < 0) YYERROR; }
;

/*  CALL statement  */

call_statement: BS_CALL new_procname {
	if (($$.u.iv = bs_start_funcall($2.u.iv, 0)) < 0) YYERROR;
} '(' argument_list ')' {
	if (bs_finalize_funcall($3.u.iv) < 0) YYERROR;
}
;

new_procname: BS_PROCNAME
| BS_SYMBOL { /* Implicit definition of FUNC */
	if (($$.u.iv = bs_define_funcname($1.u.symcode, 0)) < 0) YYERROR;
}
;

new_funcname: BS_FUNCNAME
| BS_SYMBOL { /* Implicit definition of FUNC */
	if (($$.u.iv = bs_define_funcname($1.u.symcode, 1)) < 0) YYERROR;
}
;

argument_list:
| argument_list_nonempty
;

argument_list_nonempty: argument
| argument_list_nonempty ',' argument
;

argument: expr { if (bs_check_argument($1.type, -1) < 0) YYERROR; }
| BS_DIMNAME { if (bs_check_argument(0, $1.u.iv) < 0) YYERROR; }
;

/*  READ statement  */
read_statement: BS_READ read_var_list
;

read_var_list: read_var
| read_var_list ',' read_var
;

read_var: new_lvalue { if (bs_generate_read_and_store($$.type) < 0) YYERROR; }
;

/*  LOCAL statement  */

local_statement: BS_LOCAL { bs_start_local_statement(); } local_var_list {
	if (bs_finalize_local_statement() < 0) YYERROR;
}
;

local_var_list: local_var
| local_var_list ',' local_var
;

local_var: BS_SYMBOL {
	if (($$.u.iv = bs_define_var($1.u.symcode, gParserInfo.fp - (FuncInfo *)gFuncInfoBasePtr)) < 0 ||
		($$.u.iv = bs_check_local_var($$.u.iv)) < 0)
		YYERROR;
}
;

/*  Built-in statement  */

builtin_statement: BS_BUILTIN {
	if (($$.u.iv = bs_start_funcall(-($1.u.iv + 1), 0)) < 0) YYERROR;
} argument_list {
	if (bs_finalize_funcall($2.u.iv) < 0) YYERROR;
}
;

/*  Wait statement  */
wait_statement: BS_WAIT expr {
	if (bs_generate_coerce(BS_TYPE_INTEGER, $2.type) < 0) YYERROR;
	bs_code0(C_PREP_WAIT);
	bs_code0(C_WAIT);
}
;

/*  Expressions  */

conditional_expr: expr { if (bs_make_conditional_expr($1.type) < 0) YYERROR; }
;

expr: lvalue { if (($$.type = bs_generate_load($1.type)) < 0) YYERROR; }
| non_lvalue_expr
;

non_lvalue_expr: BS_INTEGER { if (($$.type = bs_generate_const_expr(&($1))) < 0) YYERROR; }
| BS_FLOAT { if (($$.type = bs_generate_const_expr(&($1))) < 0) YYERROR; }
| BS_STRING { if (($$.type = bs_generate_const_expr(&($1))) < 0) YYERROR; }
| expr '+' expr { if (($$.type = bs_generate_op2($1.type, $3.type, '+')) < 0) YYERROR; }
| expr '-' expr { if (($$.type = bs_generate_op2($1.type, $3.type, '-')) < 0) YYERROR; }
| expr '*' expr { if (($$.type = bs_generate_op2($1.type, $3.type, '*')) < 0) YYERROR; }
| expr '/' expr { if (($$.type = bs_generate_op2($1.type, $3.type, '/')) < 0) YYERROR; }
| expr '<' expr { if (($$.type = bs_generate_op2($1.type, $3.type, '<')) < 0) YYERROR; }
| expr '>' expr { if (($$.type = bs_generate_op2($1.type, $3.type, '>')) < 0) YYERROR; }
| expr '=' expr { if (($$.type = bs_generate_op2($1.type, $3.type, '=')) < 0) YYERROR; }
| expr BS_NEQ expr { if (($$.type = bs_generate_op2($1.type, $3.type, BS_NEQ)) < 0) YYERROR; }
| expr BS_GTEQ expr { if (($$.type = bs_generate_op2($1.type, $3.type, BS_GTEQ)) < 0) YYERROR; }
| expr BS_LTEQ expr { if (($$.type = bs_generate_op2($1.type, $3.type, BS_LTEQ)) < 0) YYERROR; }
| expr '%' expr { if (($$.type = bs_generate_int_op2($1.type, $3.type, '%')) < 0) YYERROR; }
| expr BS_OR expr { if (($$.type = bs_generate_int_op2($1.type, $3.type, BS_OR)) < 0) YYERROR; }
| expr BS_AND expr { if (($$.type = bs_generate_int_op2($1.type, $3.type, BS_AND)) < 0) YYERROR; }
| expr BS_XOR expr { if (($$.type = bs_generate_int_op2($1.type, $3.type, BS_XOR)) < 0) YYERROR; }
| expr BS_LSHIFT expr { if (($$.type = bs_generate_int_op2($1.type, $3.type, BS_LSHIFT)) < 0) YYERROR; }
| expr BS_RSHIFT expr { if (($$.type = bs_generate_int_op2($1.type, $3.type, BS_RSHIFT)) < 0) YYERROR; }
| '(' expr ')'  { $$ = $2; } /*  '(lvalue)' is not an lvalue in this grammar  */
| '-' expr %prec BS_UNARY_MINUS { if (($$.type = bs_generate_op1($2.type, '-')) < 0) YYERROR; }
| '+' expr %prec BS_UNARY_PLUS  { if (($$.type = bs_generate_op1($2.type, '+')) < 0) YYERROR; }
| BS_NOT expr { if (($$.type = bs_generate_op1($2.type, BS_NOT)) < 0) YYERROR; }
| BS_SYMBOL { bs_error(MSG_(BS_M_UNDEF_SYMBOL), gConstStringBasePtr + $1.u.symcode.str); YYERROR; }
| funcall
| builtin_funcall
;

const_expr: BS_INTEGER { $$ = $1; }
| BS_FLOAT { $$ = $1; }
| BS_STRING { $$ = $1; }
| const_expr '+' const_expr { if (bs_generate_const_op2(&($$), &($1), &($3), '+') < 0) YYERROR; }
| const_expr '-' const_expr { if (bs_generate_const_op2(&($$), &($1), &($3), '-') < 0) YYERROR; }
| const_expr '*' const_expr { if (bs_generate_const_op2(&($$), &($1), &($3), '*') < 0) YYERROR; }
| const_expr '/' const_expr { if (bs_generate_const_op2(&($$), &($1), &($3), '/') < 0) YYERROR; }
| const_expr '<' const_expr { if (bs_generate_const_op2(&($$), &($1), &($3), '<') < 0) YYERROR; }
| const_expr '>' const_expr { if (bs_generate_const_op2(&($$), &($1), &($3), '>') < 0) YYERROR; }
| const_expr '=' const_expr { if (bs_generate_const_op2(&($$), &($1), &($3), '=') < 0) YYERROR; }
| const_expr BS_NEQ const_expr { if (bs_generate_const_op2(&($$), &($1), &($3), BS_NEQ) < 0) YYERROR; }
| const_expr BS_GTEQ const_expr { if (bs_generate_const_op2(&($$), &($1), &($3), BS_GTEQ) < 0) YYERROR; }
| const_expr BS_LTEQ const_expr { if (bs_generate_const_op2(&($$), &($1), &($3), BS_LTEQ) < 0) YYERROR; }
| const_expr '%' const_expr { if (bs_generate_const_int_op2(&($$), &($1), &($3), '%') < 0) YYERROR; }
| const_expr BS_OR const_expr { if (bs_generate_const_int_op2(&($$), &($1), &($3), BS_OR) < 0) YYERROR; }
| const_expr BS_AND const_expr { if (bs_generate_const_int_op2(&($$), &($1), &($3), BS_AND) < 0) YYERROR; }
| const_expr BS_XOR const_expr { if (bs_generate_const_int_op2(&($$), &($1), &($3), BS_XOR) < 0) YYERROR; }
| const_expr BS_LSHIFT const_expr { if (bs_generate_const_int_op2(&($$), &($1), &($3), BS_LSHIFT) < 0) YYERROR; }
| const_expr BS_RSHIFT const_expr { if (bs_generate_const_int_op2(&($$), &($1), &($3), BS_RSHIFT) < 0) YYERROR; }
| '(' const_expr ')'  { $$ = $2; } 
| '-' const_expr %prec BS_UNARY_MINUS { if (bs_generate_const_op1(&($$), &($2), '-') < 0) YYERROR; }
| '+' const_expr %prec BS_UNARY_PLUS { if (bs_generate_const_op1(&($$), &($2), '+') < 0) YYERROR; }
;

funcall: new_funcname {
	if (($$.u.iv = bs_start_funcall($1.u.iv, 1)) < 0) YYERROR;
} '(' argument_list ')' {
	if (($$.type = bs_finalize_funcall($2.u.iv)) < 0) YYERROR;
}
;

builtin_funcall: BS_BUILTIN {
	if (($$.u.iv = bs_start_funcall(-($1.u.iv + 1), 1)) < 0) YYERROR;
} '(' argument_list ')' {
	if (($$.type = bs_finalize_funcall($2.u.iv)) < 0) YYERROR;
}
;

new_lvalue: lvalue
| BS_SYMBOL {  /* Implicit definition of a variable */
	if (($$.u.iv = bs_define_var($1.u.symcode, 0)) < 0 ||
		($$.type = bs_generate_var_ref($$.u.iv)) < 0)
		YYERROR;
}
;

lvalue: BS_VARNAME { if (($$.type = bs_generate_var_ref($1.u.iv)) < 0) YYERROR; }
| BS_DIMNAME '(' dim_index_list ')' {
	if (($$.type = bs_generate_get_dim($3.u.iv, $1.u.iv)) < 0) YYERROR;
}
;

%%
