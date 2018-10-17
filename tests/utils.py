import difflib


def file_compare(fn1, fn2):
    """
    Compare two files and raise an Assertion error if files are different.

    The assertion error contains a diff of the files.
    """
    with open(fn1) as f, open(fn2) as g:
        flines = f.readlines()
        glines = g.readlines()
        d = difflib.Differ()
        diffs = [x for x in d.compare(flines, glines) if x[0] in ('+', '-')]
        if diffs:
            # all rows with changes
            raise AssertionError("Files not equal\n"+"".join(diffs))
            return False
        else:
            return True
