
craneweb - design decisions rationale
======================================

This is a memorandum for the basic design decisions of craneweb.


Implementation Language
-----------------------

Alternatives:

* C
* C++

Discussion:

C++:
[+] more expressive (native OOP support).
[+] much richer standard template library (no reimplementatio needed).
[-] add dependency WRT C: compiler, libstdc++.

C:
[+] lean and mean (lean enough, too mean).
[+] minimal dependency set.
[-] need to reimplement basic stuff (hash, lists & co).
[+] external dependencies provide most of the infrastructure (no direct remimplementation).

Decision:
The reference implementation will be in C.
However, an idiomatic (template, std::map & co) implementation will be provided
ASAP after the reference implementation is completed.


Hosting provider
----------------

Alternatives:

* github
* bitbucket

Discussion:

bitbucket:
[+] I like mercurial perhaps even more than git.
[+] no space constraints.

github:
[+] space does not seem an issue at all, so far.
[+] all the inspiration sources and most of the dependencies reside on github.

Decision:
github.

### EOF ###

