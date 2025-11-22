#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <windows.h>

// ---------------------- SETTINGS STRUCT ----------------------
typedef struct {
    int scanInterval;    
    int clearDelay;   
    int enableLogging;  
    int maxScans;        
} Settings;

Settings appSettings = {3, 5, 1, 10}; 

// ---------------------- UTILITY FUNCTIONS ----------------------

// Sleep wrapper (Windows only)
void sleepSeconds(int sec) {
    Sleep(sec * 1000);
}

// Trim leading and trailing spaces/newlines
void trim(char *str) {
    char *end;
    while (isspace((unsigned char)*str)) str++;  
    if (*str == 0) return;
    end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;
    *(end + 1) = 0;
}

// Log event to log.txt
void writeLog(const char *message) {
    if (!appSettings.enableLogging) return;
    FILE *log = fopen("log.txt", "a");
    if (!log) return;
    time_t now = time(NULL);
    fprintf(log, "[%s] %s\n", ctime(&now), message);
    fclose(log);
}

// ---------------------- SENSITIVE DATA CHECKS ----------------------

// Check for email pattern
int containsEmail(char *text) {
    char *at = strstr(text, "@");
    if (at && strstr(at + 1, ".")) return 1;
    return 0;
}

// Check for long numeric sequences
int containsLongDigitSequence(char *text) {
    int digits = 0;
    for (int i = 0; text[i]; i++) {
        if (isdigit(text[i])) {
            digits++;
            if (digits >= 12) return 1;
        } else {
            digits = 0;
        }
    }
    return 0;
}

// Check for common sensitive keywords
int containsKeywords(char *text) {
    const char *keywords[] = {
        "password", "pwd", "secret", "token",
        "apikey", "api_key", "cnic", "card",
        "visa", "mastercard", NULL
    };
    for (int i = 0; keywords[i] != NULL; i++) {
        if (strstr(text, keywords[i])) return 1;
    }
    return 0;
}

// Combined sensitive pattern check
int containsSensitivePattern(char *text) {
    return containsEmail(text) || containsLongDigitSequence(text) || containsKeywords(text);
}

// ---------------------- CLIPBOARD FUNCTIONS ----------------------

// Fetch current clipboard into clip.txt
int fetchClipboard() {
    return system("powershell Get-Clipboard > clip.txt 2>nul") == 0;
}

// Read clipboard text from file
int readClipboardFile(char *buffer, int size) {
    FILE *fp = fopen("clip.txt", "r");
    if (!fp) return 0;
    fgets(buffer, size, fp);
    fclose(fp);
    trim(buffer);
    return 1;
}

// Clear clipboard
void clearClipboard() {
    system("echo off | clip");
    writeLog("Clipboard cleared due to sensitive data.");
}

// ---------------------- DISPLAY FUNCTIONS ----------------------

// Banner at start
void showBanner() {
    printf("=====================================================\n");
    printf("        Advanced Smart Clipboard Cleaner\n");
    printf("=====================================================\n");
    printf(" Scan Interval : %d sec\n", appSettings.scanInterval);
    printf(" Clear Delay   : %d sec\n", appSettings.clearDelay);
    printf(" Logging       : %s\n", appSettings.enableLogging ? "Enabled" : "Disabled");
    printf(" Max Scans     : %s\n", appSettings.maxScans == 0 ? "Infinite" : "Limited");
    printf("-----------------------------------------------------\n");
}

// Alert when sensitive data detected
void showDetectionAlert() {
    printf("\nSensitive data detected!\n");
    printf(" Clearing clipboard in %d seconds...\n", appSettings.clearDelay);
}

// ---------------------- MAIN LOOP ----------------------
int main() {
    char clipboard[2048];
    int scanCount = 0;

    showBanner();
    writeLog("Application started.");

    while (1) {
        scanCount++;

        if (!fetchClipboard()) {
            printf("Unable to access clipboard.\n");
            sleepSeconds(appSettings.scanInterval);
            continue;
        }

        if (!readClipboardFile(clipboard, sizeof(clipboard))) {
            printf("Failed to read clipboard.\n");
            sleepSeconds(appSettings.scanInterval);
            continue;
        }

        if (strlen(clipboard) == 0) {
            printf("Clipboard is empty.\n");
            sleepSeconds(appSettings.scanInterval);
            continue;
        }

        printf("Clipboard content: %s\n", clipboard);

        if (containsSensitivePattern(clipboard)) {
            showDetectionAlert();
            writeLog("Sensitive content detected in clipboard.");
            sleepSeconds(appSettings.clearDelay);
            clearClipboard();
            printf("Clipboard cleared for your safety.\n\n");
        } else {
            printf("No sensitive data detected.\n\n");
        }

        // Auto-exit if maxScans > 0
        if (appSettings.maxScans > 0 && scanCount >= appSettings.maxScans) {
            printf("Max scans reached. Exiting program.\n");
            writeLog("Program exited after max scans.");
            break;
        }
        sleepSeconds(appSettings.scanInterval);
    }

    return 0;
}
