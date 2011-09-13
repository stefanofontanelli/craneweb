
craneweb - modern micro web framework for C
===========================================

	    _ 
	   /.)    modern
	  /)\|    micro
	 //)/     web
	/'&&      framework
	  ||      for
	  ||      C
	  """
	          


(C) 2011 Francesco Romani < fromani | gmail : com >

Overview
--------

craneweb is an up-to-date micro web framework for C, which tries to reproduce
the look and feel of the modern web frameworks. craneweb is licensed under the
terms of the ZLIB license.

craneweb tries heavy inspiration from the following frameworks:

* [Raphters](https://github.com/DanielWaterworth/Raphters)
* [bottle](https://github.com/defnull/bottle)


Usage scenario
--------------

Sometimes it is useful to embed a web interface into a daemon/server, for
debug, inspection of the state or for emergency procedures.
Craneweb aims to provide an easy-to-embed, easy-to-use, lightweight solution
for this specific task.
For this very reson, crane is a micro framework, not a full-stack one.

The data-layer component (e.g. ORM) is intentionally out of the scope
of the project: the state which the web interface has to expose is already
present inside the application itself, and the binding is necessarily
very application-dependent.

The template component itself, altough present and recommended,
is enterely optional.


Project directions
------------------

Given the intended usage scenario, the project directions which drive
the development of craneweb are:

* ease of embeddability.
* compactness: the bloat should be trimmed as most as is possible.
* zero-external dependencies: any external dependency is bundled with craneweb.
  Of course all licenses are compatible (and BSD-like). 
* simplicity: the code should be easy to read, easy to maintain and
  easy to keep in mind.


Requirements/Dependencies (common)
----------------------------------

* C89 compiler (tested against gcc 4+).
* [cmake](http://www.cmake.org)


Bundled dependencies (software craneweb relies on)
------------------------------------------------

* [http-parser](https://github.com/ry/http-parser)
* [ngtemplate](https://github.com/breckinloggins/ngtemplate)
* [Spencer's regex](http://www.arglist.com/regex)
* [mongoose](http://code.google.com/p/mongoose/)


Getting Started
---------------

TO WRITE


More Documentation
------------------

TO WRITE

### EOF ###

