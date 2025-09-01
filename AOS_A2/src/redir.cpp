#include "redir.h"
#include <fcntl.h>
#include <unistd.h>
#include <iostream>
#include <sys/stat.h>
#include <errno.h>

bool setup_input_redirection(const std::string& infile) {
    if (infile.empty()) {
        return true;  // No redirection needed
    }

    int fd = open(infile.c_str(), O_RDONLY);
    if (fd < 0) {
        std::cerr << "Error: Cannot open input file '" << infile << "': " 
                  << strerror(errno) << std::endl;
        return false;
    }

    if (dup2(fd, STDIN_FILENO) < 0) {
        std::cerr << "Error: Failed to redirect input: " << strerror(errno) << std::endl;
        close(fd);
        return false;
    }

    close(fd);
    return true;
}

bool setup_output_redirection(const std::string& outfile, bool append) {
    if (outfile.empty()) {
        return true;  // No redirection needed
    }

    int flags = O_WRONLY | O_CREAT;
    flags |= (append ? O_APPEND : O_TRUNC);

    int fd = open(outfile.c_str(), flags, 0644);
    if (fd < 0) {
        std::cerr << "Error: Cannot open output file '" << outfile << "': " 
                  << strerror(errno) << std::endl;
        return false;
    }

    if (dup2(fd, STDOUT_FILENO) < 0) {
        std::cerr << "Error: Failed to redirect output: " << strerror(errno) << std::endl;
        close(fd);
        return false;
    }

    close(fd);
    return true;
}

bool setup_pipe(int pipefd[2]) {
    if (pipe(pipefd) < 0) {
        std::cerr << "Error: Failed to create pipe: " << strerror(errno) << std::endl;
        return false;
    }
    return true;
}

void close_pipe_ends(int read_fd, int write_fd) {
    if (read_fd >= 0) close(read_fd);
    if (write_fd >= 0) close(write_fd);
}
