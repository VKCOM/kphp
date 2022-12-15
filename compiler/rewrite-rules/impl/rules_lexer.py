from typing import NamedTuple, Optional, Iterator
import re


class Token(NamedTuple):
    kind: str
    value: str
    line: int
    column: int
    start_pos: int
    end_pos: int


class Lexer:
    def __init__(self, filename: str, src: str):
        self.next_token: Optional[Token] = None
        self.tokens_generator: Iterator[Token] = tokenize(filename, src)
        self.src: str = src

    def scan(self) -> Token:
        if self.next_token is not None:
            tok = self.next_token
            self.next_token = None
            return tok
        return self.__do_scan()

    def peek(self) -> Token:
        if self.next_token is not None:
            return self.next_token
        self.next_token = self.__do_scan()
        return self.next_token

    def line(self) -> int:
        return self.peek().line

    def get_source(self, begin: int, end: int) -> str:
        return self.src[begin:end].strip()

    def __do_scan(self) -> Token:
        return next(self.tokens_generator)


def tokenize(filename, src) -> Iterator[Token]:
    tokens = [
        ("SKIP",          r'[ \t]+'),
        ("COMMENT",       r';;[^\n]*'),
        ("VERBATIM",      r'\{.*?\}'),
        ("KEYWORD_WHERE", r'where'),
        ("KEYWORD_IF",    r'if'),
        ("KEYWORD_LET",   r'let'),
        ("INT_LIT",       r'\d+'),
        ("IDENT_DOT3",    r'[A-Za-z_]\w*\.\.\.'),
        ("IDENT",         r'[A-Za-z_]\w*'),
        ("LPAREN",        r'\('),
        ("RPAREN",        r'\)'),
        ("COLON",         r':'),
        ("ARROW",         r'=>'),
        ("DOT3",          r'\.\.\.'),
        ("NEWLINE",       r'\n'),
        ("BAD",           r'.'),
    ]
    tokens_regex = '|'.join('(?P<%s>%s)' % pair for pair in tokens)
    line_num = 1
    line_start = 0
    start_pos = 0
    end_pos = 0
    for mo in re.finditer(tokens_regex, src):
        kind = mo.lastgroup
        value = mo.group()
        column = mo.start() - line_start
        start_pos = mo.start()
        end_pos = mo.end()
        if kind == "COMMENT":
            continue
        if kind == "NEWLINE":
            line_start = mo.end()
            line_num += 1
            continue
        elif kind == "IDENT_DOT3":
            value = value[:-len("...")]
        elif kind == "VERBATIM":
            value = value[1:-1].strip()
        elif kind == "SKIP":
            continue
        elif kind == "BAD":
            raise RuntimeError(f'{filename}:{line_num}:{column}: unexpected token {value!r}')
        yield Token(kind, value, line_num, column, start_pos, end_pos)
    yield Token('EOF', '', line_num, 1, start_pos, end_pos)
