#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <time.h> // For execution timer

#define ALPHABET_SIZE 26
#define MAX_SUGGESTIONS 5

typedef struct TrieNode {
    struct TrieNode *children[ALPHABET_SIZE];
    bool is_end_of_word;
    int frequency;
} TrieNode;

TrieNode* create_node(void) {
    TrieNode *node = (TrieNode *)malloc(sizeof(TrieNode));
    if (node) {
        node->is_end_of_word = false;
        node->frequency = 0;
        for (int i = 0; i < ALPHABET_SIZE; i++) {
            node->children[i] = NULL;
        }
    }
    return node;
}

void sanitize_word(const char *input, char *output) {
    int j = 0;
    for (int i = 0; input[i] != '\0'; i++) {
        char c = tolower((unsigned char)input[i]);
        if (c >= 'a' && c <= 'z') {
            output[j++] = c;
        }
    }
    output[j] = '\0';
}

void insert(TrieNode *root, const char *word) {
    char clean_word[100];
    sanitize_word(word, clean_word);
    
    if (strlen(clean_word) == 0) return;

    TrieNode *curr = root;
    for (int i = 0; clean_word[i] != '\0'; i++) {
        int index = clean_word[i] - 'a';
        if (!curr->children[index]) {
            curr->children[index] = create_node();
        }
        curr = curr->children[index];
    }
    curr->is_end_of_word = true;
}

bool search_and_track(TrieNode *root, const char *word) {
    char clean_word[100];
    sanitize_word(word, clean_word);
    if (strlen(clean_word) == 0) return false;

    TrieNode *curr = root;
    for (int i = 0; clean_word[i] != '\0'; i++) {
        int index = clean_word[i] - 'a';
        if (!curr->children[index]) return false;
        curr = curr->children[index];
    }

    if (curr->is_end_of_word) {
        curr->frequency++;
        return true;
    }
    return false;
}

void dfs_suggest(TrieNode *node, char *prefix_buffer, int depth, int *count) {
    if (*count >= MAX_SUGGESTIONS) return;

    if (node->is_end_of_word) {
        prefix_buffer[depth] = '\0';
        (*count)++;
        if (node->frequency > 0) {
            printf("  %d. %s (Searched %d times)\n", *count, prefix_buffer, node->frequency);
        } else {
            printf("  %d. %s\n", *count, prefix_buffer);
        }
    }

    for (int i = 0; i < ALPHABET_SIZE; i++) {
        if (node->children[i]) {
            prefix_buffer[depth] = 'a' + i;
            dfs_suggest(node->children[i], prefix_buffer, depth + 1, count);
            if (*count >= MAX_SUGGESTIONS) return;
        }
    }
}

void autocomplete(TrieNode *root, const char *prefix) {
    char clean_prefix[100];
    sanitize_word(prefix, clean_prefix);

    if (strlen(clean_prefix) == 0) {
        printf("Invalid prefix entered.\n");
        return;
    }

    // Start timer
    clock_t start_time = clock();

    TrieNode *curr = root;
    char buffer[100];
    int len = strlen(clean_prefix);

    for (int i = 0; i < len; i++) {
        int index = clean_prefix[i] - 'a';
        if (!curr->children[index]) {
            clock_t end_time = clock();
            double time_taken = ((double)(end_time - start_time)) / CLOCKS_PER_SEC * 1000.0;
            printf("No suggestions found starting with \"%s\" (Completed in %.3f ms)\n", clean_prefix, time_taken);
            return;
        }
        buffer[i] = clean_prefix[i];
        curr = curr->children[index];
    }

    int suggestion_count = 0;
    printf("\nTop suggestions for \"%s\":\n", clean_prefix);
    dfs_suggest(curr, buffer, len, &suggestion_count);

    if (search_and_track(root, clean_prefix)) {
        printf(" -> Updated frequency for exact word match '%s'!\n", clean_prefix);
    }

    // Stop timer
    clock_t end_time = clock();
    double time_taken = ((double)(end_time - start_time)) / CLOCKS_PER_SEC * 1000.0;
    printf("Execution Time: %.3f ms\n", time_taken);
}

void load_dictionary(TrieNode *root, const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        printf("Could not open '%s'. Loading built-in default words...\n", filename);
        insert(root, "apple");
        insert(root, "apply");
        insert(root, "application");
        insert(root, "aptitude");
        insert(root, "batman");
        return;
    }

    char line[100];
    int count = 0;
    while (fgets(line, sizeof(line), file)) {
        line[strcspn(line, "\r\n")] = '\0';
        if (strlen(line) > 0) {
            insert(root, line);
            count++;
        }
    }
    fclose(file);
    printf("Loaded %d words successfully!\n\n", count);
}

void free_trie(TrieNode *node) {
    if (!node) return;
    for (int i = 0; i < ALPHABET_SIZE; i++) {
        if (node->children[i]) {
            free_trie(node->children[i]);
        }
    }
    free(node);
}

int main(void) {
    TrieNode *root = create_node();

    load_dictionary(root, "words.txt");

    char query[50];
    while (1) {
        printf("\nType prefix to search (or 'exit' to quit): ");
        if (scanf("%49s", query) != 1 || strcmp(query, "exit") == 0) {
            break;
        }
        autocomplete(root, query);
    }

    free_trie(root);
    printf("\nExiting program.\n");
    return 0;
}