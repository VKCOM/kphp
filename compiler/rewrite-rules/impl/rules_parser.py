from .rules_lexer import Lexer, Token
from typing import Optional, List


class Expr:
    OP_ANY = "__op_any__"  # a pseudo-op for freestanding vars

    def __init__(self, op: str, line: int):
        self.op: str = op
        self.name: str = ""  # a name given inside the rule sources, like "x" in x:(...)
        self.vertex_string: str = ""
        self.members: List[Expr] = []
        self.dot3pos: Optional[int] = None
        self.source: str = ""
        self.line: int = line


class LetVar:
    def __init__(self, name: str, expr: str, checked: bool, line: int):
        self.name: str = name
        self.expr: str = expr
        self.checked: bool = checked
        self.line = line


class Rule:
    def __init__(self, match_expr: Expr, rewrite_expr: Expr, cond: str, let_list: List[LetVar], line: int):
        self.match_expr: Expr = match_expr
        self.rewrite_expr: Expr = rewrite_expr
        self.cond: str = cond
        self.let_list: List[LetVar] = let_list
        self.line: int = line


class RulesParser:
    def __init__(self, rules_filename: str, src: str):
        self.rules_filename: str = rules_filename
        self.lexer: Lexer = Lexer(self.rules_filename, src)

    def parse_file(self) -> List[Rule]:
        rules = []
        while self.lexer.peek().kind != 'EOF':
            rules.append(self.parse_rule())
        return rules

    def tok_error(self, tok: Token, message: str):
        raise RuntimeError(f'{self.rules_filename}:{tok.line}:{tok.column}: {message}')

    def expect_token(self, kind: str) -> Token:
        tok = self.lexer.scan()
        if tok.kind != kind:
            self.tok_error(tok, f'expected {kind}, found {tok.value} ({tok.kind})')
        return tok

    def try_consume(self, kind: str) -> bool:
        if self.lexer.peek().kind == kind:
            self.lexer.scan()
            return True
        return False

    def parse_rule(self) -> Rule:
        line = self.lexer.line()
        match_expr = self.parse_expr(None, -1)
        cond = ''
        if self.try_consume('KEYWORD_WHERE'):
            cond = self.expect_token('VERBATIM').value
        let_list: List[LetVar] = []
        while True:
            checked = False
            if self.try_consume('KEYWORD_IF'):
                checked = True
            if self.try_consume('KEYWORD_LET'):
                let_line = self.lexer.line()
                var_name = self.expect_token('IDENT').value
                var_init = self.expect_token('VERBATIM').value
                let_list.append(LetVar(var_name, var_init, checked, let_line))
                continue
            break
        self.expect_token('ARROW')
        rewrite_expr = self.parse_expr(None, -1)
        return Rule(match_expr, rewrite_expr, cond, let_list, line)

    def parse_expr(self, parent: Optional[Expr], member_index) -> Optional[Expr]:
        tok = self.lexer.scan()
        tok_line = tok.line
        if tok.kind == 'LPAREN':
            op = self.expect_token('IDENT')
            e = Expr(op.value, tok_line)
            if self.lexer.peek().kind == 'VERBATIM':
                e.vertex_string = self.lexer.scan().value
            i = 0
            while self.lexer.peek().kind != 'RPAREN':
                member = self.parse_expr(e, i)
                if member:
                    e.members.append(member)
                i += 1
            end_tok = self.expect_token('RPAREN')
            e.source = self.lexer.get_source(tok.start_pos, end_tok.end_pos)
            return e
        elif tok.kind == 'IDENT':
            name = tok.value
            if self.try_consume('COLON'):
                e = self.parse_expr(parent, member_index)
                e.name = name
                return e
            e = Expr(Expr.OP_ANY, tok_line)
            e.name = name
            return e
        elif tok.kind == 'DOT3':
            parent.dot3pos = member_index
            return None
        elif tok.kind == 'IDENT_DOT3':
            if member_index != 0:
                self.tok_error(tok, f"{tok.value}... can only be used as a first argument")
            parent.dot3pos = member_index
            e = Expr(Expr.OP_ANY, tok_line)
            e.name = tok.value
            return e
        else:
            self.tok_error(tok, f'parsing match expr: expected (, ... or IDENT, found {tok.value}')
