#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <stdbool.h> // For bool type

#define DECOY_AMOUNT 4

// Color codes
#define COLOR_RESET  "\033[0m"
#define COLOR_RED    "\033[31m"
#define COLOR_GREEN  "\033[32m"
#define COLOR_YELLOW "\033[33m"
#define COLOR_BLUE   "\033[34m"
#define COLOR_MAGENTA "\033[35m"
#define COLOR_CYAN   "\033[36m"
#define COLOR_WHITE  "\033[37m"

volatile sig_atomic_t interrupted = 0;

typedef struct {
    bool fastScan;
    bool aggressiveScan;
    bool fragmentScan;
    bool ipRandom;
    char ip[16]; // Add this line to store an IP address
} Arguments;


void printUsage() {
    printf("Usage: scan [options]\n");
    printf("Options:\n");
    printf("  -fast      Enable fast scanning mode\n");
    printf("  -r         Use random IP generation\n");
    printf("  -a         Aggressive scan\n");
    printf("  -f         Fragment scan\n");
    printf("  -ip [ip]   Scan a specific IP\n");
    // Add more options as needed
}

void signal_handler(int sig) {
    interrupted = 1;
}

void parseArguments(int argc, char *argv[], Arguments *args) {
    args->fastScan = false;  // Default value
    args->ipRandom = false; // Default is empty, indicating no specific range
    args->aggressiveScan = false;
    args->fragmentScan = false;
    args->ip[0] = '\0'; // Initialize the IP string as empty

    if (argc == 1) {
        // No arguments were passed
        printUsage();
        exit(EXIT_FAILURE);
    }

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-fast") == 0) {
            args->fastScan = true;
        } else if (strcmp(argv[i], "-r") == 0) {
            args->ipRandom = true;
        } else if (strcmp(argv[i], "-f") == 0) {
            args->fragmentScan = true;
        } else if (strcmp(argv[i], "-a") == 0) {
            args->aggressiveScan = true;
        } else if (strcmp(argv[i], "-ip") == 0) {
            if (i + 1 < argc) { // Make sure there is another argument after -ip
                strncpy(args->ip, argv[i + 1], sizeof(args->ip) - 1);
                args->ip[sizeof(args->ip) - 1] = '\0'; // Ensure null-termination
                i++; // Skip the next argument since we've just processed it
            } else {
                printf("Error: -ip option requires an IP address.\n");
                printUsage();
                exit(EXIT_FAILURE);
            }
        } else {
            printf("Invalid argument: %s\n", argv[i]);
            printUsage();
            exit(EXIT_FAILURE);
        }
    }
}

void generate_random_ip(char *ip) {
    int b1 = rand() % 256;
    int b2 = rand() % 256;
    int b3 = rand() % 256;
    int b4 = rand() % 256;
    sprintf(ip, "%d.%d.%d.%d", b1, b2, b3, b4);
}

void generate_decoys(char decoys[DECOY_AMOUNT][16]) {
    for (int i = 0; i < DECOY_AMOUNT; i++) {
        generate_random_ip(decoys[i]);
    }
}

int isIPReachable(const char *ip) {
    char cmd[256];
    printf("Pinging %s\n",ip);
    snprintf(cmd, sizeof(cmd), "ping -c 1 %s > /dev/null 2>&1", ip);
    return system(cmd) == 0; // Returns 0 if the IP is reachable
}

int executeNmapAndCheckOutput(const char* cmd) {
    FILE *fp = popen(cmd, "r");
    if (fp == NULL) {
        printf("Failed to run command\n");
        return 1;
    }

    char output[1024];
    bool errorDetected = false;
    while (fgets(output, sizeof(output), fp) != NULL) {
        if (strstr(output, "Operation not permitted")) {
            errorDetected = true;
            printf("Error detected in nmap output.\n");
            break;
        }
    }

    pclose(fp);
    return errorDetected ? 1 : 0;
}

int main(int argc, char *argv[]) {
    Arguments args;
    parseArguments(argc, argv, &args);

    srand(time(NULL)); // Seed the random number generator
    signal(SIGINT, signal_handler);

    char ip[16]; // To hold the generated IP address
    char decoys[DECOY_AMOUNT][16]; // Array to hold the generated decoy IP addresses
    char decoyCmdPart[256]; // To build the decoy part of the nmap command dynamically

    bool generateIp = true;
    if (args.ip[0] != '\0') {
        printf("IP address provided: %s\n", args.ip);
        strncpy(ip, args.ip, sizeof(ip) - 1);
        ip[sizeof(ip) - 1] = '\0'; // Ensure null-termination
        generateIp = false;
    } 

    while (!interrupted) {
        if(generateIp) {
            generate_random_ip(ip);
        }
        // if(isIPReachable(ip) != 0){
        //     printf(COLOR_RED "Ip is not reachable." COLOR_RESET "\n");
        //     if(generateIp){
        //         sleep(1);
        //         continue;
        //     }else{
        //         break;
        //     }
        // }
        printf(COLOR_YELLOW "-----------------------------------" COLOR_RESET "\n");
        printf(COLOR_YELLOW "Scanning IP: %s" COLOR_RESET "\n", ip);
        printf(COLOR_YELLOW "-----------------------------------" COLOR_RESET "\n");

        generate_decoys(decoys); // Generate decoy IPs
        // Dynamically construct the decoy command part based on DECOY_AMOUNT
        strcpy(decoyCmdPart, "-D");
        for (int i = 0; i < DECOY_AMOUNT; i++) {
            strcat(decoyCmdPart, decoys[i]);
            if (i < DECOY_AMOUNT - 1) {
                strcat(decoyCmdPart, ",");
            }
        }

        char tempFilename[] = "/tmp/temp_scan_results.txt";
        char cmd[512];
        char scanOptions[256] = ""; 
        strcat(scanOptions, args.fastScan ? "-T4 " : "-sS -sV ");
        strcat(scanOptions, args.aggressiveScan ? "-A " : "");
        strcat(scanOptions, args.fragmentScan ? "-f " : "");
        const char* const ALL_USEFUL_PORTS = "21,22,23,25,53,110,143,139,445,3306,5432,3389";

        printf(COLOR_BLUE "\nFUNCTION: nmap %s" COLOR_RESET "\n", scanOptions);
        sleep(1);
        // Construct and execute the nmap command
        snprintf(cmd, sizeof(cmd), "nmap %s %s -p%s %s | grep open > %s", 
        scanOptions, decoyCmdPart, ALL_USEFUL_PORTS, ip, tempFilename);
        system(cmd); // Execute nmap and direct output to temporary file
        sleep(5);

        // Check if the temporary file has content (indicating open ports were found)
        if (access(tempFilename, F_OK) != -1) {
            FILE *file = fopen(tempFilename, "r");
            fseek(file, 0, SEEK_END);
            long size = ftell(file);
            if (size > 0) {
                printf(COLOR_GREEN "\n------------------------------------------" COLOR_RESET "\n");
                printf(COLOR_GREEN "[%s]\nSuccessful scan >:)" COLOR_RESET "\n", ip);
                printf(COLOR_GREEN "------------------------------------------" COLOR_RESET "\n");
                FILE *resultFile = fopen("scanned_targets.txt", "a");
                fprintf(resultFile, "%s\n", ip);
                fseek(file, 0, SEEK_SET);
                char ch;
                while ((ch = fgetc(file)) != EOF) {
                    fputc(ch, resultFile);
                }
                fprintf(resultFile, "\n");
                fclose(resultFile);
            }
            fclose(file);
            unlink(tempFilename); // Delete the temporary file
        }
        
        // If IP range scanning isn't random, break after first complete scan
        if (args.ipRandom == false) {
            break;
        }
    }

    return 0;
}
