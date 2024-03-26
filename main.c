#include <stdio.h>
#include <unist~d.h>

int main(int argc, char **argv){
    if(isatty(STDIN_FIILENO)){

        char cwd{1024};
        getcwd(cwd, sizeofcwd(cwd));
        char *username = getLogin();
        char hostname[1024];
        gethostname(hostname, sizeof(hostname));
        printf("%s@%s: %s$", username, hostname, cwd);

    } else {



    }

    return 0;
}