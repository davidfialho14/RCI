#ifndef CONNECTION_SET_H
#define CONNECTION_SET_H

/*
 * descricao:	utilizada para iniciar o conjunto de ligacoes
 */
void initializeConnectionSet();


/*
 * descricao: 	retorna o descritor da primeira ligacao.
 * erros:		o valor -1 é retornado caso nao exista nenhum fd no conjunto
 */
int getFirstConnection();

/*
 * descricao:	retorna a proxima ligacao no conjunto.
 * erros:		o valor -1 é retornado caso nao exista mais nenhum fd no conjunto
 */
int getNextConnection();

/*
 * descricao:	adiciona uma ligacao ao conjunto. nao é feita qualquer verificao em relacao a
 * 				existencia da ligacao no conjunto
 */
void addConnection(int fd);

/*
 * 	descricao:	remove uma ligacao do conjunto
 */
void rmConnection(int fd);

/*
 * 	descricao: 	retorna o descritor mais elevado de todas as ligacoes no conjunto
 */
int getMaxConnection();

/*
 * 	descricao:	permite copiar o conjunto de ligacoes para outro fd_set externo
 */
void copySet(fd_set *destinationSet);


#endif
