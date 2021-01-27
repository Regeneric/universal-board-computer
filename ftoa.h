#ifndef FTOA_H
#define FTOA_H

struct ftoaInterface {
    void (*convert)(float, char*, int);
}; extern const struct ftoaInterface FTOA;

#endif  // FTOA_H