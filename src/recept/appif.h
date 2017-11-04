#include "typedef.h"

struct termios oldtio,newtio;


int ntramas_d, ntramas_r;

void timeout_handler(int sig);

/**
* Estabelecer ligacao de dados por verificacao de trama do protocolo
*/
int llopen (int fd);


/**
*  Recepcao de dados do cliente, apos estabelecimento de ligacao.
*  Deve detectar erros de recepcao, enviando informacao
*  relativa a trama que se esperava ter sido recebida (ao cliente).
*/
int llread (int fd, char* buf);

//lwrite so e usada pelo cliente
int llwrite (int fd, char* buf, int lenght);

/**
*  Terminacao do protocolo de ligacao apos correcta recepcao de dados.
*  Do mesmo modo que llopen(), recebe uma trama de terminacao, e envia
*  confirmacao ao cliente.
*/
int llclose (int fd);


