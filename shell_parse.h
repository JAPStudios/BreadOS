#ifndef SHELL_PARSE_H
#define SHELL_PARSE_H
/* Returns argument count; -1 invalid quoting/escape, -2 too many args,
   -3 insufficient output storage. Input is never modified. */
int shell_parse(const char *line,char *storage,unsigned int capacity,
                char **argv,int max_args);
#endif
