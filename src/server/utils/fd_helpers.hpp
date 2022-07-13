#pragma once

namespace echo_reverse_server::utils {

void SetNonBlocking(int fd);
bool WouldBlock();

bool IsValid(int fd);

void Close(int fd);

} // namespace echo_reverse_server::utils
