#include "menu.h"

static const struct shell_menu_entry entries[] = {
    {"System","about","Show BreadOS kernel and boot information"},
    {"System","video","Show framebuffer mode and graphics status"},
    {"System","memory","Show available physical memory frames"},
    {"System","storage","Show active storage backend"},
    {"System","uptime","Show system timer ticks"},
    {"System","tasks","Show scheduler task-switch count"},
    {"System","userdemo","Run the ring-3 and syscall smoke program"},
    {"System","reboot","Restart BreadOS"},
    {"System","halt","Halt the processor"},
    {"Files","ls","List files in the current storage volume"},
    {"Files","cat NAME","Print a file"},
    {"Files","write NAME TEXT","Create or replace a text file"},
    {"Files","mkdir PATH","Create a directory"},
    {"Files","rmdir PATH","Remove an empty directory"},
    {"Files","mv SOURCE DEST","Move a file"},
    {"Files","rm PATH","Remove a file"},
    {"Appearance","theme","List installed native color themes"},
    {"Appearance","theme NAME","Apply and persist a color theme"},
    {"Appearance","clear","Clear the console"},
    {"Help","help","Show command usage"},
    {"Help","menu [SEARCH]","Search the native BreadOS command palette"},
    {"Help","history","Show recent shell commands"},
    {"Desktop","desktop","Open the native floating-window desktop"},
    {"Notifications","notify TEXT","Create a native system notification"},
    {"Notifications","notifications","Review retained notification history"},
    {"Notifications","dnd [on|off]","Toggle Do Not Disturb"},
    {"Notifications","snooze INDEX TICKS","Temporarily hide a retained notification"},
    {"Notifications","dismiss INDEX","Dismiss a retained notification"},
    {"Clipboard","clipboard copy|sensitive|paste|history|clear","Manage native clipboard contents and retained history"},
    {"Reminders","remind TICKS TEXT","Schedule a reminder using the system timer"},
    {"Reminders","reminders","List pending reminders"},
    {"Reminders","remind clear","Clear pending reminders"},
    {"Desktop","idle [TICKS|off|status]","Configure the desktop screensaver idle timeout"},
    {"Privacy","privacy","List native device permission policy; permissions default to denied"},
    {"Privacy","privacy CAPABILITY on|off","Change and persist a device permission policy"}
};

static char lower_ascii(char value) {
    return value>='A' && value<='Z' ? (char)(value-'A'+'a') : value;
}

static int contains(const char *text,const char *query) {
    if (!query || !*query) return 1;
    if (!text) return 0;
    for (size_t start=0;text[start];start++) {
        size_t i=0;
        while (query[i] && text[start+i] &&
               lower_ascii(text[start+i])==lower_ascii(query[i])) i++;
        if (!query[i]) return 1;
    }
    return 0;
}

size_t shell_menu_count(void) {
    return sizeof(entries)/sizeof(entries[0]);
}

const struct shell_menu_entry *shell_menu_at(size_t index) {
    return index<shell_menu_count() ? &entries[index] : 0;
}

int shell_menu_matches(const struct shell_menu_entry *entry,const char *query) {
    if (!entry) return 0;
    if (!query || !*query) return 1;
    return contains(entry->category,query) || contains(entry->command,query) ||
           contains(entry->description,query);
}
