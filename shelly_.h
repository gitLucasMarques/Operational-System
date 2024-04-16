#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <pwd.h>


#define MAX_PATH_SIZE 1024
#define MAX_CMD_SIZE 1024
#define MAX_CMDS 50
#define TRUE 1
#define PIPE 1
#define OR 2
#define AND 3
#define AMPERSAND 4

// struct auxiliar para gerenciar a ordem de operaçoes e a posicao dos comands no vetor de argumentos
typedef struct {
  int index; // Variavel auxiliar, usada como indice nos vetores abaixo
  int operandOrder[MAX_CMDS]; // Armazena os operandos na ordem que aparecem no comando
  int start[MAX_CMDS]; // Armazena o indice em que cada comando começa no vetor de argumentos
  int res; // Flag utilizada em _process_cmd
} operand_counter;

char cmd[MAX_CMD_SIZE], dummy[MAX_CMD_SIZE];



void _print_cmdline(struct passwd *);
void _read_cmd(int *, char **, operand_counter *);
void _process_cmd(int *, char **, operand_counter *);
int _tokenize_cmd(int, char **, char *, operand_counter *);
int _analyze_cmd(int *, char **);
int _is_complete(char *);
int _ends_with_e(int *, char **);
int _run_cmd(int *, char **, int, int, operand_counter *);
int _pipe_cmd(int *, char **, int, int);
int _run_n_pipes(int *, char **, int, int, operand_counter *);

// Imprime a linha para a entrada de comandos
void _print_cmdline(struct passwd *p) {
  char path[MAX_PATH_SIZE];
  char *ptr = getcwd(path, sizeof(path));
  printf("\033[0;35m");
  printf("%s@%s", p->pw_name, path);
  printf("\033[0m");
  printf("~₢ ");
}

// Lê o comando digitado e tokeniza utilizando a subrotina _tokenize_cmd()
void _read_cmd(int *argc, char **argv, operand_counter *data) {
  fgets(cmd, sizeof(cmd), stdin);
  cmd[strlen(cmd) - 1] = '\0';

  // Subrotina para checar se o comando esta completo, fica presa nela ate o
  // comando estar completo
  char aux[MAX_CMD_SIZE];
  strcpy(aux, cmd);
  while (!_is_complete(aux)) {
    printf("> ");
    fgets(dummy, sizeof(dummy), stdin);
    dummy[strlen(dummy) - 1] = '\0';
    strcat(cmd, " ");
    strcat(cmd, dummy);
    strcpy(aux, cmd);
  }

  *argc = _tokenize_cmd(0, argv, cmd, data);
};

// Tokeniza o comando, separando a string de entrada nos espaços e retorna o
// número de comandos
int _tokenize_cmd(int i, char **argv, char *cmd, operand_counter *data) {
  for (int j = 0; j < MAX_CMDS; j++)
    data->operandOrder[j] = 0;
  
  data->index = 0;
  data->start[0] = 0;

  char *token = strtok(cmd, " ");
  while (token != NULL) {
    if (!strcmp(token, "|")) {
      data->operandOrder[data->index] = PIPE;
      data->start[data->index + 1] = i + 1;
      argv[i] = NULL;
      data->index++;
    } else if (!strcmp(token, "||")) {
      data->operandOrder[data->index] = OR;
      data->start[data->index + 1] = i + 1;
      argv[i] = NULL;
      data->index++;
    } else if (!strcmp(token, "&&")) {
      data->operandOrder[data->index] = AND;
      data->start[data->index + 1] = i + 1;
      argv[i] = NULL;
      data->index++;
    } else if (!strcmp(token, "&")) {
      data->operandOrder[data->index] = AMPERSAND;
      data->start[data->index + 1] = i + 1;
      argv[i] = NULL;
      data->index++;
    } else {
      argv[i] = token;
    }

    token = strtok(NULL, " ");
    i++;
  }

  argv[i] = NULL;
  return i;

  //Exemplo:
  //Para o comando "ls -l | wc || mkdir dir"
  //O vetor de saida sera: argv = ["ls", "-l", NULL, "wc", NULL, "mkdir", "dir", NULL]
}

// Processa o comando de entrada
void _process_cmd(int *argc, char **argv, operand_counter *data) {
  data->res = 0; // flag que indica se o ultimo comando executou corretamente
  for (int i = 0; i <= data->index; i++) {
    if (data->operandOrder[i] == PIPE) {
      // se houver um operador PIPE roda todos os comandos conectados por PIPE
      int pipeEnd = i + 1;
      while (data->operandOrder[pipeEnd] == PIPE)
        pipeEnd++;
      int nPipe = pipeEnd - i;

      data->res = _run_n_pipes(argc, argv, nPipe+1, i, data);
      i += pipeEnd;
      continue;
    } else if (0 < i && data->operandOrder[i - 1] == AND) {
      // Comando AND roda somente se o anterior tiver executado com sucesso
      if (!data->res)
        data->res = _run_cmd(argc, argv, data->start[i], i, data);
      continue;
    } else if (0 < i) {
      // Comando OR roda se o anterior tiver executado sem sucesso
      if (data->res)
        data->res = _run_cmd(argc, argv, data->start[i], i, data);
      continue;
    } else {
      // Apenas um comando foi passado para Shelly
      data->res = _run_cmd(argc, argv, data->start[0], i, data);
    }
  }
}

// Retorna 1 se o comando está completo, ou seja, não termina com | ou &&
// rotina auxiliar de _tokenize_cmd
int _is_complete(char *aux) {
  char *tester;
  char *token = strtok(aux, " ");

  while (token != NULL) {
    tester = token;
    token = strtok(NULL, " ");
  }

  if (!strcmp(tester, "|") || !strcmp(tester, "||") || !strcmp(tester, "&&"))
    return 0;

  return 1;
}

// Executa um comando simples passado para Shelly
int _run_cmd(int *argc, char **argv, int start, int i, operand_counter *data) {
  if (!strcmp(argv[start], "cd")) {
    if (argv[start + 1] != NULL && argv[start + 2] == NULL) {
      int s = chdir(argv[start + 1]);
      if (s < 0) {
        printf("caminho nao reconhecido\n");
        return 1;
      }
      return 0;
    } else if (argv[start + 1] == NULL) {
      chdir("/");
      return 0;
    } else {
      printf("bash: muitos comandos\n");
      return 1;
    }
  }

  pid_t kid = fork();
  int status;

  if (kid == 0) {
    int success = execvp(argv[0 + start], argv + start);

    if (success == -1)
      printf("bash: comando nao reconhecido\n");
    exit(success);
  } else if (data->operandOrder[data->index - 1] != AMPERSAND) {
    waitpid(-1, &status, 0);
    return status;
  }

  return status;
}

// Roda n pipes alocando-os dinamicamente e executando todos em seguida
int _run_n_pipes(int *argc, char **argv, int nPipes, int pipeStart, operand_counter *data) {
  int **fd;
  pid_t *pid;

  pid = (pid_t *)malloc(sizeof(pid_t) * nPipes);
  fd = (int **)malloc(sizeof(int *) * (nPipes - 1));
  
  for (int j = 0; j < (nPipes - 1); j++)
    fd[j] = (int *)malloc(sizeof(int) * 2);

  for (int j = 0; j < (nPipes - 1); j++)
    pipe(fd[j]);

  for (int j = 0; j < nPipes; j++) {
    pid[j] = fork();
    if (pid[j] == 0) {
      if (j > 0) {
        dup2(fd[j - 1][0], STDIN_FILENO);
      }
      if (j != nPipes - 1) {
        dup2(fd[j][1], STDOUT_FILENO);
      }
      for (int i = 0; i < (nPipes - 1); i++) {
        close(fd[i][0]);
        close(fd[i][1]);
      }
      break;
    }
  }

  for (int j = 0; j < nPipes; j++){
    if (pid[j] == 0) {
      int index = data->start[pipeStart + j];
      execvp(argv[index], argv+index);
    }
  }  

  for (int i = 0; i < nPipes; i++) {
    if(i != nPipes-1){
      close(fd[i][0]);
      close(fd[i][1]);
    }
    waitpid(pid[i], NULL, 0);
  }

  return 0;
}