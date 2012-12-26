/*
 * $Id: mlog.c,v 1.16 2002/12/31 18:44:02 eaquer Exp $
 * 
 * Copyright (c) 2002 Maciej Korzen <maciek@korzen.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <stdio.h>
#include <curses.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

/*
 * Writes names of all files.
 */
void            all(const u_int max_current, char **logfile);

/*
 * Highlights name of given file.
 */
void            choose(const u_int numer, char **logfile);

/*
 * Modifies value of `current' and checks if final value is in range between
 * 0 and `max_current'.
 */
void            curmod(const int change, int *current, const u_int max_current);

/*
 * Modified version of fgets function, uses int descriptors.
 */
char           *my_fgets(char *buf, size_t len, const int fp);

/*
 * Checks if given text contains only allowed chars.
 */
int             validate(const char text[]);

/*
 * Structure to save buffer and it's size.
 */
struct mybuf {
	char           *val;
	size_t          size;
};

int
main()
{
	int             fd, key;
	char            c, bufor[500];
	u_int           i;

	/*
	 * Program version.
	 */
	static float    ver = 0.7;

	/*
	 * Maximal number of logs to show.
	 */
	u_int           max_logfiles;

	/*
	 * Maximal number of chars in name of file.
	 */
	size_t          max_filename;

	/*
	 * To save full path to .mlogrc.
	 */
	struct mybuf    rcpath;

	/*
	 * Which file should be selected/highlighted.
	 */
	int             current = 0;

	/*
	 * Maximal value for `current'.
	 */
	u_int           max_current = 0;

	/*
	 * How long to wait befor next attempt of reading file's content.
	 */
	u_int           interv = 100000;

	/*
	 * To save error message.
	 */
	struct mybuf    message;

	/*
	 * Table with list of files.
	 */
	char          **logfile;

	/*
	 * Terminal height.
	 */
	u_int           term_height;

	/*
	 * Name of file with list of logs. Relatively to home dir. Slash at
	 * beginning is needed, because HOME variable doesn't have it at the
	 * end.
	 */
	char           *mlogrc = "/.mlogrc";

	/*
	 * Turn on ncurses.
	 */
	(void)initscr();

	/*
	 * Maximum number of chars in file name and number of files depends
	 * on terminal size.
	 */
	getmaxyx(stdscr, max_logfiles, max_filename);

	term_height = max_logfiles - 1;

	/*
	 * We have to leave two lines to display error messages and program
	 * version number.
	 */
	max_logfiles -= 2;

	/*
	 * We have to leave two columns for arrows.
	 */
	max_filename -= 2;

	message.size = max_filename + 3;
	message.val = (char *)malloc(message.size);

	/*
	 * If malloc couldn't allocate memory, then we are going out.
	 */
	if (message.val == NULL) {
		(void)endwin();
		fprintf(stderr, "Can't allocate memory (message.val).\n");
		exit(1);
	}
	message.val[0] = '\0';

	if (getenv("HOME") == NULL) {
		/*
		 * Before showing error message, we have to turn off Ncurses.
		 */
		(void)endwin();
		fprintf(stderr, "HOME not set.\n");
		exit(1);
	}
	/*
	 * rcpath_size will be size of $HOME + size of rcfile + one char for
	 * '\0'.
	 */
	rcpath.size = strlen(getenv("HOME")) + strlen(mlogrc) + 1;

	rcpath.val = malloc(rcpath.size);
	if (rcpath.val == NULL) {
		(void)endwin();
		fprintf(stderr, "Can't allocate memory (rcpath.val).\n");
		exit(1);
	}
	/*
	 * Copy value of HOME to `rcpath'.
	 */
	strncpy(rcpath.val, getenv("HOME"), rcpath.size);
	rcpath.val[rcpath.size] = '\0';

	/*
	 * Check if given path contains only allowed chars.
	 */
	if (validate(rcpath.val) == 1) {
		(void)endwin();
		fprintf(stderr, "HOME contains not allowable chars.\n");
		exit(1);
	}
	/*
	 * To end of `rcpath' we are addind file name from `mlogrc'.
	 */
	strncat(rcpath.val, mlogrc, rcpath.size - strlen(rcpath.val));

	/*
	 * Open config file.
	 */
	fd = open(rcpath.val, O_RDONLY);
	if (fd == -1) {
		(void)endwin();
		fprintf(stderr, "Can't open file ");
		(void)fputs(rcpath.val, stderr);
		fprintf(stderr, ".\n");
		exit(1);
	}
	/*
	 * We won't need `rcpath' anymore.
	 */
	free(rcpath.val);

	logfile = (char **)malloc(max_logfiles * sizeof(char *));

	if (logfile == NULL) {
		(void)endwin();
		fprintf(stderr, "Can't allocate memory (logfile).\n");
		exit(1);
	}
	/*
	 * We are reading list of log files from config file.
	 */
	i = 0;
	while (read(fd, &c, 1) != 0) {
		if (c != '\n') {
			if (max_current == max_logfiles) {
				snprintf(message.val, message.size, "Too many files to show.");
				break;
			}
			if (i < (u_int) max_filename) {
				if (i == 0)
					logfile[max_current] = (char *)malloc(max_filename);

				if (logfile[max_current] == NULL) {
					(void)endwin();
					fprintf(stderr, "Can't allocate memory (logfile[]). %i\n", max_current);
					exit(1);
				}
				/*
				 * If at beginning of entry is space, then
				 * don't increase `i', and skip to next char.
				 */
				if (i == 0 && c == ' ') {
					continue;
				} else {
					logfile[max_current][i] = c;
					i++;
				}
			} else {
				(void)endwin();
				fprintf(stderr, "Line %u: too long filename. Maximally %u chars.\n", max_current + 1, (u_int) max_filename);
				exit(1);
			}
		} else {
			/*
			 * If the line was empty, then free space and skip to
			 * next entry. Don't increase `max_current'.
			 */
			if (i == 0) {
				free(logfile[max_current]);
				continue;
			}
			/*
			 * Remove spaces from end of the filename.
			 */
			for (i = (strlen(logfile[max_current]) - 1); logfile[max_current][i] == ' '; i--) {
				logfile[max_current][i] = '\0';
			}

			i = 0;

			if (validate(logfile[max_current]) == 1) {
				(void)endwin();
				fprintf(stderr, "Line %u: filename contains not allowable chars.\n", max_current + 1);
				exit(1);
			}
			max_current++;
		}
	}

	if (max_current == 0) {
		(void)endwin();
		fprintf(stderr, "No files defined to show.\n");
		exit(1);
	}
	max_current--;

	(void)close(fd);

	/*
	 * Turn on keypad and scrolling of text.
	 */
	(void)keypad(stdscr, TRUE);
	(void)scrollok(stdscr, TRUE);

	for (;;) {
		(void)clear();
		(void)wmove(stdscr, (int)term_height, 0);
		(void)wprintw(stdscr, "Mlog %.1f", ver);
		all(max_current, logfile);

		/*
		 * If `message.val' is not empty, then show message and clear
		 * `message.val'.
		 */
		if (message.val[0] != '\0') {
			(void)wmove(stdscr, 0, 0);
			/*
			 * Turn on reverse.
			 */
			(void)attron(A_REVERSE);
			(void)wprintw(stdscr, message.val);
			/*
			 * Turn off reverse.
			 */
			attrset(A_NORMAL);
			message.val[0] = '\0';
		}
		choose((u_int) current, logfile);

		/*
		 * Check which key was pressed.
		 */
		key = wgetch(stdscr);
		if (key == KEY_DOWN) {
			curmod(1, &current, max_current);
		} else if (key == KEY_UP) {
			curmod(-1, &current, max_current);
		} else if (key == KEY_END) {
			current = (int)max_current;
		} else if (key == KEY_HOME) {
			current = 0;
		} else if (key == 10 || key == 32) {
			/*
			 * If Enter or Space was pressed.
			 */
			/*
			 * nodelay - non-blocking call. Reading a key will
			 * success only if a key will be already in bufor.
			 */
			(void)nodelay(stdscr, TRUE);
			(void)clear();
			(void)refresh();

			fd = open(logfile[current], O_RDONLY);
			if (fd >= 0) {
				/*
				 * If Enter was pressed, go 9k chars before end
				 * of file, else go at end of file.
				 */
				if ( key == 10 ) {
					if (lseek(fd, -9000, SEEK_END) < 0) {
						/*
						 * If file is smaller than 9k chars,
						 * then go at the beginning of file.
						 */
						(void)lseek(fd, 0, SEEK_SET);
					}
				} else {
					lseek(fd, 0, SEEK_END);
				}
				while (1) {
					key = wgetch(stdscr);
					if ((char)key == 'q') {
						break;
					}
					if ((my_fgets(bufor, sizeof(bufor), fd)) == NULL) {
						/*
						 * If we are at the end of
						 * file, update screen and
						 * wait for a short time.
						 */
						(void)doupdate();
						(void)usleep(interv);
					} else {
						/*
						 * If we aren't at the end,
						 * write file content.
						 */
						printf("%s\r", bufor);
						(void)wnoutrefresh(stdscr);
					}
				}
				(void)close(fd);
				(void)nodelay(stdscr, FALSE);
			} else {
				snprintf(message.val, message.size, "Can't open file %s", logfile[current]);
				(void)nodelay(stdscr, FALSE);
			}
		} else {
			/*
			 * To check if the pressed key was [Q], we have to
			 * turn off keypad...
			 */
			(void)ungetch(key);
			(void)keypad(stdscr, FALSE);
			key = wgetch(stdscr);
			if ((char)key == 'q') {
				(void)clear();
				(void)refresh();
				(void)endwin();
				exit(0);
			}
			(void)keypad(stdscr, TRUE);
		}
	}

}

void
all(const u_int max_current, char **logfile)
{
	u_int           current;

	for (current = 0; current <= max_current; current++) {
		(void)wmove(stdscr, current + 1, 1);
		(void)wprintw(stdscr, "%s", logfile[current]);
	}
	(void)refresh();
}

void
choose(const u_int numer, char **logfile)
{
	(void)attron(A_BOLD);
	(void)wmove(stdscr, (int)numer + 1, 0);
	(void)wprintw(stdscr, ">%s<", logfile[numer]);
	attrset(A_NORMAL);
	(void)refresh();
}

void
curmod(const int change, int *current, const u_int max_current)
{
	*current += change;
	if (*current > (int)max_current) {
		*current = 0;
	} else if (*current < 0) {
		*current = (int)max_current;
	}
}

char           *
my_fgets(char *buf, size_t len, const int fp)
{
	char           *cp = buf;
	char            c;

	/*
	 * Copy until the buffer fills up, until EOF, or until a newline is
	 * found.
	 */
	while (len > 1 && (read(fp, &c, 1) > 0)) {
		len--;
		*cp++ = c;
		if (c == '\n')
			break;
	}

	/*
	 * Return 0 if nothing was read. This is correct even when a silly
	 * buffer length was specified.
	 */
	if (cp > buf) {
		*cp = 0;
		return (buf);
	} else {
		return (0);
	}
}

int
validate(const char text[])
{
	u_int           i;

	static char     allowed_char[] =
	{
		' ', '#', '&', '~', '+', ',', '-', '.', '/', '0', '1', '2',
		'3', '4', '5', '6', '7', '8', '9', ':', '?', '@', 'A', 'B',
		'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N',
		'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
		'[', ']', '_', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i',
		'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u',
		'v', 'w', 'x', 'y', 'z', '{', '}', '\0'
	};

	for (i = 0; text[i] != '\0'; i++) {
		if (strchr(allowed_char, text[i]) == NULL) {
			return 1;
		}
	}
	return 0;
}
