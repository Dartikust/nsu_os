#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct node {
    char *text;
    struct node *next;
};

int main(void)
{
    struct node *head = NULL, *tail = NULL, *current;
    char buffer[4096];
    int status = 0;

    puts("Enter lines; a dot at the beginning finishes input:");
    while (fgets(buffer, sizeof(buffer), stdin) != NULL) {
        size_t length;
        if (buffer[0] == '.')
            break;
        length = strlen(buffer);
        if (length == sizeof(buffer) - 1 && buffer[length - 1] != '\n') {
            fprintf(stderr, "Line must fit into the 4096-byte buffer\n");
            status = 1;
            break;
        }
        current = malloc(sizeof(*current));
        if (current == NULL) {
            perror("malloc");
            status = 1;
            break;
        }
        current->text = malloc(length + 1);
        if (current->text == NULL) {
            perror("malloc");
            free(current);
            status = 1;
            break;
        }
        strcpy(current->text, buffer);
        current->next = NULL;
        if (head == NULL)
            head = current;
        else
            tail->next = current;
        tail = current;
    }
    if (ferror(stdin)) {
        perror("fgets");
        status = 1;
    }
    puts("Saved lines:");
    for (current = head; current != NULL; current = current->next)
        fputs(current->text, stdout);
    while (head != NULL) {
        current = head->next;
        free(head->text);
        free(head);
        head = current;
    }
    return status;
}
