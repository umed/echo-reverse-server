#pragma once

namespace echo_reverse_server::utils {

void SetNonBlocking(int fd);
bool WouldBlock();

struct FileDescriptorHolder {
    constexpr static int INVALID_FILE_DESCRIPTOR = -1;

    FileDescriptorHolder();
    FileDescriptorHolder(const FileDescriptorHolder&) = delete;
    FileDescriptorHolder(int fd);

    FileDescriptorHolder(int& fd);

    FileDescriptorHolder(FileDescriptorHolder&& other);

    bool IsValid();

    int Release();

    ~FileDescriptorHolder();

    FileDescriptorHolder& operator=(int fd);

    operator int() const;

    int ToInt() const;

private:
    int fd;
};

} // namespace echo_reverse_server::utils
