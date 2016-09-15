import re
import copy

def tokenize(s):
    tokens = [
        ('STR', r'"[^"]*"'),
        ('END', r';'),
        ('EQU', r'='),
        ('SKP', r'\s+'),
        ('TOK', r'[^;\s]+'),
    ]
    tok_re = '|'.join('(?P<%s>%s)' % p for p in tokens)
    for m in re.finditer(tok_re, s):
        value = m.group()
        kind = m.lastgroup
        if kind == 'SKP':
            continue
        if kind == 'STR':
            value = value[1:-1]
        yield (kind, value, m.start())

def parse_tokens(f):
    line_no = 1
    for line in f:
        line = re.sub(r'#.*$', '', line.strip())
        for token in tokenize(line):
            yield (*token, line_no)
        line_no += 1
    yield ('EOF', None, line_no - 1)

def parse(f, tree=None):
    if tree is None:
        tree = {'programmer': {}, 'serialadapter': {}, 'part': {}}
    gen = parse_tokens(f)

    def expect(*t):
        tok = next(gen)
        if tok[0] not in t:
            raise Exception(f'Unexpected token on line {tok[3]}, column {tok[2]}')
        return tok[0], tok[1]

    def get_equ():
        expect('EQU')
        s = []
        while True:
            kind, v = expect('TOK', 'STR', 'END')
            if kind == 'END':
                break
            s.append(v)
        return ' '.join(s)
    while True:
        kind, sect = expect('TOK', 'EOF')
        if kind == 'EOF':
            break
        if sect not in ('programmer', 'serialadapter', 'part'):
            tree[sect] = get_equ()
            continue

        obj = {}
        if sect == 'part':
            obj['memory'] = {}
        while True:
            kind, opt = expect('TOK', 'END')
            if kind == 'END':
                break
            if opt == 'parent':
                _, name = expect('TOK', 'STR')
                obj = copy.deepcopy(tree[sect][name])
            elif opt == 'memory' and sect == 'part':
                mem = {}
                _, name = expect('TOK', 'STR')
                while True:
                    kind, sub = expect('TOK', 'END')
                    if kind == 'END':
                        break
                    if sub == 'alias':
                        _, alias = expect('TOK', 'STR')
                        mem = copy.deepcopy(obj[opt][alias])
                        expect('END')
                    else:
                        mem[sub] = get_equ()
                obj[opt][name] = mem
            else:
                obj[opt] = get_equ()
        for _id in [s.strip() for s in obj['id'].split(',')]:
            tree[sect][_id] = obj
    return tree

def signatures(tree):
    sigs = {}
    for _id, obj in tree['part'].items():
        sig = obj.get('signature', None)
        if sig:
            sigs[''.join([f'{int(n, 0x10):02x}' for n in sig.split()])] = _id
    return sigs
