#include "coRConfigParser.h"
#include "src/coROutput.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wayland-util.h>
#include <xkbcommon/xkbcommon.h>

#define CONFIG_FILE "tmpConfig.conf"

void skipUselessWord(char **splittedStr) {
  if (splittedStr == NULL)
    return;

  if (*splittedStr == NULL)
    return;

  while (strcmp(*splittedStr, "->") == 0 || strcmp(*splittedStr, "<-") == 0 ||
         strcmp(*splittedStr, "|") == 0) {
    *splittedStr = strtok(NULL, " ");
    continue;
  }
}

void runConfig(struct coR_state *coRState) {
  // Open the main config file
  FILE *file = fopen(CONFIG_FILE, "r");
  if (file == NULL)
    return;

  // Each line
  char buffer[171];
  while (fgets(buffer, sizeof(buffer), file) != NULL) {
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
        skipUselessWord(&splittedStr);
        printf("%s\n", splittedStr);

        if (i == 0) {
          coROutput->name = strdup(splittedStr);
        } else if (i == 1) {
          coROutput->posX = atoi(splittedStr);
          printf("detected posX: %d\n", coROutput->posX);
        } else if (i == 2) {
          coROutput->posY = atoi(splittedStr);
          printf("detected posY: %d\n", coROutput->posY);
        }

        splittedStr = strtok(NULL, " ");
        i++;
      }
    }

    // Auto Start application / command / compositor action
    // TODO: command with arguments
    else if (strcmp(splittedStr, "autoStart") == 0) {
      splittedStr = strtok(NULL, " ");
      skipUselessWord(&splittedStr);
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

    // Start application / command with keys
    else if (strcmp(splittedStr, "key") == 0) {
      uint8_t nbElem = 0;
      char *elemsArray[17];

      // ----- KEYS -----
      // Count how much key
      splittedStr = strtok(NULL, " ");
      skipUselessWord(&splittedStr);
      while (splittedStr != NULL && strcmp(splittedStr, "command") != 0 &&
             strcmp(splittedStr, "compositorAction") != 0) {
        if (nbElem < 17) {
          elemsArray[nbElem] = splittedStr;
          nbElem++;
        }

        printf("%10s\n", splittedStr);
        splittedStr = strtok(NULL, " ");
        skipUselessWord(&splittedStr);
      }

      // Structure for the command
      struct keyCommand *command = calloc(1, sizeof(struct keyCommand));
      if (command == NULL)
        continue;

      command->keys = calloc(nbElem, sizeof(int));
      if (command->keys == NULL) {
        free(command);
        continue;
      }

      command->nbKeys = nbElem;
      // Import in the command structure
      int i = 0;
      while (i < nbElem) {

        char *str = elemsArray[i];
        command->keys[i] =
            xkb_keysym_from_name(str, XKB_KEYSYM_CASE_INSENSITIVE);
        printf("%u\n", command->keys[i]);

        i++;
      }

      // ----- COMMAND -----
      uint8_t isCompositorAction = 0;
      if (strcmp(splittedStr, "compositorAction") == 0) {
        isCompositorAction = 1;
      }

      splittedStr = strtok(NULL, " ");
      skipUselessWord(&splittedStr);
      if (splittedStr == NULL) {
        free(command->keys);
        free(command);
        continue;
      }

      if (isCompositorAction == 1) {
        command->command = calloc(2, sizeof(char *));
        command->command[0] = strdup("compositorAction");

        for (int j = 0; j < strlen(splittedStr); j++)
          if (splittedStr[j] == '\n')
            splittedStr[j] = '\0';
        command->command[1] = strdup(splittedStr);
        printf("%50s\n",splittedStr);
        wl_list_insert(&coRState->commands, &command->link);
        continue;
      }

      // Count how much command's elem
      nbElem = 0;
      while (splittedStr != NULL && nbElem < 17) {
        elemsArray[nbElem] = splittedStr;
        nbElem++;

        printf("%10s\n", splittedStr);
        splittedStr = strtok(NULL, " ");
        skipUselessWord(&splittedStr);
      }

      command->command = calloc(nbElem + 1, sizeof(char *));
      command->command[nbElem] = NULL;
      if (command->command == NULL) {
        free(command->keys);
        free(command);
        continue;
      }

      // Import in the command structure
      i = 0;
      while (i < nbElem) {
        char *str = elemsArray[i];

        // Clean it
        for (int j = 0; j < strlen(str); j++)
          if (str[j] == '\n')
            str[j] = '\0';

        // Place into the structure
        command->command[i] = strdup(str);
        printf("%s\n", command->command[i]);
        i++;
      }

      wl_list_insert(&coRState->commands, &command->link);
    }
  }
  fclose(file);
  printf("End config\n");
}
