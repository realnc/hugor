#ifndef HUGORFILE_H
#define HUGORFILE_H

#include <cstdio>

class HugorFile
{
public:
    HugorFile(FILE* handle)
        : fHandle(handle)
    { }

    ~HugorFile()
    {
        close();
    }

    FILE* get() const
    {
        return fHandle;
    }

    int close()
    {
        if (fHandle) {
            auto ret = std::fclose(fHandle);
            fHandle = nullptr;
            return ret;
        }
        return 0;
    }

private:
    FILE* fHandle;
};

#endif // HUGORFILE_H
