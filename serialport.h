#ifndef SERIALPORT_H
#define SERIALPORT_H

int open_port(const char* device);
int close_port(int fd);
int setup_port(int fd, int speed, int data_bits, int parity, int stop_bits);

#endif /* SERIALPORT_H */