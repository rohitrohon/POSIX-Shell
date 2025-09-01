#ifndef REDIR_H
#define REDIR_H

#include <string>

// Function to set up input redirection
bool setup_input_redirection(const std::string& infile);

// Function to set up output redirection
bool setup_output_redirection(const std::string& outfile, bool append);

// Function to create and set up a pipe
bool setup_pipe(int pipefd[2]);

// Function to close pipe ends
void close_pipe_ends(int read_fd, int write_fd);

#endif // REDIR_H
