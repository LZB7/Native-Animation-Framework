#!/usr/bin/env python3
import argparse

from pyxdia import PDB


parser = argparse.ArgumentParser()
parser.add_argument("pdb")
parser.add_argument("terms", nargs="+")
args = parser.parse_args()

terms = tuple(term.casefold() for term in args.terms)
pdb = PDB(args.pdb)
for symbol in pdb.globals or pdb.publics or ():
    name = symbol.get("undecoratedName") or symbol.get("name") or ""
    if any(term in name.casefold() for term in terms):
        print(f"{symbol.get('relativeVirtualAddress', 0):#x} {symbol.get('symTag')} {name}")
