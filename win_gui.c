#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>

#define ALPHABET_SIZE 26
#define MAX_SUGGESTIONS 10

// ID constants for Windows GUI controls
#define ID_INPUT_BOX 101
#define ID_LIST_BOX  102

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

void dfs_suggest(TrieNode *node, char *prefix_buffer, int depth, HWND hListBox, int *count) {
    if (*count >= MAX_SUGGESTIONS) return;

    if (node->is_end_of_word) {
        prefix_buffer[depth] = '\0';
        // Add matching string directly into the native Windows ListBox
        SendMessage(hListBox, LB_ADDSTRING, 0, (LPARAM)prefix_buffer);
        (*count)++;
    }

    for (int i = 0; i < ALPHABET_SIZE; i++) {
        if (node->children[i]) {
            prefix_buffer[depth] = 'a' + i;
            dfs_suggest(node->children[i], prefix_buffer, depth + 1, hListBox, count);
            if (*count >= MAX_SUGGESTIONS) return;
        }
    }
}

void autocomplete_gui(HWND hListBox, const char *prefix) {
    // Clear list box
    SendMessage(hListBox, LB_RESETCONTENT, 0, 0);

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
    dfs_suggest(curr, buffer, len, hListBox, &count);
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

// --- WIN32 GUI EVENT HANDLER ---
HWND hInput, hList;

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CREATE:
            // Title Label
            CreateWindow("STATIC", "Type prefix to search:", 
                         WS_VISIBLE | WS_CHILD, 
                         30, 20, 200, 20, hwnd, NULL, NULL, NULL);

            // Native Text Input Box
            hInput = CreateWindow("EDIT", "", 
                                  WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL, 
                                  30, 45, 320, 25, hwnd, (HMENU)ID_INPUT_BOX, NULL, NULL);

            // Native Suggestions List Box
            hList = CreateWindow("LISTBOX", "", 
                                 WS_VISIBLE | WS_CHILD | WS_BORDER | LBS_NOTIFY | WS_VSCROLL, 
                                 30, 85, 320, 180, hwnd, (HMENU)ID_LIST_BOX, NULL, NULL);
            break;

        case WM_COMMAND:
            // Triggers every time user types or deletes a character in the text box
            if (LOWORD(wParam) == ID_INPUT_BOX && HIWORD(wParam) == EN_CHANGE) {
                char text[100];
                GetWindowText(hInput, text, sizeof(text));
                autocomplete_gui(hList, text);
            }
            break;

        case WM_DESTROY:
            PostQuitMessage(0);
            break;

        default:
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // Load Trie data
    root = create_node();
    load_dictionary(root, "words.txt");

    // Register Window Class
    const char CLASS_NAME[] = "TrieGuiWindow";
    WNDCLASS wc = {0};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

    RegisterClass(&wc);

    // Create Main Window
    HWND hwnd = CreateWindowEx(
        0, CLASS_NAME, "Trie Autocomplete (Native Windows GUI)",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 400, 330,
        NULL, NULL, hInstance, NULL
    );

    if (hwnd == NULL) return 0;

    ShowWindow(hwnd, nCmdShow);

    // Main Windows Message Loop
    MSG msg = {0};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}