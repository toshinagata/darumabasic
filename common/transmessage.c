/*
 *  transmessage.c
 *  Daruma Basic
 *
 *  Created by Toshi Nagata on 16/02/15.
 *  Copyright 2016 Toshi Nagata. All rights reserved.
 *
 */

#include <stdlib.h>
#include <string.h>

#include "transmessage.h"

const char *err_msg[] = {
	/* BS_M_ONLY_ONE_DO_COND */
	"only one WHILE/UNTIL condition may appear with DO or LOOP keyword",
	
	/* BS_M_UNDEF_SYMBOL */
	"undefined symbol %s",
	
	/* BS_M_DIM_DECL */
	"DIM declaration",
	
	/* BS_M_BAD_CONST_EXPR */
	"Internal error: bad constant expression type %d",
	
	/* BS_M_UNKNOWN_UNARY */
	"unknown unary operation %d",
	
	/* BS_M_NUMBER_EXPECTED */
	"type mismatch: a number is expected",
	
	/* BS_M_TYPE_MISMATCH */
	"type mismatch: cannot operate two different types",
	
	/* BS_M_ILLEGAL_OP_STR */
	"Illegal operation %c on string",
	
	/* BS_M_UNKNOWN_BINARY */
	"internal error: unknown binary operation %d",
	
	/* BS_M_MUST_BE_INTEGER */
	"type mismatch: the operand must be integers",
	
	/* BS_M_UNKNOWN_INT_BINARY */
	"internal error: unknown integer binary operation %d",
	
	/* BS_M_INTERNAL_ERROR_1 */
	"Internal parser error: unknown variable type %d",
	
	/* BS_M_CANNOT_CONVERT_FLT */
	"Cannot convert value to float",
	
	/* BS_M_CANNOT_CONVERT_INT */
	"Cannot convert value to integer",
	
	/* BS_M_CANNOT_CONVERT_STR */
	"Cannot convert value to string",
	
	/* BS_M_UNKNOWN_TYPE_STORE */
	"Unknown type in STORE",
	
	/* BS_M_UNKNOWN_TYPE_LOAD */
	"Unknown type in LOAD",
	
	/* BS_M_NOT_INSIDE_IF */
	"%s cannot be inside IF statement",
	
	/* BS_M_NOT_INSIDE_LOOP */
	"%s cannot be inside FOR or DO loop",
	
	/* BS_M_ILLEGAL_DIM */
	"Illegal dimension type %d",
	
	/* BS_M_DIM_NO_MATCH */
	"Dimension does not match: %d %s are expected",
	
	/* BS_M_INDEX */
	"index",
	
	/* BS_M_INDICES */
	"indices",
	
	/* BS_M_TOO_MANY_LOOPS */
	"Too many loop structures",
	
	/* BS_M_TOO_MANY_LOCALS */
	"Too many local variables (including implicit variable in FOR loop)",
	
	/* BS_M_BREAK_OUTSIDE_LOOP */
	"BREAK is used outside a loop",
	
	/* BS_M_CONTINUE_OUTSIDE_LOOP */
	"CONTINUE is used outside a loop",
	
	/* BS_M_COND_NUMERIC */
	"Conditional expression must be a numeric type",
	
	/* BS_M_DUPLICATE_LABEL */
	"duplicate label",
	
	/* BS_M_JUMP_INTO_LOOP_OR_FUNC */
	"GOTO jump into FOR loop or FUNC/SUB definition",
	
	/* BS_M_JUMP_INTO_OUTOF_FUNC */
	"GOTO jump into or out of a FUNC/SUB definition is not allowed",
	
	/* BS_M_JUMP_INTO_FORLOOP */
	"GOTO jump into FOR loop is not allowed",
	
	/* BS_M_LINES_FMT */
	"lines %s",
	
	/* BS_M_LINE_FMT */
	"line %s",
	
	/* BS_M_ETC */
	"etc.",
	
	/* BS_M_LABEL_NOT_DEFINED */
	"The label %s is used in %s but not defined in this scope",
	
	/* BS_M_PROCFUNC_NOT_DEFINED */
	"The %s %s is used in %s but not defined",
	
	/* BS_M_PROC*/
	"proc",
	
	/* BS_M_FUNC */
	"func",
	
	/* BS_M_PROC_NOT_FUNC */
	"%s cannot be used as a function; it is a procedure",
	
	/* BS_M_FUNC_NOT_PROC */
	"%s cannot be used as a procedure; it is a function",
	
	/* BS_M_OUT_OF_MEMORY_ARG */
	"Out of memory during making argument list",
	
	/* BS_M_ARG_CANNOT_COERCE */
	"Argument %d: cannot coerce %s to %s",
	
	/* BS_M_BAD_DIM_ARG */
	"Argument %d: %s DIM argument is given but not expected",
	
	/* BS_M_TOO_MANY_ARGS */
	"Too many arguments",
	
	/* BS_M_MISSING_ARG */
	"Missing argument: argument %d is not given",
	
	/* BS_M_PROCFUNC_DEF_NESTED */
	"FUNC/PROC definition cannot be nested",
	
	/* BS_M_PROCFUNC_DEF */
	"FUNC/PROC definition",
	
	/* BS_M_FORMAL_ARGS_MISMATCH_NUM */
	"This %s is implicitly defined to have %d argument(s), but here too many formal arguments are given",
	
	/* BS_M_FORMAL_ARG_MISMATCH_TYPE */
	"This %s is implicitly defined to have argument %d as type %s, but here type %s",
	
	/* BS_M_TOO_MANY_FORMAL_ARGS */
	"Too many formal arguments",
	
	/* BS_M_FUNCDEF_BAD_END */
	"%s definition must be closed by %s",
	
	/* BS_M_RETURN_OUTSIDE_FUNC */
	"RETURN statement should be inside FUNC/PROC definition",
	
	/* BS_M_RETURN_NO_VALUE */
	"RETURN inside PROC definition should not have value",
	
	/* BS_M_RETURN_MISSING_VALUE */
	"RETURN inside FUNC definition should have value",
	
	/* BS_M_LOCAL_OUTSIDE_FUNC */
	"LOCAL statement should be inside FUNC/PROC definition",
	
	/* BS_M_TOO_MANY_LOCAL_VARS */
	"Too many local variables",
	
	/* BS_M_CDATA_ONLY_INTEGER */
	"CDATA can contain only integer data",
	
	/* BS_M_UNKNOWN_TYPE_READ */
	"Unknown type in READ",
	
	/* BS_M_COMPILE_ERROR */
	"Compilation aborted with error.\n",
	
	/* BS_M_START_PROGRAM */
	"Type 'edit' to start programming!\n",
	
	/* BS_M_RUNNING */
	"Running %s...\n",
	
	/* BS_M_PROG_DELETED_YN */
	"The program will be deleted. Do you really want to do this? (y/N) ",
	
	/* BS_M_EXIT_IN_DIRECT */
	"EXIT is not usable in direct mode. (Did you mean QUIT?)\n",
	
	/* BS_M_PROG_SAVE_YN */
	"The program is not saved yet. Do you really want to quit? (y/N) ",
	
	/* BS_M_FINISHED */
	"[[ Daruma Basic Finished ]]\n",
	
	/* BS_M_ESC_END_EDIT */
	"<esc>=end edit",

	/* BS_M_OUT_OF_MEMORY */
	"Out of memory",

	/* BS_M_CANNOT_OPEN */
	"Cannot open file %s\n",
	
	/* BS_M_SOURCE_TOO_LARGE */
	"Source file too large\n",
	
	/* BS_M_NO_PROGRAM_TO_SAVE */
	"There is no program to save.\n",
	
	/* BS_M_FILE_EXISTS */
	"File %s already exists. Overright? (y/N) ",

	/* BS_M_SAVE_CANCELED */
	"Save canceled.\n",
	
	/* BS_M_CANNOT_WRITE */
	"Cannot write to file %s\n",
	
	/* BS_M_ONLY_DIRECT */
	"%s is only usable in direct mode.",
	
	/* BS_M_INVALID_FNAME */
	"File name is invalid",
};

const char *translated_err_msg[] = {
	/* BS_M_ONLY_ONE_DO_COND */
	"DO...LOOPのWHILE/UNTIL条件は一つまでです",
	
	/* BS_M_UNDEF_SYMBOL */
	"%sは未定義です",
	
	/* BS_M_DIM_DECL */
	"配列宣言",
	
	/* BS_M_BAD_CONST_EXPR */
	"内部エラー: 定数式の型が間違っています(%d)",
	
	/* BS_M_UNKNOWN_UNARY */
	"演算子%dは使えません",
	
	/* BS_M_NUMBER_EXPECTED */
	"型の誤り: 数値が必要です",
	
	/* BS_M_TYPE_MISMATCH */
	"型の誤り: 違う型の間で計算ができません",
	
	/* BS_M_ILLEGAL_OP_STR */
	"文字列にこの演算%cはできません",
	
	/* BS_M_UNKNOWN_BINARY */
	"内部エラー: 演算子%dは使えません",
	
	/* BS_M_MUST_BE_INTEGER */
	"型の誤り: 整数が必要です",
	
	/* BS_M_UNKNOWN_INT_BINARY */
	"内部エラー: 整数の演算子%dは使えません",
	
	/* BS_M_INTERNAL_ERROR_1 */
	"内部エラー: 変数タイプ%dは使えません",
	
	/* BS_M_CANNOT_CONVERT_FLT */
	"実数に変換できません",
	
	/* BS_M_CANNOT_CONVERT_INT */
	"整数に変換できません",
	
	/* BS_M_CANNOT_CONVERT_STR */
	"文字列に変換できません",
	
	/* BS_M_UNKNOWN_TYPE_STORE */
	"内部エラー: STORE命令で使えない型です",
	
	/* BS_M_UNKNOWN_TYPE_LOAD */
	"内部エラー: LOAD命令で使えない型です",
	
	/* BS_M_NOT_INSIDE_IF */
	"%sはIF文の中では使えません",
	
	/* BS_M_NOT_INSIDE_LOOP */
	"%sはFOR, DOループの中では使えません",
	
	/* BS_M_ILLEGAL_DIM */
	"配列にできない型です(%d)",
	
	/* BS_M_DIM_NO_MATCH */
	"配列の次元が合いません: %d %sが必要です",
	
	/* BS_M_INDEX */
	"個の添字",
	
	/* BS_M_INDICES */
	"個の添字",
	
	/* BS_M_TOO_MANY_LOOPS */
	"ループの数が多すぎます",
	
	/* BS_M_TOO_MANY_LOCALS */
	"ローカル変数が多すぎてFOR文が処理できません",
	
	/* BS_M_BREAK_OUTSIDE_LOOP */
	"BREAKはループの中でしか使えません",
	
	/* BS_M_CONTINUE_OUTSIDE_LOOP */
	"CONTINUEはループの中でしか使えません",
	
	/* BS_M_COND_NUMERIC */
	"条件式は数値でないといけません",
	
	/* BS_M_DUPLICATE_LABEL */
	"同じラベルが２つあります",
	
	/* BS_M_JUMP_INTO_LOOP_OR_FUNC */
	"GOTOでFORループやFUNC/PROCの中には飛び込めません",
	
	/* BS_M_JUMP_INTO_OUTOF_FUNC */
	"GOTOで別のFUNC/PROCの間を飛ぶことはできません",
	
	/* BS_M_JUMP_INTO_FORLOOP */
	"GOTOでFORループの中には飛び込めません",
	
	/* BS_M_LINES_FMT */
	"%s行",
	
	/* BS_M_LINE_FMT */
	"%s行",
	
	/* BS_M_ETC */
	"などの",
	
	/* BS_M_LABEL_NOT_DEFINED */
	"ラベル %sが%sで使われていますが,定義がありません",
	
	/* BS_M_PROCFUNC_NOT_DEFINED */
	"%s %sが%sで使われていますが,定義がありません",
	
	/* BS_M_PROC*/
	"手続き",
	
	/* BS_M_FUNC */
	"関数",
	
	/* BS_M_PROC_NOT_FUNC */
	"%sは手続きなので,関数としては使えません",
	
	/* BS_M_FUNC_NOT_PROC */
	"%sは関数なので,手続きとしては使えません",
	
	/* BS_M_OUT_OF_MEMORY_ARG */
	"引数リストの処理中にメモリが足りなくなりました",
	
	/* BS_M_ARG_CANNOT_COERCE */
	"引数 %d: %sを%sに変換できません",
	
	/* BS_M_BAD_DIM_ARG */
	"引数 %d: 配列%sはここでは引数にできません",
	
	/* BS_M_TOO_MANY_ARGS */
	"引数が多すぎます",
	
	/* BS_M_MISSING_ARG */
	"引数が足りません(%d番目)",
	
	/* BS_M_PROCFUNC_DEF_NESTED */
	"FUNC/PROC定義は入れ子にできません",
	
	/* BS_M_PROCFUNC_DEF */
	"FUNC/PROC定義",
	
	/* BS_M_FORMAL_ARGS_MISMATCH_NUM */
	"この%sは %d個の引数を持つはずですが、仮引数が多すぎます",
	
	/* BS_M_FORMAL_ARG_MISMATCH_TYPE */
	"この%sの %d番目の引数は%s型のはずですが、仮引数が%s型です",
	
	/* BS_M_TOO_MANY_FORMAL_ARGS */
	"仮引数が多すぎます",
	
	/* BS_M_FUNCDEF_BAD_END */
	"%s定義は%sで終えなければなりません",
	
	/* BS_M_RETURN_OUTSIDE_FUNC */
	"RETURN文はFUNC/PROC定義の中でしか使えません",
	
	/* BS_M_RETURN_NO_VALUE */
	"PROC定義の中のRETURNは値を持てません",
	
	/* BS_M_RETURN_MISSING_VALUE */
	"FUNC定義の中のRETURNは値が必要です",
	
	/* BS_M_LOCAL_OUTSIDE_FUNC */
	"LOCAL文はFUNC/PROC定義の中でしか使えません",
	
	/* BS_M_TOO_MANY_LOCAL_VARS */
	"ローカル変数が多すぎます",
	
	/* BS_M_CDATA_ONLY_INTEGER */
	"CDATAは整数型のデータしか持てません",
	
	/* BS_M_UNKNOWN_TYPE_READ */
	"READ中に変な値がありました",
	
	/* BS_M_COMPILE_ERROR */
	"コンパイル中にエラーが起きました.\n",
	
	/* BS_M_START_PROGRAM */
	"'EDIT'と打ってプログラミングを始めよう!\n",

	/* BS_M_RUNNING */
	"%s を実行しています...\n",
	
	/* BS_M_PROG_DELETED_YN */
	"プログラムは消去されます。本当にnewしてもいいですか? (y/N) ",
	
	/* BS_M_EXIT_IN_DIRECT */
	"EXITはダイレクトモードでは使えません. (QUITのつもり?)\n",
	
	/* BS_M_PROG_SAVE_YN */
	"プログラムを保存していません。本当にQUITしますか? (y/N) ",

	/* BS_M_FINISHED */
	"[[ Daruma BASICを終了します.またね! ]]\n",
	
	/* BS_M_ESC_END_EDIT */
	"<esc>=edit終了",

	/* BS_M_OUT_OF_MEMORY */
	"メモリが足りません",
	
	/* BS_M_CANNOT_OPEN */
	"ファイル %s が開けません\n",
	
	/* BS_M_SOURCE_TOO_LARGE */
	"ソースファイルが大きすぎます\n",
	
	/* BS_M_NO_PROGRAM_TO_SAVE */
	"保存するプログラムがありません\n",

	/* BS_M_FILE_EXISTS */
	"ファイル %s はすでにあります。上書きする? (y/N) ",
	
	/* BS_M_SAVE_CANCELED */
	"保存をキャンセルしました.\n",
	
	/* BS_M_CANNOT_WRITE */
	"ファイル %s に保存できません",
	
	/* BS_M_NO_DIRECT */
	"%sはダイレクトモードでしか使えません."

	/* BS_M_INVALID_FNAME */
	"ファイル名が無効です",

};


const char *original_messages[] = {
	"syntax error",
	"syntax error: cannot back up",
	"syntax error, unexpected ",
	", expecting ",
	" or ",
	"memory exhausted",
};

const char *translated_messages[] = {
	"文法エラー",
	"文法エラー:読みもどしできません",
	"文法エラー: ",
	"は誤り,これが来るはず:",
	",",
	"メモリが足りません",
};

const char *
bs_translate_msg(const char *msg)
{
	static char smsg[256];
	char *p;
	int i, n1, n2, len;
	strncpy(smsg, msg, sizeof(smsg) - 1);
	smsg[sizeof(smsg) - 1] = 0;
	len = strlen(smsg);
	while (1) {
		for (i = sizeof(original_messages) / sizeof(original_messages[0]) - 1; i >= 0; i--) {
			if ((p = strstr(smsg, original_messages[i])) != NULL)
				break;
		}
		if (p == NULL)
			break;
		n1 = strlen(original_messages[i]);
		n2 = strlen(translated_messages[i]);
		if (len + (n2 - n1) < sizeof(smsg) - 1) {
			/* smsg...p...p+n1...0 -> smsg...p...p+n2...0  */
			memmove(p + n2, p + n1, len - (p - smsg) - n1 + 1);
			memmove(p, translated_messages[i], n2);
			len += n2 - n1;
		} else break;
	}
	smsg[len] = 0;
	return smsg;
}

const char *
bs_get_translated_msg(int msgid)
{
	const char *s = translated_err_msg[msgid];
	if (s != NULL)
		return s;
	else
		return err_msg[msgid];
}

/*  Replace yytnamerr in y.tab.c  */
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
 quotes and backslashes, so that it's suitable for yyerror.  The
 heuristic is that double-quoting is unnecessary unless the string
 contains an apostrophe, a comma, or backslash (other than
 backslash-backslash).  YYSTR is taken from yytname.  If YYRES is
 null, do not copy; instead, return the length of what the result
 would have been.  */
unsigned int
my_yytnamerr(char *yyres, const char *yystr)
{
	/*  This is a patch to remove 'BS_' from the token name  */
	if (strncmp(yystr, "BS_", 3) == 0)
		yystr += 3;
	/*  End patch  */
	
	if (*yystr == '"')
    {
		unsigned int yyn = 0;
		char const *yyp = yystr;
		
		for (;;)
			switch (*++yyp)
		{
			case '\'':
			case ',':
				goto do_not_strip_quotes;
				
			case '\\':
				if (*++yyp != '\\')
					goto do_not_strip_quotes;
				/* Fall through.  */
			default:
				if (yyres)
					yyres[yyn] = *yyp;
				yyn++;
				break;
				
			case '"':
				if (yyres)
					yyres[yyn] = '\0';
				return yyn;
		}
    do_not_strip_quotes: ;
    }
	
	if (! yyres)
		return strlen(yystr);
	
	return stpcpy(yyres, yystr) - yyres;
}
