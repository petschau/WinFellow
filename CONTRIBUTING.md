Contributing to WinFellow
=========================
WinFellow is developed by a team of volunteers in their spare time; you
are very welcome to make contributions of your own.
Contributions can be improvements to the code, new features, but can also
include documentation updates or whatever else you feel could be an improvement.
It is best if you find something you want to work on rather than mailing
us and ask to be assigned a task.
Since WinFellow is developed on an idealistic basis, the basic idea is
that everyone should do something they like.

When you start doing something it is wise to mail some of us in advance
to make sure no one is doing double work.
Since WinFellow is developed as open source software hosted on GitHub,
it is easy to contribute.
You can fork the source code of WinFellow, create a branch for your topic
and start commiting changes to it; your changes can be submitted to our
project in form of a pull request.
Raising a pull request early on allows us to discuss your request early,
so that we can see if/how your request may fit into WinFellow before you
put any effort into it.

There is a how-to guide in MarkDown syntax within the source code archive
in the directory "fellow\doxygen\Documentation" called
"HOWTO Development environment setup.md", that can help you get started
setting up a development environment for WinFellow.

Feel free to mail us
([Petter Schau](mailto:petschau@gmail.com) / [Torsten Enderling](mailto:carfesh@gmx.net))
if you are interested in helping!

Coding Style Guidelines
-----------------------
To ensure consistency across the different modules that are developed by different authors, we
believe it is reasonable to have a common set of guidelines all developers follow.

- For optimum layout, configure the tab width to 8, and enable the replacing of tabs with spaces.
- Configure the indentations to 2.
- No line should be longer than 200 characters.
- Prefer the use of built-in C++ datatypes for new code (bool instead of BOOLE).
- New modules can be created in C++.
- Instead of one-line statements, the use of the curly brackets {} is preferred, but not mandatory.