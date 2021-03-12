ftmap 1.8.2 readme
==================

Certain resources in the ftmap init file no longer
have default values if they are undefined these are:

	foregroundColor
	leaderColor
	locusColor
	
They can be commented out in the init file by using the
comment character ';' semi-colon or omitted all together.

If a text label has a null value then the label will not
be plotted. A null value is a line with nothing on it but
a carriage return.