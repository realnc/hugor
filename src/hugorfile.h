#pragma once
#include <cstdio>

struct HugorFile final
{
public:
    HugorFile(FILE* handle)
        : handle_(handle)
    {}

    ~HugorFile()
    {
        close();
    }

    HugorFile(const HugorFile&) = delete;

    FILE* get() const
    {
        return handle_;
    }

    int close()
    {
        if (handle_ == nullptr) {
            return 0;
        }
        auto ret = std::fclose(handle_);
        handle_ = nullptr;
        return ret;
    }

private:
    FILE* handle_;
};
