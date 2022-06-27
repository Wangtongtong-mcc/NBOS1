#ifndef _KERNEL_STR_H
#define _KERNEL_STR_H

void strcopy(char * source, char * dest, int size);

int strsize(char *string);

void strcuttail(char *string, char *separator);
int findlastsprt(char * string, char *separator);

void strcuthead(char *string, char *separator);
int findfirstsprt(char * string, char *separator);

void strcut(char *string, char *separator, char *result);

int strmatch(char *source,char *dest);

int strcount(char *string, char a);

int strcmpa(char *string1, char *string2, int size);

#endif