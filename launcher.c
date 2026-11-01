#include "launcher.h"

static const struct bread_launcher_entry entries[] = {
    {"Shell", "System", "Kernel command interface", 0},
    {"Files", "System", "Browse BreadFS files", 1},
    {"Appearance", "Personalization", "Choose desktop color theme", 2},
    {"System", "System", "View kernel and memory status", 3},
    {"Notifications", "System", "Review alerts and notification history", 4},
    {"Clipboard", "Productivity", "Browse and restore clipboard history", 5}
};

static char fold(char c) {
    return c >= 'A' && c <= 'Z' ? (char)(c - 'A' + 'a') : c;
}

static int score_text(const char *candidate, const char *query) {
    if (!query || !*query) return 0;
    int score = 0;
    int first = 1;
    size_t position = 0;
    for (; *query; ++query) {
        char wanted = fold(*query);
        while (candidate[position] && fold(candidate[position]) != wanted) {
            ++position;
            ++score;
        }
        if (!candidate[position]) return -1;
        if (first && position != 0u) score += 4;
        first = 0;
        ++position;
    }
    return score;
}

static int entry_score(const struct bread_launcher_entry *entry, const char *query) {
    int best = score_text(entry->name, query);
    int category = score_text(entry->category, query);
    int description = score_text(entry->description, query);
    if (category >= 0 && (best < 0 || category + 3 < best)) best = category + 3;
    if (description >= 0 && (best < 0 || description + 8 < best)) best = description + 8;
    return best;
}

const struct bread_launcher_entry *bread_launcher_entry_at(size_t index) {
    return index < sizeof(entries) / sizeof(entries[0]) ? &entries[index] : 0;
}

size_t bread_launcher_entry_count(void) {
    return sizeof(entries) / sizeof(entries[0]);
}

size_t bread_launcher_search(const char *query, size_t *indices, size_t capacity) {
    if (!indices || !capacity) return 0;
    size_t count = 0;
    for (size_t i = 0; i < bread_launcher_entry_count(); ++i) {
        int score = entry_score(&entries[i], query);
        if (score < 0) continue;
        if (count == capacity) {
            int worst = entry_score(&entries[indices[count - 1u]], query);
            if (score >= worst) continue;
        }
        size_t at = count < capacity ? count : capacity - 1u;
        while (at > 0u) {
            int previous = entry_score(&entries[indices[at - 1u]], query);
            if (previous <= score) break;
            indices[at] = indices[at - 1u];
            --at;
        }
        indices[at] = i;
        if (count < capacity) ++count;
    }
    return count;
}
