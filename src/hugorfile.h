#pragma once
#include <cstdio>

struct HugorFile final
{
public:
    HugorFile(FILE* handle)
        : fHandle(handle)
    { }

    ~HugorFile()
    {
        close();
    }

    HugorFile(const HugorFile&) = delete;

    FILE* get() const
    {
        return fHandle;
    }

    int close()
    {
        if (fHandle == nullptr) {
            return 0;
        }
        auto ret = std::fclose(fHandle);
        fHandle = nullptr;
        return ret;
    }

private:
    FILE* fHandle;
};
