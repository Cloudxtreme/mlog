
	Mlog by Maciej Korzen <maciek@korzen.org>
	http://www.korzen.org/soft/mlog/

Mlog can be used to view system logs. You can define list of log files to watch
and easily move between them. Program needs ncurses library to work.

Curses development files have to be installed on your system.

To compile program just run 'make'. To install it - 'make install'. Mlog will be
installed in /usr/local/bin.

In ~/.mlogrc put list of log files, You would like to view. Sample .mlogrc file
is delivered with source code.

When You run program, after highlighting log file name, press [Enter] to view
it's content. You will see last 9000 chars of file, or less if it is smaller.
When You press [Space] (instead of [Enter]), mlog will start reading from end of
file. You can go back to main screen pressing [Q]. To close the program, press
[Q] in main screen.

Mlog works fine on FreeBSD and Linux.

-- 
Any bug fixes are welcome.
