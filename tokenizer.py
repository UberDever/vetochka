# pylint: disable=missing-module-docstring
# pylint: disable=missing-class-docstring
# pylint: disable=missing-function-docstring

from dataclasses import dataclass


@dataclass
class Location:

    def __init__(self, s=-1, e=-1, ln=-1, c=-1):
        self.start = s
        self.end = e
        self.line = ln
        self.col = c

    start: int
    end: int
    line: int
    col: int


@dataclass
class Token:
    s: str
    loc: Location = Location()


@dataclass
class Tree(Token):

    def __init__(self, s='^', loc=Location()):
        super().__init__(s, loc)


@dataclass
class Symbol(Token):
    pass


@dataclass
class Delim(Token):
    pass


@dataclass
class String(Token):
    pass


@dataclass
class Delimeters:
    rsquare = Delim('[')
    lsquare = Delim(']')
    rparen = Delim('(')
    lparen = Delim(')')
    rcurly = Delim('{')
    lcurly = Delim('}')
    comma = Delim(',')


class Word:

    def __init__(self, s=-1, e=-1, ln=-1, c=-1):
        self.word = []
        self.start = s
        self.end = e
        self.line = ln
        self.col = c

    def add(self, letter):
        self.word.append(letter)

    def reset(self):
        self.word = []

    def compose(self):
        return ''.join(self.word)

    def valid(self) -> bool:
        return self.word


def tokenize(byte_array: bytearray) -> [Token]:
    line = 1
    col = 1

    in_string = False
    tokens = []
    word = Word(ln=line, c=col)
    block_level = -1

    def flush_word(token_ctor):
        nonlocal word, tokens
        nonlocal line, col
        if word.valid():
            tokens.append(
                token_ctor(word.compose(),
                           loc=Location(word.start, word.end, word.line,
                                        word.col)))
            word.line = line
            word.col = col
            word.reset()

    for i, b in enumerate(byte_array):
        if not in_string:
            match b:
                case '^':
                    flush_word(Symbol)
                    tokens.append(Tree(loc=Location(i, i + 1, line, col)))
                case ')' | '(' | '[' | ']' | ',':
                    flush_word(Symbol)
                    tokens.append(Delim(b, loc=Location(i, i + 1, line, col)))
                case '{':
                    in_string = True
                    col += 1
                    continue
                case '}':
                    assert False
                case '\n':
                    line += 1
                    col = 0
                    flush_word(Symbol)
                case _ if b.isspace():
                    flush_word(Symbol)
                case _:
                    word.add(b)
        else:
            match b:
                case '{':
                    block_level += 1
                    word.add(b)
                case '}':
                    if block_level >= 0:
                        block_level -= 1
                        word.add(b)
                    else:
                        in_string = False
                        assert word.valid()
                        flush_word(String)
                case '\n':
                    line += 1
                    col = 0
                    word.add(b)
                case _:
                    word.add(b)
        col += 1

    if in_string:
        assert not word.valid()
    flush_word(Symbol)

    if block_level != -1:
        raise RuntimeError(
            "Can't tokenize, encountered unbalanced expression along the way")
    return tokens
