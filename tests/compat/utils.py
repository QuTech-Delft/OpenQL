import difflib


def file_compare(fn1, fn2):
    """
    Compare two cQASM files and raise an Assertion error if they are different,
    but ignoring for the # comment block that the files start with.

    The assertion error contains a diff of the files.
    """
    with open(fn1) as f, open(fn2) as g:
        flines = list(f.readlines())
        while flines and flines[0].startswith('#'):
            flines.pop(0)
        glines = list(g.readlines())
        while glines and glines[0].startswith('#'):
            glines.pop(0)
        d = difflib.Differ()
        diffs = [x for x in d.compare(flines, glines) if x[0] in ('+', '-')]
        if diffs:
            # all rows with changes
            raise AssertionError("Files not equal\n"+"".join(diffs))
            return False
        else:
            return True
