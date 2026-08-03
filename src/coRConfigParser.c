#include "coRConfigParser.h"
#include "src/coROutput.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CONFIG_FILE "tmpConfig.conf"

void runConfig(struct coR_state *coRState) {
  // Open the main config file
  FILE *file = fopen(CONFIG_FILE, "r");
  if (file == NULL)
    return;

  // Each line
  char buffer[71];
  while (fgets(buffer, 71, file) != NULL) {
    if (strlen(buffer) < 3)
      continue;

    // First word
    char *splittedStr = strtok(buffer, " ");
    if (splittedStr == NULL)
      continue;
    printf("%s\n", splittedStr);

    // Monitor config
    if (strcmp(splittedStr, "monitor") == 0) {
      // Create the structure
      struct coR_output *coROutput = calloc(1, sizeof(struct coR_output));
      if (coROutput == NULL) {
        continue;
      }
      wl_list_insert(&coRState->outputs, &coROutput->link);
      coROutput->output = NULL;

      // Fill the structure
      int i = 0;
      splittedStr = strtok(NULL, " ");
      while (splittedStr != NULL && i < 3) {
        printf("%s\n", splittedStr);
        if (strcmp(splittedStr, "->") == 0) {
          splittedStr = strtok(NULL, " ");
          continue;
        }

        if (i == 0) {
          coROutput->name = strdup(splittedStr);
        } else if (i == 1) {
          coROutput->posX = atoi(splittedStr);
        } else if (i == 2) {
          coROutput->posY = atoi(splittedStr);
        }

        splittedStr = strtok(NULL, " ");
        i++;
      }
    }

    // Auto Start application / command (No arguments yet)
    // TODO: command with arguments
    else if (strcmp(splittedStr, "autoStart") == 0) {
      splittedStr = strtok(NULL, " ");
      if (splittedStr == NULL)
        continue;

      for (int i = 0; i < strlen(splittedStr); i++)
        if (splittedStr[i] == '\n')
          splittedStr[i] = '\0';

      printf("%s\n", splittedStr);
      if (fork() == 0) {
        execlp(splittedStr, splittedStr, NULL);
        exit(1);
      }
    }
  }
  fclose(file);
}

