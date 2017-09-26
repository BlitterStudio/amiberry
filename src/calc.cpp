/*
* UAE - The Un*x Amiga Emulator
*
* Infix->RPN conversion and calculation
*
*/


/*

 Original code from http://en.wikipedia.org/wiki/Shunting_yard_algorithm

*/

#define calc_log(x)

#include "sysconfig.h"
#include "sysdeps.h"

#include "calc.h"

#include <string.h>
#include <stdio.h>
 
#define STACK_SIZE 32
#define MAX_VALUES 32
#define IOBUFFERS 256

static double parsedvalues[MAX_VALUES];

// operators
// precedence   operators       associativity
// 1            !               right to left
// 2            * / %           left to right
// 3            + -             left to right
// 4            =               right to left
static int op_preced(const TCHAR c)
{
    switch(c)    {
        case '!':
            return 4;
		case '*':  case '/': case '\\': case '%':
            return 3;
        case '+': case '-':
            return 2;
        case '=':
            return 1;
    }
    return 0;
}
 
static bool op_left_assoc(const TCHAR c)
{
    switch(c)    {
        // left to right
        case '*': case '/': case '%': case '+': case '-':
            return true;
        // right to left
        case '=': case '!':
            return false;
    }
    return false;
}
 
static unsigned int op_arg_count(const TCHAR c)
{
    switch(c)  {
        case '*': case '/': case '%': case '+': case '-': case '=':
            return 2;
        case '!':
            return 1;
        default:
            return c - 'A';
    }
    return 0;
}
 
#define is_operator(c)  (c == '+' || c == '-' || c == '/' || c == '*' || c == '!' || c == '%' || c == '=')
#define is_function(c)  (c >= 'A' && c <= 'Z')
#define is_ident(c)     ((c >= '0' && c <= '9') || (c >= 'a' && c <= 'z'))
 
static bool shunting_yard(const TCHAR *input, TCHAR *output)
{
    const TCHAR *strpos = input, *strend = input + _tcslen(input);
    TCHAR c, *outpos = output;
 
    TCHAR stack[STACK_SIZE];       // operator stack
    unsigned int sl = 0;  // stack length
    TCHAR    sc;          // used for record stack element
 
    while(strpos < strend)   {
		if (sl >= STACK_SIZE)
			return false;

		// read one token from the input stream
        c = *strpos;
        if(c != ' ')    {
            // If the token is a number (identifier), then add it to the output queue.
            if(is_ident(c))  {
                *outpos = c; ++outpos;
            }
            // If the token is a function token, then push it onto the stack.
            else if(is_function(c))   {
                stack[sl] = c;
                ++sl;
            }
            // If the token is a function argument separator (e.g., a comma):
            else if(c == ',')   {
                bool pe = false;
                while(sl > 0)   {
                    sc = stack[sl - 1];
                    if(sc == '(')  {
                        pe = true;
                        break;
                    }
                    else  {
                        // Until the token at the top of the stack is a left parenthesis,
                        // pop operators off the stack onto the output queue.
                        *outpos = sc; 
                        ++outpos;
                        sl--;
                    }
                }
                // If no left parentheses are encountered, either the separator was misplaced
                // or parentheses were mismatched.
                if(!pe)   {
                    calc_log ((_T("Error: separator or parentheses mismatched\n")));
                    return false;
                }
            }
            // If the token is an operator, op1, then:
            else if(is_operator(c))  {
                while(sl > 0)    {
                    sc = stack[sl - 1];
                    // While there is an operator token, o2, at the top of the stack
                    // op1 is left-associative and its precedence is less than or equal to that of op2,
                    // or op1 is right-associative and its precedence is less than that of op2,
                    if(is_operator(sc) &&
                        ((op_left_assoc(c) && (op_preced(c) <= op_preced(sc))) ||
                           (!op_left_assoc(c) && (op_preced(c) < op_preced(sc)))))   {
                        // Pop o2 off the stack, onto the output queue;
                        *outpos = sc; 
                        ++outpos;
                        sl--;
                    }
                    else   {
                        break;
                    }
                }
                // push op1 onto the stack.
                stack[sl] = c;
                ++sl;
            }
            // If the token is a left parenthesis, then push it onto the stack.
            else if(c == '(')   {
                stack[sl] = c;
                ++sl;
            }
            // If the token is a right parenthesis:
            else if(c == ')')    {
                bool pe = false;
                // Until the token at the top of the stack is a left parenthesis,
                // pop operators off the stack onto the output queue
                while(sl > 0)     {
                    sc = stack[sl - 1];
                    if(sc == '(')    {
                        pe = true;
                        break;
                    }
                    else  {
                        *outpos = sc; 
                        ++outpos;
                        sl--;
                    }
                }
                // If the stack runs out without finding a left parenthesis, then there are mismatched parentheses.
                if(!pe)  {
                    calc_log ((_T("Error: parentheses mismatched\n")));
                    return false;
                }
                // Pop the left parenthesis from the stack, but not onto the output queue.
                sl--;
                // If the token at the top of the stack is a function token, pop it onto the output queue.
                if(sl > 0)   {
                    sc = stack[sl - 1];
                    if(is_function(sc))   {
                        *outpos = sc; 
                        ++outpos;
                        sl--;
                    }
                }
            }
            else  {
                calc_log ((_T("Unknown token %c\n"), c));
                return false; // Unknown token
            }
        }
        ++strpos;
    }
    // When there are no more tokens to read:
    // While there are still operator tokens in the stack:
    while(sl > 0)  {
        sc = stack[sl - 1];
        if(sc == '(' || sc == ')')   {
            printf("Error: parentheses mismatched\n");
            return false;
        }
        *outpos = sc; 
        ++outpos;
        --sl;
    }
    *outpos = 0; // Null terminator
    return true;
}
 

struct calcstack
{
	TCHAR *s;
	double val;
};

static double docalcx(TCHAR op, double v1, double v2)
{
	switch (op)
	{
		case '-':
		return v1 - v2;
		case '+':
		return v1 + v2;
		case '*':
		return v1 * v2;
		case '/':
		return v1 / v2;
		case '\\':
		return (int)v1 % (int)v2;

	}
	return 0;
}

static double stacktoval(struct calcstack *st)
{
	if (st->s) {
		if (_tcslen(st->s) == 1 && st->s[0] >= 'a' && st->s[0] <= 'z')
			return parsedvalues[st->s[0] - 'a'];
		return _tstof (st->s);
	} else {
		return st->val;
	}
}

static double docalc2(TCHAR op, struct calcstack *sv1, struct calcstack *sv2)
{
	double v1, v2;

	v1 = stacktoval(sv1);
	v2 = stacktoval(sv2);
	return docalcx (op, v1, v2);
}
static double docalc1(TCHAR op, struct calcstack *sv1, double v2)
{
	double v1;

	v1 = stacktoval(sv1);
	return docalcx (op, v1, v2);
}

static TCHAR *chartostack(TCHAR c)
{
	TCHAR *s = xmalloc (TCHAR, 2);
	s[0] = c;
	s[1] = 0;
	return s;
}

static bool execution_order(const TCHAR *input, double *outval)
{
    const TCHAR *strpos = input, *strend = input + _tcslen(input);
    TCHAR c, res[4];
    unsigned int sl = 0, rn = 0;
	struct calcstack stack[STACK_SIZE] = { { 0 } }, *sc, *sc2;
	double val = 0;
	int i;
	bool ok = false;

	// While there are input tokens left
    while(strpos < strend)  {

		if (sl >= STACK_SIZE)
			return false;

               // Read the next token from input.
		c = *strpos;
                // If the token is a value or identifier
        if(is_ident(c))    {
                        // Push it onto the stack.
			stack[sl].s = chartostack (c);
            ++sl;
        }
                // Otherwise, the token is an operator  (operator here includes both operators, and functions).
        else if(is_operator(c) || is_function(c))    {
                        _stprintf(res, _T("_%02d"), rn);
                        calc_log ((_T("%s = "), res));
                        ++rn;
                        // It is known a priori that the operator takes n arguments.
                        unsigned int nargs = op_arg_count(c);
                        // If there are fewer than n values on the stack
                        if(sl < nargs) {
                                // (Error) The user has not input sufficient values in the expression.
                                return false;
                        }
                        // Else, Pop the top n values from the stack.
                        // Evaluate the operator, with the values as arguments.
                        if(is_function(c)) {
                                calc_log ((_T("%c("), c));
                                while(nargs > 0){
                                        sc = &stack[sl - nargs]; // to remove reverse order of arguments
                                        if(nargs > 1)   {
                                                calc_log ((_T("%s, "), sc));
                                        }
                                        else {
                                                calc_log ((_T("%s)\n"), sc));
                                        }
                                        --nargs;
                                }
                                sl-=op_arg_count(c);
                        }
                        else   {
                                if(nargs == 1) {
                                        sc = &stack[sl - 1];
                                        sl--;
										val = docalc1 (c, sc, val);
										calc_log ((_T("%c %s = %f;\n"), c, stacktostr(sc), val));
                               }
                                else   {
                                        sc = &stack[sl - 2];
                                        calc_log ((_T("%s %c "), stacktostr(sc), c));
                                        sc2 = &stack[sl - 1];
										val = docalc2 (c, sc, sc2);
                                         sl--;sl--;
                                        calc_log ((_T("%s = %f;\n"), stacktostr(sc2), val));
                               }
                        }
                        // Push the returned results, if any, back onto the stack.
						stack[sl].val = val;
						stack[sl].s = NULL;
            ++sl;
        }
        ++strpos;
    }
        // If there is only one value in the stack
        // That value is the result of the calculation.
        if(sl == 1) {
                sc = &stack[sl - 1];
                sl--;
				calc_log ((_T("result = %f\n"), val));
				if (outval)
					*outval = val;
				ok = true;
		}
		for (i = 0; i < STACK_SIZE; i++)
			xfree (stack[i].s);
 
		// If there are more values in the stack
        // (Error) The user input has too many values.

		return ok;
}

static bool is_separator(TCHAR c)
{
	if (is_operator(c))
		return true;
	return c == 0 || c == ')' || c == ' ';
}

static bool parse_values(const TCHAR *ins, TCHAR *out)
{
	int ident = 0;
	TCHAR tmp;
	TCHAR inbuf[IOBUFFERS];
	int op;

	_tcscpy (inbuf, ins);
	TCHAR *in = inbuf;
	TCHAR *p = out;
	op = 0;
	if (in[0] == '-' || in[0] == '+') {
		*p++ = '0';
	}
	while (*in) {
		TCHAR *instart = in;
		if (!_tcsncmp(in, _T("true"), 4) && is_separator(in[4])) {
			in[0] = '1';
			in[1] = ' ';
			in[2] = ' ';
			in[3] = ' ';
		} else if (!_tcsncmp(in, _T("false"), 5) && is_separator(in[5])) {
			in[0] = '0';
			in[1] = ' ';
			in[2] = ' ';
			in[3] = ' ';
			in[4] = ' ';
		}
		if (_istdigit (*in)) {
			if (ident >= MAX_VALUES)
				return false;
			if (op > 1 && (in[-1] == '-' || in[-1] == '+')) {
				instart--;
				p--;
			}
			*p++ = ident + 'a';
			while (_istdigit (*in) || *in == '.')
				in++;
			tmp = *in;
			*in = 0;
			parsedvalues[ident++] = _tstof (instart);
			*in = tmp;
			op = 0;
		} else {
			if (is_operator(*in))
				op++;
			*p++ = *in++;
		}
	}
	*p = 0;
	return true;
}

bool calc(const TCHAR *input, double *outval)
{
    TCHAR output[IOBUFFERS], output2[IOBUFFERS];
    calc_log ((_T("IN: '%s'\n"), input));
	if (parse_values(input, output2)) {
		if(shunting_yard(output2, output))    {
			calc_log ((_T("RPN OUT: %s\n"), output));
			if(!execution_order(output, outval)) {
				calc_log ((_T("PARSE ERROR!\n")));
			} else {
				return true;
			}
		}
    }
    return false;
}

bool iscalcformula (const TCHAR *formula)
{
	for (int i = 0; i < _tcslen (formula); i++) {
		TCHAR c = formula[i];
		if (is_operator (c))
			return true;
	}
	return false;
}

