#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define VERSION		"0.1"
#define CH_BLANK	'\0'
#define BUF_SIZE	256
#define COL_INIT	0

/* 전역 변수 */
int prevCh = CH_BLANK;
char lexeme[BUF_SIZE] = { 0, };
int chCnt = 0; // 읽어들인 어휘소 문자열 길이

FILE *fp = NULL;
FILE *out = NULL;
int lineCnt = 1;
int colCnt = COL_INIT;
int prevColCnt; // 이전 행의 열 변호 (개행 문자를 unget하는 경우에 사용됨)
int tokCol; // 토큰의 시작 부분을 가리키는 열 번호

char errMsg[BUF_SIZE];

enum Token {
	TOK_PUSH = 0,
	TOK_POPA,
	TOK_POPM,
	TOK_NEG,
	TOK_RCP,
	TOK_DUP,
	TOK_NUM, // operand
	TOK_SEP, // separator
	TOK_SEM, // comment (;)
	TOK_EOF
};

const char * const opcode[] = {
	"push",
	"popa",
	"popm",
	"neg",
	"rcp",
	"dup"
};

const char * const gencode[][4] = {
	{ "형", "혀", "어", "엉" }, // push
	{ "항", "하", "아", "앙" }, // popa
	{ "핫", "하", "아", "앗" }, // popm
	{ "흣", "흐", "으", "읏" }, // neg
	{ "흡", "흐", "으", "읍" }, // rcp
	{ "흑", "흐", "으", "윽" }  // dup
};

/* 함수 프로토타입 선언 */
int next(void);
void unget(int c);
int isBlank(int c);
int isNumber(const char *str);
void tolowerString(char *str);
void eatBlank(void);
int getOpcodeToken(const char *str);
int lex(void);
void comment(void);
int operand(int tokOpcode, int *op1, int *op2);
void operation(int tok);
void asm_error(const char *msg);
void usage(const char *arg);


// 입력 스트림에서 다음 문자를 반환한다.
int next(void) {
	int curCh;
	++colCnt;
	if (prevCh == CH_BLANK)
		curCh = fgetc(fp);
	else {
		curCh = prevCh;
		prevCh = CH_BLANK;
	}
	if (curCh == '\n')
		prevColCnt = colCnt;
	return curCh;
}

// 입력 스트림에 문자 하나를 되돌린다.
inline void unget(int c) {
	if (c == '\n')
		colCnt = prevColCnt;
	else
		--colCnt;
	prevCh = c;
}

inline int isBlank(int c) {
	return (c == ' ') || (c == '\t') || (c == '\n');
}

inline int isNumber(const char *str) {
	const char *ptr = str;
	while (*ptr) {
		if (!isdigit(*ptr++))
			return 0;
	}
	return 1;
}

void tolowerString(char *str) {
	char *ptr = str;
	while (*ptr) {
		*ptr = tolower(*ptr);
		ptr++;
	}
}

// 입력 스트림으로부터 다음 토큰이 나올 때까지 공백 문자들을 무시한다.
void eatBlank(void) {
	int curCh;
	
	//puts("eatBlank()");
	while (1) {
		curCh = next();
		if (curCh == '\n')
			colCnt = COL_INIT;
		if (curCh == '\r')
			continue;
		if (!isBlank(curCh) || curCh != '\r') {
			unget(curCh);
			break;
		}
	}
}

int getOpcodeToken(const char *str) {
	int i;
	for (i = 0; i <= TOK_DUP; i++)
		if (!strcmp(str, opcode[i]))
			return i;
	return -1;
}

// 입력스트림으로부터 문자열을 읽어 하나의 토큰 단위로 분리시켜 반환한다.
int lex(void) {
	int tok;
	int curCh;
	
	chCnt = 0; // lexeme의 길이를 0으로 초기화한다.
	
	// 반복문을 실행하기 전에 단일 문자로 구성된 토큰을 인식한다.
	curCh = next();
	if (curCh == EOF || curCh == ',' || curCh == ';') {
		lexeme[chCnt++] = curCh;
		lexeme[chCnt] = 0;
	}
	
	//printf("[line %d col %d]'%c'(%d)\n", lineCnt, colCnt, curCh, curCh);
	if (curCh == EOF)
		return TOK_EOF;
	else if (curCh == ',')
		return TOK_SEP;
	else if (curCh == ';')
		return TOK_SEM;
	else if (isBlank(curCh))
		eatBlank();
	else
		unget(curCh);
	
	while (1) {
		curCh = next();
		if (curCh == EOF || curCh == ',' || curCh == ';') {
			unget(curCh);
			break;
		} else if (isBlank(curCh)) { // 토큰 다음에 공백이 있을 경우, 공백을 먼저 제거한 후에 토큰 인식을 종료한다.
			eatBlank();
			break;
		} else if (isalnum(curCh)) {
			if (chCnt == 0)
				tokCol = colCnt;
			lexeme[chCnt++] = curCh;
		} else {
			fprintf(stderr, "[line %d, col %d] lex(): unknown character - curCh=%c(%d)\n", lineCnt, colCnt, curCh, curCh);
			exit(1);
		}
	}
	// 다음 토큰의 문자를 만나 루프가 종료되면 이전까지 읽어들인 문자열을 처리해야 한다.
	lexeme[chCnt] = 0; // 문자 배열의 널 종료 처리
	tolowerString(lexeme); // 문자열 비교를 위한 소문자화
	//printf("lexeme: %s\n", lexeme);
	tok = getOpcodeToken(lexeme);
	if (tok != -1)
		return tok;
	else if (isNumber(lexeme))
		return TOK_NUM;
	else {
		sprintf(errMsg, "unknown symbol \"%s\"", lexeme);
		asm_error(errMsg);
	}
	
	return -1;
}

// 입력 스트림으로부터 주석을 읽어들여 무시한다.
void comment(void) {
	//puts("comment()");
	int c;
	while (1) {
		c = next();
		if (c == '\n')
			break;
		else if (c == EOF) {
			unget(c);
			return;
		}
	}
	++lineCnt;
	colCnt = COL_INIT;
}

int operand(int tokOpcode, int *op1, int *op2) {
	int tok;
	const char *msg = "illegal operand";
	
	tok = lex();
	if (tok != TOK_NUM)
		asm_error(msg);
	*op1 = atoi(lexeme);
	
	tok = lex();
	if (tok != TOK_SEP)
		asm_error(msg);
	
	tok = lex();
	if (tok != TOK_NUM)
		asm_error(msg);
	*op2 = atoi(lexeme);
	
	return 0;
}

void operation(int tok) {
	int op1, op2;
	int i;
	
	if (tok > TOK_DUP) {
		sprintf(errMsg, "tok=%d is not a opcode", tok);
		asm_error(errMsg);
	}
	
	operand(TOK_PUSH, &op1, &op2);
	
	if (op1 < 0) {
		sprintf(errMsg, "%d is illegal argument", op1);
		asm_error(errMsg);
	} else if (op2 < 0) {
		sprintf(errMsg, "%d is illegal argument", op2);
		asm_error(errMsg);
	}
	
	if (op1 < 2)
		fputs(gencode[tok][0], out);
	else {
		fputs(gencode[tok][1], out);
		for (i = 0; i < op1 - 2; i++)
			fputs(gencode[tok][2], out);
		fputs(gencode[tok][3], out);
	}
	for (i = 0; i < op2; i++)
		fputs(".", out);
	fputs("\n", out);
}

void asm_error(const char *msg) {
	fprintf(stderr, "error occurred at line: %d, column: %d\n", lineCnt, tokCol);
	fputs(msg, stderr);
	fputc('\n', stderr);
	exit(1);
}

void usage(const char *arg) {
	puts("hyeong pseudo assembler Ver. "VERSION);
	puts("(C)2020 catnip1917");
	printf("usage: %s src dest\n", arg);
}

int main(int argc, char *argv[]) {
	int tok;
	int ret;
	
	if (argc != 3) {
		usage(argv[0]);
		return 1;
	}
	
	fp = fopen(argv[1], "rt");
	if (!fp) {
		perror(argv[1]);
		return 1;
	}
	
	out = fopen(argv[2], "wt");
	if (!out) {
		perror(argv[2]);
		return 1;
	}
	
	while ((tok = lex()) != TOK_EOF) {
		//printf("[%d, %d]tok: %d, lexeme: \"%s\"\t=> ", lineCnt, tokCol, tok, lexeme);
		switch (tok) {
		case TOK_SEM:
			//printf("comment!\n");
			comment();
			break;
		case TOK_PUSH: case TOK_POPA: case TOK_POPM:
		case TOK_NEG: case TOK_RCP: case TOK_DUP:
			//printf("opcode! (%d)\n", tok);
			operation(tok);
			break;
		case TOK_NUM:
			//printf("number! (%d)", atoi(lexeme));
			break;
		case TOK_EOF:
			//printf("EOF");
			break;
		default:
			sprintf(errMsg, "unkown token \"%s\"(%d)", lexeme, tok);
			asm_error(errMsg);
			break;
		}
		//puts("");
	}
	
	fclose(fp);
	fclose(out);
	return 0;
}
