#ifndef TERMINALCANVAS_H
#define TERMINALCANVAS_H 

void initCanvas(char* filename, int inDimX, int inDimY);
void resizeCanvas(int inDimX, int inDimY);
void printScreen(double* screenVector, int lineCount, int delayTime);
int lineIntersect(double x1, double y1, double x2, double y2, double x3, double y3, double x4, double y4);

#endif
