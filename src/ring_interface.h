#ifndef SRC_RING_INTERFACE_H_
#define SRC_RING_INTERFACE_H_

int handleMessage(const char* message, int fd);

int distance(int nodeK, int nodeL);

int executeQRY(int searcherId, int searchedId, int *ownerId, char *ownerIp, char *ownerPort);

#endif /* SRC_RING_INTERFACE_H_ */
