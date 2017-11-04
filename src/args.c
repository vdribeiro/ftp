#include "args.h"

//WILDCARDS


int divide(const char *str, const char *delimiters, char ***retstr) {
	int error, i, tokens;
	const char *new;
	char *t;

	if ((str == NULL) || (delimiters == NULL) || (retstr == NULL)) 
	{
		return -1;
	}
	
	*retstr = NULL;
	
	//aqui tiramos os primeiros delimitadores para sabermos onde comeca mesmo o texto
	new = str + strspn(str, delimiters);
	if ((t = (char *) malloc(strlen(new) + 1)) == NULL)
		return -1;
	strcpy(t, new);               
	tokens = 0;

	//contamos o numero de tokens em str
	if (strtok(t, delimiters) != NULL)
		for (tokens = 1; strtok(NULL, delimiters) != NULL; tokens++);
		
	//criar array de dados
	if ((*retstr = (char **)malloc((tokens + 1)*sizeof(char *))) == NULL) 
	{
		error = errno;
		free(t);
		errno = error;
		return -1; 
	} 
	
	//inserir no array
	if (tokens == 0)
		free(t);
	else 
	{
		strcpy(t, new);
		**retstr = strtok(t, delimiters);
		for (i = 1; i < tokens; i++)
			*((*retstr) + i) = strtok(NULL, delimiters);
	}
	
	//fechar com o NULL no fim
	*((*retstr) + tokens) = NULL;
	return tokens;
}     

void freedivide(char **str) {
   if (str == NULL)
      return;
   if (*str != NULL)
      free(*str);
   free(str);
}

