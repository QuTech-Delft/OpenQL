/* YYSTYPE union */
%union {
    char           	*str;
    IntegerLiteral  *ilit;
    FloatLiteral    *flit;
    MatrixLiteral   *mat;
    StringBuilder   *strb;
    StringLiteral   *slit;
    JsonLiteral     *jlit;
    Identifier      *idnt;
    FunctionCall    *func;
    Index           *indx;
    UnaryOp         *unop;
    BinaryOp        *biop;
    TernaryCond     *tcnd;
    Expression      *expr;
    ExpressionList  *expl;
    IndexItem       *idxi;
    IndexRange      *idxr;
    IndexEntry      *idxe;
    IndexList       *idxl;
    AnnotationData  *adat;
    Instruction     *inst;
    Bundle          *bun;
    Mapping         *map;
    Variables       *vars;
    Subcircuit      *sub;
    Assignment      *asgn;
    IfElse          *ifel;
    ForLoop         *forl;
    ForeachLoop     *fore;
    WhileLoop       *whil;
    RepeatUntilLoop *repu;
    BreakStatement  *brk;
    ContinueStatement *cont;
    Statement       *stmt;
    StatementList   *stms;
    Version         *vers;
    Program         *prog;
};

/* Typenames for nonterminals */
%type <ilit> IntegerLiteral
%type <flit> FloatLiteral
%type <idnt> Identifier
%type <func> FunctionCall
%type <indx> Index
%type <indx> IndexNP
%type <unop> UnaryOp
%type <unop> UnaryOpNP
%type <biop> BinaryOp
%type <biop> BinaryOpNP
%type <tcnd> TernaryOp
%type <tcnd> TernaryOpNP
%type <expr> Expression
%type <expr> ExpressionNP
%type <expl> ExpressionList
%type <expl> ExpressionListNP
%type <idxi> IndexItem
%type <idxr> IndexRange
%type <idxe> IndexEntry
%type <idxl> IndexList
%type <adat> AnnotationName AnnotationData
%type <inst> Instruction AnnotInstr
%type <bun>  SLParInstrList CBParInstrList
%type <map>  Mapping
%type <vars> Variable VariableBody
%type <sub>  Subcircuit
%type <asgn> Assignment OptAssignment
%type <ifel> IfElse
%type <forl> ForLoop
%type <fore> ForeachLoop
%type <whil> WhileLoop
%type <repu> RepeatUntilLoop
%type <brk>  Break
%type <cont> Continue
%type <stmt> Statement AnnotStatement
%type <stms> StatementList Statements SubStatements
%type <vers> Version
%type <prog> Program

/* Function calls. */
FunctionCall    : Identifier '(' ')' %prec '('                                  { NEW($$, FunctionCall); $$->name.set_raw($1); $$->arguments.set_raw(new ExpressionList()); }
                | Identifier '(' ExpressionList ')' %prec '('                   { NEW($$, FunctionCall); $$->name.set_raw($1); $$->arguments.set_raw($3); }
                ;


/* Subcircuit header statement. */
Subcircuit      : '.' Identifier                                                { NEW($$, Subcircuit); $$->name.set_raw($2); }
                | '.' Identifier '(' Expression ')'                             { NEW($$, Subcircuit); $$->name.set_raw($2); $$->iterations.set_raw($4); }
                ;

/* cQASM 1.2 statements. */
SubStatements   : '{' OptNewline StatementList OptNewline '}'                   { FROM($$, $3); }
                | '{' OptNewline '}'                                            { NEW($$, StatementList); }
                ;

Assignment      : Expression '=' Expression                                     { NEW($$, Assignment); $$->lhs.set_raw($1); $$->rhs.set_raw($3); }
                ;

OptAssignment   : Assignment                                                    { FROM($$, $1); }
                |                                                               { $$ = nullptr; }
                ;

IfElse          : IF '(' Expression ')' SubStatements                           {
                                                                                    NEW($$, IfElse);
                                                                                    $$->branches.add_raw(new IfElseBranch());
                                                                                    $$->branches[0]->condition.set_raw($3);
                                                                                    $$->branches[0]->body.set_raw($5);
                                                                                }
                | IF '(' Expression ')' SubStatements ELSE IfElse               {
                                                                                    FROM($$, $7);
                                                                                    $$->branches.add_raw(new IfElseBranch(), 0);
                                                                                    $$->branches[0]->condition.set_raw($3);
                                                                                    $$->branches[0]->body.set_raw($5);
                                                                                }
                | IF '(' Expression ')' SubStatements ELSE SubStatements        {
                                                                                    NEW($$, IfElse);
                                                                                    $$->branches.add_raw(new IfElseBranch());
                                                                                    $$->branches[0]->condition.set_raw($3);
                                                                                    $$->branches[0]->body.set_raw($5);
                                                                                    $$->otherwise.set_raw($7);
                                                                                }
                ;

ForLoop         : FOR '(' OptAssignment NEWLINE Expression NEWLINE
                    OptAssignment ')' SubStatements                             {
                                                                                    NEW($$, ForLoop);
                                                                                    if ($3) $$->initialize.set_raw($3);
                                                                                    $$->condition.set_raw($5);
                                                                                    if ($7) $$->update.set_raw($7);
                                                                                    $$->body.set_raw($9);
                                                                                }
                ;

ForeachLoop     : FOREACH '(' Expression '=' Expression ELLIPSIS Expression
                    ')' SubStatements                                           {
                                                                                    NEW($$, ForeachLoop);
                                                                                    $$->lhs.set_raw($3);
                                                                                    $$->frm.set_raw($5);
                                                                                    $$->to.set_raw($7);
                                                                                    $$->body.set_raw($9);
                                                                                }
                ;

WhileLoop       : WHILE '(' Expression ')' SubStatements                        { NEW($$, WhileLoop); $$->condition.set_raw($3); $$->body.set_raw($5); }
                ;

RepeatUntilLoop : REPEAT SubStatements UNTIL '(' Expression ')'                 { NEW($$, RepeatUntilLoop); $$->body.set_raw($2); $$->condition.set_raw($5); }
                ;

Continue        : CONTINUE                                                      { NEW($$, ContinueStatement); }
                ;

Break           : BREAK                                                         { NEW($$, BreakStatement); }
                ;

/* FIXME: statement recovery fails even on simple things like
    "this is an invalid statement" */

/* List of one or more statements. */
StatementList   : StatementList Newline AnnotStatement                          { FROM($$, $1); $$->items.add_raw($3); }
                | AnnotStatement                                                { NEW($$, StatementList); $$->items.add_raw($1); }
                ;

/* List of zero or more statements preceded by a newline. */
Statements      : Newline StatementList OptNewline                              { FROM($$, $2); }
                | OptNewline                                                    { NEW($$, StatementList); }
                ;
