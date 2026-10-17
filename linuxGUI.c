#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>

#define ALPHABET_SIZE 26
#define MAX_SUGGESTIONS 10

// --- TRIE ENGINE ---
typedef struct TrieNode {
    struct TrieNode *children[ALPHABET_SIZE];
    bool is_end_of_word;
} TrieNode;

TrieNode* root = NULL;

TrieNode* create_node(void) {
    TrieNode *node = (TrieNode *)malloc(sizeof(TrieNode));
    if (node) {
        node->is_end_of_word = false;
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

void insert(TrieNode *r, const char *word) {
    char clean_word[100];
    sanitize_word(word, clean_word);
    if (strlen(clean_word) == 0) return;

    TrieNode *curr = r;
    for (int i = 0; clean_word[i] != '\0'; i++) {
        int index = clean_word[i] - 'a';
        if (!curr->children[index]) {
            curr->children[index] = create_node();
        }
        curr = curr->children[index];
    }
    curr->is_end_of_word = true;
}

void dfs_suggest(TrieNode *node, char *prefix_buffer, int depth, GtkListStore *store, int *count) {
    if (*count >= MAX_SUGGESTIONS) return;

    if (node->is_end_of_word) {
        prefix_buffer[depth] = '\0';
        GtkTreeIter iter;
        gtk_list_store_append(store, &iter);
        gtk_list_store_set(store, &iter, 0, prefix_buffer, -1);
        (*count)++;
    }

    for (int i = 0; i < ALPHABET_SIZE; i++) {
        if (node->children[i]) {
            prefix_buffer[depth] = 'a' + i;
            dfs_suggest(node->children[i], prefix_buffer, depth + 1, store, count);
            if (*count >= MAX_SUGGESTIONS) return;
        }
    }
}

void autocomplete_gui(GtkListStore *store, const char *prefix) {
    // Clear list view
    gtk_list_store_clear(store);

    char clean_prefix[100];
    sanitize_word(prefix, clean_prefix);
    if (strlen(clean_prefix) == 0) return;

    TrieNode *curr = root;
    char buffer[100];
    int len = strlen(clean_prefix);

    for (int i = 0; i < len; i++) {
        int index = clean_prefix[i] - 'a';
        if (!curr->children[index]) return;
        buffer[i] = clean_prefix[i];
        curr = curr->children[index];
    }

    int count = 0;
    dfs_suggest(curr, buffer, len, store, &count);
}

void load_dictionary(TrieNode *r, const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        insert(r, "apple");
        insert(r, "apply");
        insert(r, "application");
        insert(r, "aptitude");
        insert(r, "batman");
        return;
    }
    char line[100];
    while (fgets(line, sizeof(line), file)) {
        line[strcspn(line, "\r\n")] = '\0';
        if (strlen(line) > 0) insert(r, line);
    }
    fclose(file);
}

// --- GTK 3 EVENT HANDLERS ---
static void on_search_changed(GtkSearchEntry *entry, gpointer user_data) {
    GtkListStore *store = GTK_LIST_STORE(user_data);
    const char *text = gtk_entry_get_text(GTK_ENTRY(entry));
    autocomplete_gui(store, text);
}

int main(int argc, char *argv[]) {
    // 1. Initialize Trie
    root = create_node();
    load_dictionary(root, "words.txt");

    // 2. Initialize GTK
    gtk_init(&argc, &argv);

    // Create top-level Window
    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Trie Autocomplete (Native GNOME)");
    gtk_window_set_default_size(GTK_WINDOW(window), 450, 400);
    gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    // Main layout container (Vertical Box)
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 15);
    gtk_container_add(GTK_CONTAINER(window), vbox);

    // Heading Label
    GtkWidget *label = gtk_label_new("Type prefix to search dictionary:");
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);

    // List Store & TreeView (For Suggestions)
    GtkListStore *store = gtk_list_store_new(1, G_TYPE_STRING);
    GtkWidget *tree_view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
    GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
    GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes("Suggestions", renderer, "text", 0, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(tree_view), column);

    // Search Entry Box
    GtkWidget *search_entry = gtk_search_entry_new();
    g_signal_connect(search_entry, "search-changed", G_CALLBACK(on_search_changed), store);
    gtk_box_pack_start(GTK_BOX(vbox), search_entry, FALSE, FALSE, 0);

    // Scrollable Window for List
    GtkWidget *scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(scrolled_window), tree_view);
    gtk_box_pack_start(GTK_BOX(vbox), scrolled_window, TRUE, TRUE, 0);

    // Show all widgets & start GTK loop
    gtk_widget_show_all(window);
    gtk_main();

    return 0;
}
