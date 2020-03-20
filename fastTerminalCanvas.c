#include <time.h>
#include <math.h>
#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <stdlib.h>
#include <windows.h>
#include "fastTerminalCanvas.h"

CONSOLE_SCREEN_BUFFER_INFO csbi;
int G_fileRead = 0;
int G_fileLength;
int G_characterWidth;
int G_characterHeight;
int G_characterLength;
int G_characterCount;
int G_windowWidth;
int G_windowHeight;
char *G_characterChars = NULL;
char **G_characterCharMaps = NULL;
int G_outDimX;
int G_outDimY;
double G_convX;
double G_convY;
char *G_output = NULL;
double *G_singleLine = NULL;
char *G_singleChar = NULL;
int *G_bounds = NULL;

char** readFile(char* fileName) {
    FILE *fp = fopen(fileName, "rt");
    assert(fp);
    fseek(fp, 0, SEEK_END);
    long length = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    char *buffer = malloc((length + 1) * sizeof(char));
    buffer[length] = '\0';
    if (fread(buffer, sizeof(char), length, fp)) {
        fclose(fp);
        G_fileLength = 1;
        int lineCount = 0;
        char *line;
        char **file = malloc(1 * sizeof(char*));
        file[0] = buffer;
        for (int i  = 0; i < length; i++) {
            if (buffer[i] == '\n') {
                buffer[i] = '\0';
                G_fileLength++;
                file = realloc(file, G_fileLength * sizeof(char*));
                file[G_fileLength - 1] = buffer + i + 1;
            };
        };
        return file;
    };
    fclose(fp);
    printf("Error reading file!\n");
    return NULL;
};

void initChars(char* fileName) {
    char **file = readFile(fileName);
    G_characterWidth = atoi(file[0]);
    G_characterHeight = atoi(file[1]);
    G_characterLength = G_characterWidth * G_characterHeight;
    G_characterCount = G_fileLength - 2;
    G_characterChars = malloc(G_characterCount * sizeof(char));
    G_characterCharMaps = malloc(G_characterCount * sizeof(char*));
    int *imageStr;
    for (int i = 2; i < G_fileLength; i++) {
        G_characterChars[i - 2] = file[i][0];
        G_characterCharMaps[i - 2] = &file[i][1];
    };
    free(file);
};

int compare(char* charA, int charBIndex) {
    int sum = 0;
    for (int i = 0; i < G_characterLength; i++) {
        if (charA[i] == G_characterCharMaps[charBIndex][i]) { sum++; }
        else { sum--; };
    };
    return sum;
};

char getChar(char* image) {
    int highestScore = compare(image, 0);
    int highestIndex = 0;
    if (highestScore == G_characterLength) { return G_characterChars[highestIndex]; };
    for (int i = 0; i < G_characterCount; i++) {
        if (compare(image, i) > highestScore) {
            highestScore = compare(image, i);
            highestIndex = i;
        };
    };
    return G_characterChars[highestIndex];
};

int lineIntersect(double x1, double y1, double x2, double y2, double x3, double y3, double x4, double y4) {
        if ((x1 == x2 && y1 == y2) || (x3 == x4 && y3 == y4)) {
            return 0;
        };
        double denominator = ((y4 - y3) * (x2 - x1) - (x4 - x3) * (y2 - y1));
        if (denominator == 0) {
            return 0;
        };
        double ua = ((x4 - x3) * (y1 - y3) - (y4 - y3) * (x1 - x3)) / denominator;
        double ub = ((x2 - x1) * (y1 - y3) - (y2 - y1) * (x1 - x3)) / denominator;
        if (ua < 0 || ua > 1 || ub < 0 || ub > 1) {
            return 0;
        };
        return 1;
};

void resizeCanvas(int inDimX, int inDimY) {
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
    G_windowWidth = csbi.srWindow.Right - csbi.srWindow.Left + 1;
    G_windowHeight = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
    G_outDimX = G_windowWidth * G_characterWidth;
    G_outDimY = G_windowHeight * G_characterHeight;
    G_convX = (double)G_outDimX / (double)inDimX;
    G_convY = (double)G_outDimY / (double)inDimY;
    G_output = realloc(G_output, (G_windowWidth * G_windowHeight + 1) * sizeof(char));
    G_singleLine = realloc(G_singleLine, 4 * sizeof(double));
    G_singleChar = realloc(G_singleChar, G_characterLength * sizeof(char));
    G_bounds = realloc(G_bounds, 4 * sizeof(int));
};

void initCanvas(char* filename, int inDimX, int inDimY) {
    if (G_fileRead) { return; }
    else { G_fileRead = 1; };
    initChars(filename);
    resizeCanvas(inDimX, inDimY);
};

void printScreen(double* screenVector, int lineCount, int delayTime) {
    long pause;
    clock_t now,then;
    pause = delayTime * (CLOCKS_PER_SEC / 1000);
    now = then = clock();
    char pixel;
    int isBlank;
    for (int i = 0; i < lineCount * 4; i += 2) {
        screenVector[i] = screenVector[i] * G_convX;
        screenVector[i + 1] = screenVector[i + 1] * G_convY;
    };
    for (int charY = 0; charY < G_windowHeight; charY++) {
        for (int charX = 0; charX < G_windowWidth; charX++) {
            G_bounds[0] = charX * G_characterWidth - 0.5;
            G_bounds[1] = charY * G_characterHeight - 0.5;
            G_bounds[2] = (charX + 1) * G_characterWidth - 0.5;
            G_bounds[3] = (charY + 1) * G_characterHeight - 0.5;
            isBlank = 1;
            for (int i = 0; i < lineCount * 4; i += 4) {
                if (lineIntersect(G_bounds[0], G_bounds[1], G_bounds[2], G_bounds[3],
                    screenVector[i], screenVector[i + 1], screenVector[i + 2], screenVector[i + 3])

                    || lineIntersect(G_bounds[0], G_bounds[3], G_bounds[2], G_bounds[1],
                    screenVector[i], screenVector[i + 1], screenVector[i + 2], screenVector[i + 3])

                    || (screenVector[i] >= G_bounds[0] && screenVector[i] <= G_bounds[2]
                    && screenVector[i + 1] >= G_bounds[1] && screenVector[i + 1] <= G_bounds[3])

                    || (screenVector[i + 2] >= G_bounds[0] && screenVector[i + 2] <= G_bounds[2]
                    && screenVector[i + 3] >= G_bounds[1] && screenVector[i + 3] <= G_bounds[3]))
                {
                    isBlank = 0;
                    break;
                };
            };
            if (!isBlank) {
                for (int y = 0; y < G_characterHeight; y++) {
                    for (int x = 0; x < G_characterWidth; x++) {
                        pixel = '0';
                        for (int i = 0; i < lineCount * 4; i += 4) {
                            G_bounds[0] = charX * G_characterWidth + x - 0.5;
                            G_bounds[1] = charY * G_characterHeight + y - 0.5;
                            G_bounds[2] = charX * G_characterWidth + x + 0.5;
                            G_bounds[3] = charY * G_characterHeight + y + 0.5;
                            if (lineIntersect(G_bounds[0], G_bounds[1], G_bounds[2], G_bounds[3],
                            screenVector[i], screenVector[i + 1], screenVector[i + 2], screenVector[i + 3])

                            || lineIntersect(G_bounds[0], G_bounds[3], G_bounds[2], G_bounds[1],
                            screenVector[i], screenVector[i + 1], screenVector[i + 2], screenVector[i + 3])

                            || (screenVector[i] >= G_bounds[0] && screenVector[i] <= G_bounds[2]
                            && screenVector[i + 1] >= G_bounds[1] && screenVector[i + 1] <= G_bounds[3])

                            || (screenVector[i + 2] >= G_bounds[0] && screenVector[i + 2] <= G_bounds[2]
                            && screenVector[i + 3] >= G_bounds[1] && screenVector[i + 3] <= G_bounds[3]))
                            {
                                pixel = '1';
                                break;
                            };
                        };
                        G_singleChar[x + y * G_characterWidth] = pixel;
                    };
                };
                G_output[charX + charY * G_windowWidth] = getChar(G_singleChar);
            } else { G_output[charX + charY * G_windowWidth] = ' '; };
        };
    };
    for (int i = 0; i < lineCount * 4; i += 2) {
        screenVector[i] = screenVector[i] / G_convX;
        screenVector[i + 1] = screenVector[i + 1] / G_convY;
    };
    G_output[G_windowWidth * G_windowHeight] = '\0';
    printf("%s\n", G_output);
    while( (now-then) < pause ) { now = clock(); };
};
