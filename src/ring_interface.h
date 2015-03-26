#ifndef SRC_RING_INTERFACE_H_
#define SRC_RING_INTERFACE_H_

int handleMessage(const char* message, int fd);

int distance(int nodeK, int nodeL);

int handleQRY(int searcherId, int searchedId, int *ownerId, char *ownerIp, char *ownerPort);

int rebuild();

#endif /* SRC_RING_INTERFACE_H_ */
